
/**
 * @file lv_draw_bitblt.c
 *
 */

/**
 * Copyright 2024 Nuvoton
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_bitblt.h"

#if LV_USE_DRAW_BITBLT
#include "../lv_draw_buf_private.h"

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_BITBLT     254

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*
 * Evaluate a task and set the score and preferred BITBLT unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _bitblt_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the BITBLT unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _bitblt_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the BITBLT draw unit.
 */
static int32_t _bitblt_delete(lv_draw_unit_t *draw_unit);

#if LV_USE_OS
    static void _bitblt_render_thread_cb(void *ptr);
#endif

static void _bitblt_execute_drawing(lv_draw_task_t *t);

static void _bitblt_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_bitblt_init(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb  = _bitblt_invalidate_cache;

    lv_draw_bitblt_unit_t *draw_bitblt_unit = lv_draw_create_unit(sizeof(lv_draw_bitblt_unit_t));
    draw_bitblt_unit->base_unit.evaluate_cb = _bitblt_evaluate;
    draw_bitblt_unit->base_unit.dispatch_cb = _bitblt_dispatch;
    draw_bitblt_unit->base_unit.delete_cb = _bitblt_delete;

#if LV_USE_OS
    void bitbltInterruptInit(void);
    bitbltInterruptInit();
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_bitblt_unit->thread_dsc;
    thread_dsc->idx = 0;
    thread_dsc->draw_unit = (void *) draw_bitblt_unit;
    lv_thread_init(&thread_dsc->thread, "blt_draw", LV_DRAW_THREAD_PRIO, _bitblt_render_thread_cb, LV_DRAW_THREAD_STACK_SIZE, thread_dsc);
#endif
}

void lv_draw_bitblt_deinit(void)
{
#if LV_USE_OS
    void bitbltInterruptDeinit(void);
    bitbltInterruptDeinit();
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline bool _bitblt_src_cf_supported(lv_color_format_t cf)
{
    bool is_cf_supported = false;

    switch (cf)
    {
    case LV_COLOR_FORMAT_RGB565:
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_XRGB8888:
        is_cf_supported = true;
        break;
    default:
        break;
    }

    return is_cf_supported;
}

static inline bool _bitblt_dest_cf_supported(lv_color_format_t cf)
{
    bool is_cf_supported = false;

    switch (cf)
    {
    case LV_COLOR_FORMAT_RGB565:
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_XRGB8888:
        is_cf_supported = true;
        break;
    default:
        break;
    }

    return is_cf_supported;
}

static bool _bitblt_draw_img_supported(const lv_draw_image_dsc_t *draw_dsc)
{
    const lv_image_dsc_t *img_dsc = draw_dsc->src;

    bool has_recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);

    bool has_transform = (/*draw_dsc->rotation != 0 ||*/ draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE);
    bool has_opa = (draw_dsc->opa < (lv_opa_t)LV_OPA_MAX);
    bool src_has_alpha = (img_dsc->header.cf == LV_COLOR_FORMAT_ARGB8888);

    /* Recolor and transformation are not supported at the same time. */
    if (has_recolor || has_transform)
        return false;

    return true;
}

static bool _bitblt_buf_aligned(const void *buf, uint32_t stride)
{
    /* Test for pointer alignment */
    if ((uintptr_t)buf % 4)
        return false;

    /* Test for invalid stride (no stride alignment required) */
    if ((stride == 0) || (stride % 4))
        return false;

    return true;
}

void dump_draw_info(
    const char *str1,
    const char *str2,
    const lv_area_t *area,
    const lv_draw_buf_t *draw_buf)
{
    return;

    LV_LOG_USER("[%s]%s", str1, str2);

    if (area)
    {
        LV_LOG_USER("\tarea(%d-%d): (%d, %d) -> (%d, %d)",
                    lv_area_get_width(area),
                    lv_area_get_height(area),
                    area->x1,
                    area->y1,
                    area->x2,
                    area->y2);
    }

    if (draw_buf)
    {
        LV_LOG_USER("\tdraw_buf@%08x, sz: %d",
                    draw_buf->data,
                    draw_buf->data_size);

        LV_LOG_USER("\t\theader(%08x): cf: %d, w:%d, h:%d, stride:%d",
                    draw_buf->header.magic,
                    draw_buf->header.cf,
                    draw_buf->header.w,
                    draw_buf->header.h,
                    draw_buf->header.stride);
    }
}

static int32_t _bitblt_evaluate(lv_draw_unit_t *u, lv_draw_task_t *task)
{
    LV_UNUSED(u);

    const lv_draw_dsc_base_t *draw_dsc_base = (lv_draw_dsc_base_t *) task->draw_dsc;

    /* Check capacity. */
    if (!_bitblt_dest_cf_supported(draw_dsc_base->layer->color_format))
        return 0;

    uint8_t px_size = lv_color_format_get_size(draw_dsc_base->layer->color_format);

    lv_area_t blend_area;
    lv_area_copy(&blend_area, &draw_dsc_base->layer->buf_area);
    uint32_t blend_area_stride = lv_area_get_width(&blend_area) * px_size;

//    LV_LOG_USER("[%d] px_size:%d, dest_cf:%d, blend_area_stride:%d, blend.x:%d",
//                task->type,
//                px_size,
//                draw_dsc_base->layer->color_format,
//                blend_area_stride,
//                blend_area.x1);

    /* for bitblt limitation. */
    if (px_size == 2)
    {
        /* Check Hardware constraint: The stride must be a word-alignment. */
        bool bAlignedWord = ((blend_area_stride & 0x3) == 0) &&
                            (((blend_area.x1 * px_size) & 0x3) == 0) ? true : false;

        if (!bAlignedWord)
            goto _bitblt_evaluate_not_ok;
    }

    switch (task->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
    {
        const lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *) task->draw_dsc;

        if ((draw_dsc->radius != 0) ||
                (draw_dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE))
            goto _bitblt_evaluate_not_ok;
    }
    break;

    case LV_DRAW_TASK_TYPE_LAYER:
    {
        const lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *) task->draw_dsc;
        lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;

        if (!_bitblt_src_cf_supported(layer_to_draw->color_format) ||
                !_bitblt_buf_aligned(layer_to_draw->draw_buf->data, layer_to_draw->draw_buf->header.stride))
            goto _bitblt_evaluate_not_ok;

        if (!_bitblt_draw_img_supported(draw_dsc))
            goto _bitblt_evaluate_not_ok;
    }
    break;

    case LV_DRAW_TASK_TYPE_IMAGE:
    {
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *) task->draw_dsc;
        const lv_image_dsc_t *img_dsc = draw_dsc->src;

        int32_t src_stride = (img_dsc->header.stride == 0) ? // Set?
                             lv_draw_buf_width_to_stride(img_dsc->header.w, img_dsc->header.cf) :
                             img_dsc->header.stride;
        int32_t dest_x = blend_area.x1;
        int32_t dest_y = blend_area.y1;
        int32_t dest_w = lv_area_get_width(&blend_area);
        int32_t dest_h = lv_area_get_height(&blend_area);
        int32_t dest_stride = blend_area_stride;
        uint8_t *dest_buf = draw_dsc_base->layer->draw_buf->data;

        LV_LOG_USER("[%d] src_cf:%d -> dst_cf:%d, src_buf:%08x, src_stride:%d",
                    task->type,
                    img_dsc->header.cf,
                    draw_dsc_base->layer->draw_buf->header.cf,
                    img_dsc->data,
                    img_dsc->header.stride);

        /*
        (1) Because reducing color depth caused noise artifacts after the H/W bitblt operation, which affected image quality, it was therefore canceled.
        */
        if ((draw_dsc->tile) ||
                (img_dsc->header.cf != draw_dsc_base->layer->draw_buf->header.cf) ||
                !_bitblt_src_cf_supported(img_dsc->header.cf) ||
                !_bitblt_buf_aligned(img_dsc->data, img_dsc->header.stride) ||
                !_bitblt_buf_aligned(dest_buf + dest_stride * dest_h + dest_x * px_size, dest_stride))
            goto _bitblt_evaluate_not_ok;

        if (!_bitblt_draw_img_supported(draw_dsc))
            goto _bitblt_evaluate_not_ok;
    }
    break;

    default:
        goto _bitblt_evaluate_not_ok;
    }

_bitblt_evaluate_ok:

    if (task->preference_score > 70)
    {
        task->preference_score = 70;
        task->preferred_draw_unit_id = DRAW_UNIT_ID_BITBLT;
    }

    return 1;

_bitblt_evaluate_not_ok:

    return 0;
}

static int32_t _bitblt_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_bitblt_unit_t *draw_bitblt_unit = (lv_draw_bitblt_unit_t *) draw_unit;

    /* Return immediately if it's busy with draw task. */
#if LV_USE_OS
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_bitblt_unit->thread_dsc;

    /* Return immediately if it's busy with draw task. */
    if (thread_dsc->task_act)
        return 0;
#else
    /* Return immediately if it's busy with draw task. */
    if (draw_bitblt_unit->task_act)
        return 0;
#endif

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_BITBLT);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_BITBLT)
        return LV_DRAW_UNIT_IDLE;

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL)
        return LV_DRAW_UNIT_IDLE;

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    t->draw_unit = draw_unit;

#if LV_USE_OS
    thread_dsc->task_act = t;

    /* Let the render thread work. */
    if (thread_dsc->inited)
        lv_thread_sync_signal(&thread_dsc->sync);
#else
    _bitblt_execute_drawing(t);

    draw_bitblt_unit->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
    draw_bitblt_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _bitblt_delete(lv_draw_unit_t *draw_unit)
{
#if LV_USE_OS
    lv_draw_bitblt_unit_t *draw_bitblt_unit = (lv_draw_bitblt_unit_t *) draw_unit;
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_bitblt_unit->thread_dsc;
    LV_LOG_INFO("Cancel BITBLT draw thread.");
    thread_dsc->exit_status = true;

    if (thread_dsc->inited)
        lv_thread_sync_signal(&thread_dsc->sync);

    return lv_thread_delete(&thread_dsc->thread);
#else

    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _bitblt_execute_drawing(lv_draw_task_t *t)
{
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    /* Invalidate only the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, NULL);

    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
        lv_draw_bitblt_fill(t);
        break;
    case LV_DRAW_TASK_TYPE_LAYER:
        lv_draw_bitblt_layer(t);
        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
        lv_draw_bitblt_image(t);
        break;
    default:
        break;
    }
}

#if LV_USE_OS
static void _bitblt_render_thread_cb(void *ptr)
{
    lv_draw_sw_thread_dsc_t *thread_dsc = ptr;
    lv_thread_sync_init(&thread_dsc->sync);
    thread_dsc->inited = true;

    while (1)
    {
        /* Wait for sync if there is no task set. */
        while (thread_dsc->task_act == NULL)
        {
            if (thread_dsc->exit_status)
                break;

            lv_thread_sync_wait(&thread_dsc->sync);
        }

        if (thread_dsc->exit_status)
        {
            LV_LOG_INFO("Ready to exit BITBLT draw thread.");
            break;
        }

        _bitblt_execute_drawing(thread_dsc->task_act);

        /* Signal the ready state to dispatcher. */
        thread_dsc->task_act->state = LV_DRAW_TASK_STATE_FINISHED;

        /* Cleanup. */
        thread_dsc->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    thread_dsc->inited = false;
    lv_thread_sync_delete(&thread_dsc->sync);

    LV_LOG_INFO("Exit BITBLT draw thread.");
}
#endif

static void _bitblt_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    const lv_image_header_t *header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;

    uint8_t *address = draw_buf->data;
    int32_t i = 0;
    uint32_t bytes_per_pixel = lv_color_format_get_size(cf);
    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);
    int32_t bytes_to_flush_per_line = (int32_t)width * (int32_t)bytes_per_pixel;

    /* Stride is in bytes, not pixels */
    address = address + (area->x1 * (int32_t)bytes_per_pixel) + (stride * (uint32_t)area->y1);

    for (i = 0; i < lines; i++)
    {
        sysCleanInvalidatedDcache((UINT32)address, bytes_to_flush_per_line);
        address += stride;
    }
}

#endif /*LV_USE_DRAW_PXP*/
