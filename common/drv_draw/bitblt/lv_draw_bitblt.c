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

#if LV_USE_PARALLEL_DRAW_DEBUG
    #include "../../core/lv_global.h"
#endif

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

static void _bitblt_execute_drawing(lv_draw_bitblt_unit_t *u);

static void _bitblt_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

#if LV_USE_PARALLEL_DRAW_DEBUG
    #define _draw_info LV_GLOBAL_DEFAULT()->draw_info
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_bitblt_init(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _bitblt_invalidate_cache;

    lv_draw_bitblt_unit_t *draw_bitblt_unit = lv_draw_create_unit(sizeof(lv_draw_bitblt_unit_t));
    draw_bitblt_unit->base_unit.evaluate_cb = _bitblt_evaluate;
    draw_bitblt_unit->base_unit.dispatch_cb = _bitblt_dispatch;
    draw_bitblt_unit->base_unit.delete_cb = _bitblt_delete;

#if LV_USE_OS
    lv_thread_init(&draw_bitblt_unit->thread, LV_THREAD_PRIO_HIGH, _bitblt_render_thread_cb, 2 * 1024, draw_bitblt_unit);
#endif
	
    bltOpen();
}

void lv_draw_bitblt_deinit(void)
{
    bltClose();
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

static int32_t _bitblt_evaluate(lv_draw_unit_t *u, lv_draw_task_t *t)
{
    LV_UNUSED(u);

    const lv_draw_dsc_base_t *draw_dsc_base = (lv_draw_dsc_base_t *) t->draw_dsc;

    uint8_t px_size = lv_color_format_get_size(draw_dsc_base->layer->color_format);

    /* Check capacity. */
    if (!_bitblt_dest_cf_supported(draw_dsc_base->layer->color_format))
        return 0;

    if (lv_area_get_size(&draw_dsc_base->layer->buf_area) < 7200u)
        return 0;


    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
    {
        const lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *) t->draw_dsc;

        if ((draw_dsc->radius != 0) ||
                (draw_dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE) ||
                (draw_dsc->opa < LV_OPA_MAX))
            return 0;

#if 0
        sysprintf("buf_area x1: %d\n", draw_dsc_base->layer->buf_area.x1);
        sysprintf("buf_area x2: %d\n", draw_dsc_base->layer->buf_area.x2);
        sysprintf("buf_area y1: %d\n", draw_dsc_base->layer->buf_area.y1);
        sysprintf("buf_area y2: %d\n", draw_dsc_base->layer->buf_area.y2);
        sysprintf("buf_area size: %d\n", lv_area_get_size(&draw_dsc_base->layer->buf_area));
#endif

        if (t->preference_score > 70)
        {
            t->preference_score = 70;
            t->preferred_draw_unit_id = DRAW_UNIT_ID_BITBLT;
        }
        return 1;
    }

    default:
        return 0;
    }

    return 0;
}

static int32_t _bitblt_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_bitblt_unit_t *draw_bitblt_unit = (lv_draw_bitblt_unit_t *) draw_unit;

    /* Return immediately if it's busy with draw task. */
    if (draw_bitblt_unit->task_act)
        return 0;

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_BITBLT);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_BITBLT)
        return -1;

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL)
        return -1;

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_bitblt_unit->base_unit.target_layer = layer;
    draw_bitblt_unit->base_unit.clip_area = &t->clip_area;
    draw_bitblt_unit->task_act = t;

#if LV_USE_OS
    /* Let the render thread work. */
    if (draw_bitblt_unit->inited)
        lv_thread_sync_signal(&draw_bitblt_unit->sync);
#else
    _bitblt_execute_drawing(draw_bitblt_unit);

    draw_bitblt_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
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

    LV_LOG_INFO("Cancel BITBLT draw thread.");
    draw_bitblt_unit->exit_status = true;

    if (draw_bitblt_unit->inited)
        lv_thread_sync_signal(&draw_bitblt_unit->sync);

    lv_result_t res = lv_thread_delete(&draw_bitblt_unit->thread);

    return res;
#else
    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _bitblt_execute_drawing(lv_draw_bitblt_unit_t *u)
{
    lv_draw_task_t *t = u->task_act;
    lv_draw_unit_t *draw_unit = (lv_draw_unit_t *)u;
    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t draw_area;
    if (!_lv_area_intersect(&draw_area, &t->area, draw_unit->clip_area))
        return; /*Fully clipped, nothing to do*/

    /* Make area relative to the buffer */
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

    /* Invalidate only the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, &draw_area);

    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
        lv_draw_bitblt_fill(draw_unit, t->draw_dsc, &t->area);
        break;
    default:
        break;
    }

#if LV_USE_PARALLEL_DRAW_DEBUG
    /*Layers manage it for themselves*/
    if (t->type != LV_DRAW_TASK_TYPE_LAYER)
    {
        lv_area_t draw_area;
        if (!_lv_area_intersect(&draw_area, &t->area, u->base_unit.clip_area))
            return;

        int32_t idx = 0;
        lv_draw_unit_t *draw_unit_tmp = _draw_info.unit_head;
        while (draw_unit_tmp != (lv_draw_unit_t *)u)
        {
            draw_unit_tmp = draw_unit_tmp->next;
            idx++;
        }
        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = lv_palette_main(idx % _LV_PALETTE_LAST);
        rect_dsc.border_color = rect_dsc.bg_color;
        rect_dsc.bg_opa = LV_OPA_10;
        rect_dsc.border_opa = LV_OPA_80;
        rect_dsc.border_width = 1;
        lv_draw_sw_fill((lv_draw_unit_t *)u, &rect_dsc, &draw_area);

        lv_point_t txt_size;
        lv_text_get_size(&txt_size, "W", LV_FONT_DEFAULT, 0, 0, 100, LV_TEXT_FLAG_NONE);

        lv_area_t txt_area;
        txt_area.x1 = draw_area.x1;
        txt_area.y1 = draw_area.y1;
        txt_area.x2 = draw_area.x1 + txt_size.x - 1;
        txt_area.y2 = draw_area.y1 + txt_size.y - 1;

        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = lv_color_white();
        lv_draw_sw_fill((lv_draw_unit_t *)u, &rect_dsc, &txt_area);

        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", idx);
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_black();
        label_dsc.text = buf;
        lv_draw_sw_label((lv_draw_unit_t *)u, &label_dsc, &txt_area);
    }
#endif
}

#if LV_USE_OS
static void _bitblt_render_thread_cb(void *ptr)
{
    lv_draw_bitblt_unit_t *u = ptr;

    lv_thread_sync_init(&u->sync);
    u->inited = true;

    while (1)
    {
        /* Wait for sync if there is no task set. */
        while (u->task_act == NULL)
        {
            if (u->exit_status)
                break;

            lv_thread_sync_wait(&u->sync);
        }

        if (u->exit_status)
        {
            LV_LOG_INFO("Ready to exit BITBLT draw thread.");
            break;
        }

        _bitblt_execute_drawing(u);

        /* Signal the ready state to dispatcher. */
        u->task_act->state = LV_DRAW_TASK_STATE_READY;

        /* Cleanup. */
        u->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
    LV_LOG_INFO("Exit BITBLT draw thread.");
}
#endif

static void _bitblt_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    const lv_image_header_t * header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;

    uint8_t * address = draw_buf->data;
    int32_t i = 0;
    uint32_t bytes_per_pixel = lv_color_format_get_size(cf);
    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);
    int32_t bytes_to_flush_per_line = (int32_t)width * (int32_t)bytes_per_pixel;

    /* Stride is in bytes, not pixels */
    address = address + (area->x1 * (int32_t)bytes_per_pixel) + (stride * (uint32_t)area->y1);

    for(i = 0; i < lines; i++) {
        sysCleanInvalidatedDcache((UINT32)address, bytes_to_flush_per_line);
        address += stride;
    }
}

#endif /*LV_USE_DRAW_PXP*/
