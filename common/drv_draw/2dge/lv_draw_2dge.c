/**
 * @file lv_draw_2dge.c
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

#include "lv_draw_2dge.h"

#if LV_USE_DRAW_2DGE

#if LV_USE_PARALLEL_DRAW_DEBUG
    #include "../../../core/lv_global.h"
#endif

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_2DGE     254

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*
 * Evaluate a task and set the score and preferred 2DGE unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _2dge_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the 2DGE unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _2dge_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the 2DGE draw unit.
 */
static int32_t _2dge_delete(lv_draw_unit_t *draw_unit);

#if LV_USE_OS
    static void _2dge_render_thread_cb(void *ptr);
#endif

static void _2dge_execute_drawing(lv_draw_2dge_unit_t *u);

static void _2dge_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area);

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

void lv_draw_2dge_init(void)
{
    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _2dge_invalidate_cache;

    lv_draw_2dge_unit_t *draw_2dge_unit = lv_draw_create_unit(sizeof(lv_draw_2dge_unit_t));
    draw_2dge_unit->base_unit.evaluate_cb = _2dge_evaluate;
    draw_2dge_unit->base_unit.dispatch_cb = _2dge_dispatch;
    draw_2dge_unit->base_unit.delete_cb = _2dge_delete;

#if LV_USE_OS
    lv_thread_init(&draw_2dge_unit->thread, LV_THREAD_PRIO_HIGH, _2dge_render_thread_cb, 2 * 1024, draw_2dge_unit);
#endif
}

void lv_draw_2dge_deinit(void)
{
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline bool _2dge_src_cf_supported(lv_color_format_t cf)
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

static inline bool _2dge_dest_cf_supported(lv_color_format_t cf)
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

static int32_t _2dge_evaluate(lv_draw_unit_t *u, lv_draw_task_t *t)
{
    LV_UNUSED(u);

    /* Cooking, in developing. */
    return 0;

    const lv_draw_dsc_base_t *draw_dsc_base = (lv_draw_dsc_base_t *) t->draw_dsc;

    if (!_2dge_dest_cf_supported(draw_dsc_base->layer->color_format))
        return 0;

    if (lv_area_get_size(&draw_dsc_base->layer->buf_area) < 7200u)
        return 0;

    uint8_t px_size = lv_color_format_get_size(draw_dsc_base->layer->color_format);

    if (px_size == 2)   //RGB565
    {
        lv_area_t blend_area;

        lv_area_copy(&blend_area, &draw_dsc_base->layer->buf_area);

        uint32_t blend_area_stride = lv_area_get_width(&blend_area) * px_size;

        /* Check Hardware constraint: The stride must be a word-alignment. */
        bool bAlignedWord = ((blend_area_stride & 0x3) == 0) &&
                            (((blend_area.x1 * px_size) & 0x3) == 0) ? true : false;

        if (!bAlignedWord)
            return 0;
    }

    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
    {
#if 0
        sysprintf("buf_area x1: %d\n", draw_dsc_base->layer->buf_area.x1);
        sysprintf("buf_area x2: %d\n", draw_dsc_base->layer->buf_area.x2);
        sysprintf("buf_area y1: %d\n", draw_dsc_base->layer->buf_area.y1);
        sysprintf("buf_area y2: %d\n", draw_dsc_base->layer->buf_area.y2);
        sysprintf("buf_area size: %d\n", lv_area_get_size(&draw_dsc_base->layer->buf_area));
#endif
        const lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *) t->draw_dsc;

        if ((draw_dsc->radius != 0) ||
                (draw_dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE) ||
                (draw_dsc->opa < LV_OPA_MAX))
            return 0;

        if (t->preference_score > 70)
        {
            t->preference_score = 70;
            t->preferred_draw_unit_id = DRAW_UNIT_ID_2DGE;
        }
        return 1;
    }

    default:
        return 0;
    }

    return 0;
}

static int32_t _2dge_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_2dge_unit_t *draw_2dge_unit = (lv_draw_2dge_unit_t *) draw_unit;

    /* Return immediately if it's busy with draw task. */
    if (draw_2dge_unit->task_act)
        return 0;

    /* Try to get an ready to draw. */
    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_2DGE);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_2DGE)
        return -1;

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL)
        return -1;

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_2dge_unit->base_unit.target_layer = layer;
    draw_2dge_unit->base_unit.clip_area = &t->clip_area;
    draw_2dge_unit->task_act = t;

#if LV_USE_OS
    /* Let the render thread work. */
    if (draw_2dge_unit->inited)
        lv_thread_sync_signal(&draw_2dge_unit->sync);
#else
    _2dge_execute_drawing(draw_2dge_unit);

    draw_2dge_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
    draw_2dge_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _2dge_delete(lv_draw_unit_t *draw_unit)
{
#if LV_USE_OS
    lv_draw_2dge_unit_t *draw_2dge_unit = (lv_draw_2dge_unit_t *) draw_unit;

    LV_LOG_INFO("Cancel 2DGE draw thread.");
    draw_2dge_unit->exit_status = true;

    if (draw_2dge_unit->inited)
        lv_thread_sync_signal(&draw_2dge_unit->sync);

    lv_result_t res = lv_thread_delete(&draw_2dge_unit->thread);

    return res;
#else
    LV_UNUSED(draw_unit);

    return 0;
#endif
}

static void _2dge_execute_drawing(lv_draw_2dge_unit_t *u)
{
    lv_draw_task_t *t = u->task_act;
    lv_draw_unit_t *draw_unit = (lv_draw_unit_t *)u;
    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

#if 0
    lv_area_t draw_area;
    if (!_lv_area_intersect(&draw_area, &t->area, draw_unit->clip_area))
        return; /*Fully clipped, nothing to do*/

    /* Make area relative to the buffer */
    lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);

    /* Invalidate only the drawing area */
    lv_draw_buf_invalidate_cache(draw_buf, &draw_area);
#else
    lv_area_t draw_area;
    int32_t x;
    int32_t y;

    _lv_area_intersect(&draw_area,  &t->area, u->base_unit.clip_area);

    x = 0 - u->base_unit.target_layer->buf_area.x1;
    y = 0 - u->base_unit.target_layer->buf_area.y1;

    lv_area_move(&draw_area, x, y);

    /* Invalidate cache */
    lv_draw_buf_invalidate_cache(layer->draw_buf, &draw_area);

#endif
    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
        lv_draw_2dge_fill(draw_unit, t->draw_dsc, &t->area);
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
static void _2dge_render_thread_cb(void *ptr)
{
    lv_draw_2dge_unit_t *u = ptr;

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
            LV_LOG_INFO("Ready to exit 2DGE draw thread.");
            break;
        }

        _2dge_execute_drawing(u);

        /* Signal the ready state to dispatcher. */
        u->task_act->state = LV_DRAW_TASK_STATE_READY;

        /* Cleanup. */
        u->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new task. */
        lv_draw_dispatch_request();
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
    LV_LOG_INFO("Exit 2DGE draw thread.");
}
#endif

static void _2dge_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    const lv_image_header_t *header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;

    if (area->y1 == 0)
    {
        uint16_t size = stride * lv_area_get_height(area);

        /* Invalidate full buffer. */
        sysCleanInvalidatedDcache((UINT32)draw_buf->data, size);
        return;
    }

    const uint8_t *buf_u8 = draw_buf->data;
    /* ARM require a 32 byte aligned address. */
    uint8_t align_bytes = 32;
    uint8_t bits_per_pixel = lv_color_format_get_bpp(cf);

    uint16_t align_pixels = align_bytes * 8 / bits_per_pixel;
    uint16_t offset_x = 0;

    if (area->x1 >= (int32_t)(area->x1 % align_pixels))
    {
        uint16_t shift_x = area->x1 - (area->x1 % align_pixels);

        offset_x = area->x1 - shift_x;
        buf_u8 += (shift_x * bits_per_pixel) / 8;
    }

    if (area->y1)
    {
        uint16_t shift_y = area->y1;

        buf_u8 += shift_y * stride;
    }

    /* Area to clear can start from a different offset in buffer.
     * Invalidate the area line by line.
     */
    uint16_t line_pixels = offset_x + lv_area_get_width(area);
    uint16_t line_size = (line_pixels * bits_per_pixel) / 8;
    uint16_t area_height = lv_area_get_height(area);

    for (uint16_t y = 0; y < area_height; y++)
    {
        const void *line_addr = buf_u8 + y * stride;

        sysCleanInvalidatedDcache((UINT32)line_addr, line_size);
    }
}

#endif /*LV_USE_DRAW_PXP*/
