/**
 * @file lv_draw_gdma.c
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

#include "lv_draw_gdma.h"

#if LV_USE_DRAW_GDMA
#include "../lv_draw_buf_private.h"

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_GDMA     254

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*
 * Evaluate a task and set the score and preferred GDMA unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _gdma_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the GDMA unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _gdma_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the GDMA draw unit.
 */
static int32_t _gdma_delete(lv_draw_unit_t *draw_unit);

#if LV_USE_OS
    static void _gdma_render_thread_cb(void *ptr);
#endif

static void _gdma_execute_drawing(lv_draw_task_t *t);

static void _gdma_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

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

void lv_draw_gdma_init(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _gdma_invalidate_cache;

    lv_draw_gdma_unit_t *draw_gdma_unit = lv_draw_create_unit(sizeof(lv_draw_gdma_unit_t));
    draw_gdma_unit->base_unit.evaluate_cb = _gdma_evaluate;
    draw_gdma_unit->base_unit.dispatch_cb = _gdma_dispatch;
    draw_gdma_unit->base_unit.delete_cb = _gdma_delete;
    draw_gdma_unit->base_unit.name = "GDMA";

#if LV_USE_OS
    void gdmaInterruptInit(void);
    gdmaInterruptInit();
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gdma_unit->thread_dsc;

    thread_dsc->idx = 0;
    thread_dsc->draw_unit = (void *) draw_gdma_unit;
    lv_thread_init(&thread_dsc->thread, "gdma_draw", LV_DRAW_THREAD_PRIO, _gdma_render_thread_cb, LV_DRAW_THREAD_STACK_SIZE, thread_dsc);
#endif
}

void lv_draw_gdma_deinit(void)
{
#if LV_USE_OS
    void gdmaInterruptDeinit(void);
    gdmaInterruptDeinit();
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline bool _gdma_src_cf_supported(lv_color_format_t cf)
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

static inline bool _gdma_dest_cf_supported(lv_color_format_t cf)
{
    bool is_cf_supported = false;

    switch (cf)
    {
    case LV_COLOR_FORMAT_RGB565:
    case LV_COLOR_FORMAT_RGB888:
    case LV_COLOR_FORMAT_ARGB8888:
    case LV_COLOR_FORMAT_XRGB8888:
        is_cf_supported = true;
        break;
    default:
        break;
    }

    return is_cf_supported;
}

static bool _gdma_draw_img_supported(const lv_draw_image_dsc_t *draw_dsc)
{
    const lv_image_dsc_t *img_dsc = draw_dsc->src;
    bool is_tiled = draw_dsc->tile;

    bool has_recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);
    bool has_transform = (draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE ||
                          draw_dsc->scale_y != LV_SCALE_NONE);
    bool has_opa = (draw_dsc->opa < (lv_opa_t)LV_OPA_MAX);
    bool src_has_alpha = (img_dsc->header.cf == LV_COLOR_FORMAT_ARGB8888);

    /* Recolor and transformation are not supported at the same time. */
    if (is_tiled || has_recolor || has_transform || draw_dsc->rotation)
        return false;

    return true;
}

static bool _gdma_buf_aligned(const void *buf, uint32_t stride)
{
    /* Test for pointer alignment */
    if ((uintptr_t)buf % 4)
        return false;

    /* Test for invalid stride (no stride alignment required) */
    if (stride % 4)
        return false;

    return true;
}

static int32_t _gdma_evaluate(lv_draw_unit_t *u, lv_draw_task_t *task)
{
    LV_UNUSED(u);

    const lv_draw_dsc_base_t *draw_dsc_base = (lv_draw_dsc_base_t *) task->draw_dsc;

    uint8_t px_size = lv_color_format_get_size(draw_dsc_base->layer->color_format);

    lv_area_t blend_area;

    /* Check capacity. */
    if (!_gdma_dest_cf_supported(draw_dsc_base->layer->color_format))
        goto _gdma_evaluate_not_ok;

    lv_area_copy(&blend_area, &draw_dsc_base->layer->buf_area);

    switch (task->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
    {
        const lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *) task->draw_dsc;

        if (!((draw_dsc->radius == 0) && (draw_dsc->grad.dir == LV_GRAD_DIR_NONE) && (draw_dsc->opa >= LV_OPA_MAX)))
            goto _gdma_evaluate_not_ok;
    }
    break;

    case LV_DRAW_TASK_TYPE_IMAGE:
    {
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *) task->draw_dsc;
        const lv_image_dsc_t *img_dsc = draw_dsc->src;

        if (!_gdma_src_cf_supported(img_dsc->header.cf) ||
                !_gdma_buf_aligned(img_dsc->data, img_dsc->header.stride) ||
                (img_dsc->header.cf != draw_dsc_base->layer->color_format))
            goto _gdma_evaluate_not_ok;

        if (!_gdma_draw_img_supported(draw_dsc))
            goto _gdma_evaluate_not_ok;
    }
    break;

    default:
        goto _gdma_evaluate_not_ok;
    }

_gdma_evaluate_ok:

    if (task->preference_score > 70)
    {
        task->preference_score = 70;
        task->preferred_draw_unit_id = DRAW_UNIT_ID_GDMA;
    }

    return 1;

_gdma_evaluate_not_ok:

    return 0;
}

static int32_t _gdma_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_gdma_unit_t *draw_gdma_unit = (lv_draw_gdma_unit_t *) draw_unit;

    /* Return immediately if it's busy with draw task. */
#if LV_USE_OS
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gdma_unit->thread_dsc;

    /* Return immediately if it's busy with draw task. */
    if (thread_dsc->task_act)
        return 0;
#else
    /* Return immediately if it's busy with draw task. */
    if (draw_gdma_unit->task_act)
        return 0;
#endif

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_GDMA);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_GDMA)
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
    _gdma_execute_drawing(t);

    draw_gdma_unit->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
    draw_gdma_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _gdma_delete(lv_draw_unit_t *draw_unit)
{
#if LV_USE_OS
    lv_draw_gdma_unit_t *draw_gdma_unit = (lv_draw_gdma_unit_t *) draw_unit;
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gdma_unit->thread_dsc;
    LV_LOG_INFO("Cancel GDMA draw thread.");
    thread_dsc->exit_status = true;

    if (thread_dsc->inited)
        lv_thread_sync_signal(&thread_dsc->sync);

    return lv_thread_delete(&thread_dsc->thread);
#else

    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _gdma_execute_drawing(lv_draw_task_t *t)
{
    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    /* Invalidate only the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, NULL);

    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
        lv_draw_gdma_fill(t);
        break;
    case LV_DRAW_TASK_TYPE_LAYER:
        lv_draw_gdma_layer(t);
        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
        lv_draw_gdma_image(t);
        break;
    default:
        break;
    }
}

#if LV_USE_OS
static void _gdma_render_thread_cb(void *ptr)
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
            LV_LOG_INFO("Ready to exit GDMA draw thread.");
            break;
        }

        _gdma_execute_drawing(thread_dsc->task_act);

        /* Signal the ready state to dispatcher. */
        thread_dsc->task_act->state = LV_DRAW_TASK_STATE_FINISHED;

        /* Cleanup. */
        thread_dsc->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    thread_dsc->inited = false;
    lv_thread_sync_delete(&thread_dsc->sync);

    LV_LOG_INFO("Exit GDMA draw thread.");
}
#endif

static void _gdma_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
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
        SCB_CleanInvalidateDCache_by_Addr(address, bytes_to_flush_per_line);
        address += stride;
    }
}

#endif /*LV_USE_DRAW_PXP*/
