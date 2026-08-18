/**
 * @file lv_draw_gfx.c
 *
 */

/**
 * Copyright 2026 Nuvoton
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_gfx.h"

#if LV_USE_DRAW_GFX
#include "../lv_draw_buf_private.h"
#include "cache.h"

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_GFX 254

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*
 * Evaluate a task and set the score and preferred GFX unit.
 * Return 1 if task is preferred, 0 otherwise (task is not supported).
 */
static int32_t _gfx_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

/*
 * Dispatch a task to the GFX unit.
 * Return 1 if task was dispatched, 0 otherwise (task not supported).
 */
static int32_t _gfx_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

/*
 * Delete the GFX draw unit.
 */
static int32_t _gfx_delete(lv_draw_unit_t *draw_unit);

#if LV_USE_OS
    static void _gfx_render_thread_cb(void *ptr);
#endif

static void _gfx_execute_drawing(lv_draw_task_t *t);

static void _gfx_invalidate_cache(const lv_draw_buf_t *draw_buf,
                                  const lv_area_t *area);

static void _gfx_buf_copy_cb(lv_draw_buf_t *dest, const lv_area_t *dest_area,
                             const lv_draw_buf_t *src, const lv_area_t *src_area);

/**********************
 *  STATIC VARIABLES
 **********************/

void *g_gfx_handle = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_gfx_init(void)
{
    /* Open dedicated GFX device handle for draw/rendering operations */
    int ret = gfx_open(&g_gfx_handle);
    if (ret != 0)
    {
        sysprintf("[GFX_RENDER] gfx_open failed: %d\n", ret);
    }
    else
    {
        sysprintf("[GFX_RENDER] GFX render handle initialized: 0x%p (libgfx)!\n", g_gfx_handle);
    }

    lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

    handlers->invalidate_cache_cb = _gfx_invalidate_cache;
    handlers->buf_copy_cb = _gfx_buf_copy_cb;

    lv_draw_gfx_unit_t *draw_gfx_unit =
        lv_draw_create_unit(sizeof(lv_draw_gfx_unit_t));
    draw_gfx_unit->base_unit.evaluate_cb = _gfx_evaluate;
    draw_gfx_unit->base_unit.dispatch_cb = _gfx_dispatch;
    draw_gfx_unit->base_unit.delete_cb = _gfx_delete;

#if LV_USE_OS
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gfx_unit->thread_dsc;
    thread_dsc->idx = 0;
    thread_dsc->draw_unit = (void *)draw_gfx_unit;
    lv_thread_init(&thread_dsc->thread, "gfx_draw", LV_DRAW_THREAD_PRIO,
                   _gfx_render_thread_cb, LV_DRAW_THREAD_STACK_SIZE, thread_dsc);
#endif
}

void lv_draw_gfx_deinit(void)
{
    if (g_gfx_handle)
    {
        gfx_close(g_gfx_handle);
        g_gfx_handle = NULL;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline bool _gfx_src_cf_supported(lv_color_format_t cf)
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

static inline bool _gfx_dest_cf_supported(lv_color_format_t cf)
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

static inline bool _gfx_is_cf_blend_compatible(lv_color_format_t src_cf,
        lv_color_format_t dest_cf)
{
    if (src_cf == dest_cf)
        return true;

    if ((src_cf == LV_COLOR_FORMAT_ARGB8888 ||
            src_cf == LV_COLOR_FORMAT_XRGB8888) &&
            (dest_cf == LV_COLOR_FORMAT_ARGB8888 ||
             dest_cf == LV_COLOR_FORMAT_XRGB8888))
        return true;

    return false;
}

static bool _gfx_draw_img_supported(const lv_draw_image_dsc_t *draw_dsc)
{
    bool has_recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);
    bool has_rotation = (draw_dsc->rotation != 0);

    if (has_recolor || has_rotation || draw_dsc->tile)
        return false;

    return true;
}

static bool _gfx_buf_aligned(const void *buf, uint32_t stride)
{
    /* Test for pointer alignment (at least 4-byte) */
    if ((uintptr_t)buf % 4)
        return false;

    /* GC520L surface stride must be 16-byte aligned */
    if ((stride == 0) || (stride & 15u))
        return false;

    return true;
}

static int32_t _gfx_evaluate(lv_draw_unit_t *u, lv_draw_task_t *task)
{
    LV_UNUSED(u);

    const lv_draw_dsc_base_t *draw_dsc_base =
        (lv_draw_dsc_base_t *)task->draw_dsc;

    uint8_t px_size =
        lv_color_format_get_size(draw_dsc_base->layer->color_format);

    lv_area_t blend_area;
    uint32_t blend_area_stride;

    /* Check capacity. */
    if (!_gfx_dest_cf_supported(draw_dsc_base->layer->color_format))
        goto _gfx_evaluate_not_ok;

    lv_area_copy(&blend_area, &draw_dsc_base->layer->buf_area);
    blend_area_stride = lv_area_get_width(&blend_area) * px_size;

    /* Check GC520L stride constraint: 16-byte alignment. */
    if (blend_area_stride & 15u)
        goto _gfx_evaluate_not_ok;

    switch (task->type)
    {
#if 1
    case LV_DRAW_TASK_TYPE_FILL:
    {
        const lv_draw_fill_dsc_t *draw_dsc = (lv_draw_fill_dsc_t *)task->draw_dsc;

        if (!((draw_dsc->radius == 0) && (draw_dsc->grad.dir == LV_GRAD_DIR_NONE) &&
                (draw_dsc->opa >= LV_OPA_MAX) &&
                lv_area_is_in(&task->area, &task->clip_area, false)))
            goto _gfx_evaluate_not_ok;

        int32_t fill_w = lv_area_get_width(&task->area);
        int32_t fill_h = lv_area_get_height(&task->area);
        int32_t rel_x1 = task->area.x1 - draw_dsc_base->layer->buf_area.x1;

        /*
         * GC520L hardware fill requirements:
         * 1. 16-byte alignment on X coordinate and width (4 pixels for 32bpp, 8 for 16bpp).
         * 2. Size threshold: Only accelerate large fills (>= 64x64). Small widgets/boxes
         *    are much faster on CPU (lv_draw_sw_fill in L1 cache) and avoid GPU/cache overhead.
         */
        if (fill_w < 64 || fill_h < 64 ||
            ((rel_x1 * px_size) & 15u) ||
            (((uint32_t)fill_w * px_size) & 15u))
            goto _gfx_evaluate_not_ok;
    }
    break;
#endif
    case LV_DRAW_TASK_TYPE_LAYER:
    {
        const lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *) task->draw_dsc;
        lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;

        if (layer_to_draw == NULL || layer_to_draw->draw_buf == NULL)
            goto _gfx_evaluate_not_ok;

        bool has_scale = (draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE);
        if (!_gfx_src_cf_supported(layer_to_draw->color_format) ||
                !_gfx_buf_aligned(layer_to_draw->draw_buf->data, layer_to_draw->draw_buf->header.stride) ||
                !_gfx_is_cf_blend_compatible(layer_to_draw->color_format, draw_dsc_base->layer->color_format) ||
                (layer_to_draw->color_format == LV_COLOR_FORMAT_ARGB8888 && (draw_dsc->opa < LV_OPA_MAX || has_scale)) ||
                !lv_area_is_in(&task->_real_area, &task->clip_area, false))
            goto _gfx_evaluate_not_ok;

        if (!_gfx_draw_img_supported(draw_dsc))
            goto _gfx_evaluate_not_ok;
    }
    break;
    case LV_DRAW_TASK_TYPE_IMAGE:
    {
        lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *) task->draw_dsc;
        const lv_image_dsc_t *img_dsc = draw_dsc->src;

        if (img_dsc == NULL || img_dsc->data == NULL)
            goto _gfx_evaluate_not_ok;

        int32_t src_stride = (img_dsc->header.stride == 0) ?
                             lv_draw_buf_width_to_stride(img_dsc->header.w, img_dsc->header.cf) :
                             img_dsc->header.stride;

        bool has_scale = (draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE);
        if (draw_dsc->tile ||
                !_gfx_src_cf_supported(img_dsc->header.cf) ||
                !_gfx_buf_aligned(img_dsc->data, src_stride) ||
                !_gfx_is_cf_blend_compatible(img_dsc->header.cf, draw_dsc_base->layer->color_format) ||
                (img_dsc->header.cf == LV_COLOR_FORMAT_ARGB8888 && (draw_dsc->opa < LV_OPA_MAX || has_scale)) ||
                !lv_area_is_in(&task->_real_area, &task->clip_area, false))
            goto _gfx_evaluate_not_ok;

        if (!_gfx_draw_img_supported(draw_dsc))
            goto _gfx_evaluate_not_ok;
    }
    break;

    default:
        goto _gfx_evaluate_not_ok;
    }

_gfx_evaluate_ok:

    if (task->preference_score > 70)
    {
        task->preference_score = 70;
        task->preferred_draw_unit_id = DRAW_UNIT_ID_GFX;
    }

    return 1;

_gfx_evaluate_not_ok:

    return 0;
}

static int32_t _gfx_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_draw_gfx_unit_t *draw_gfx_unit = (lv_draw_gfx_unit_t *)draw_unit;

#if LV_USE_OS
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gfx_unit->thread_dsc;

    /* Return immediately if it's busy with draw task. */
    if (thread_dsc->task_act)
        return 0;
#else
    /* Return immediately if it's busy with draw task. */
    if (draw_gfx_unit->task_act)
        return 0;
#endif

    /* Try to get a task ready to draw. */
    lv_draw_task_t *t =
        lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_GFX);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_GFX)
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
    _gfx_execute_drawing(t);

    draw_gfx_unit->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
    draw_gfx_unit->task_act = NULL;

    /* The draw unit is free now. Request a new dispatching as it can get a new
     * task. */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _gfx_delete(lv_draw_unit_t *draw_unit)
{
#if LV_USE_OS
    lv_draw_gfx_unit_t *draw_gfx_unit = (lv_draw_gfx_unit_t *)draw_unit;
    lv_draw_sw_thread_dsc_t *thread_dsc = &draw_gfx_unit->thread_dsc;
    LV_LOG_INFO("Cancel GFX draw thread.");
    thread_dsc->exit_status = true;

    if (thread_dsc->inited)
        lv_thread_sync_signal(&thread_dsc->sync);

    return lv_thread_delete(&thread_dsc->thread);
#else
    LV_UNUSED(draw_unit);
    return 0;
#endif
}

static void _gfx_execute_drawing(lv_draw_task_t *t)
{
    switch (t->type)
    {
    case LV_DRAW_TASK_TYPE_FILL:
        lv_draw_gfx_fill(t);
        break;
    case LV_DRAW_TASK_TYPE_LAYER:
        lv_draw_gfx_layer(t);
        break;
    case LV_DRAW_TASK_TYPE_IMAGE:
        lv_draw_gfx_image(t);
        break;
    default:
        break;
    }
}

#if LV_USE_OS
static void _gfx_render_thread_cb(void *ptr)
{
    lv_draw_sw_thread_dsc_t *thread_dsc = ptr;
    lv_thread_sync_init(&thread_dsc->sync);
    thread_dsc->inited = true;

    /* Ensure dedicated render context handle is opened */
    if (g_gfx_handle == NULL)
    {
        int ret = gfx_open(&g_gfx_handle);
        if (ret != 0)
        {
            sysprintf("[GFX_RENDER] gfx_open in render thread failed: %d\n", ret);
        }
        else
        {
            sysprintf("[GFX_RENDER] GFX render handle initialized in thread: 0x%p\n", g_gfx_handle);
        }
    }

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
            LV_LOG_INFO("Ready to exit GFX draw thread.");
            break;
        }

        _gfx_execute_drawing(thread_dsc->task_act);

        /* Signal the ready state to dispatcher. */
        thread_dsc->task_act->state = LV_DRAW_TASK_STATE_FINISHED;

        /* Cleanup. */
        thread_dsc->task_act = NULL;

        /* The draw unit is free now. Request a new dispatching as it can get a new
         * task. */
        lv_draw_dispatch_request();
    }

    thread_dsc->inited = false;
    lv_thread_sync_delete(&thread_dsc->sync);

    LV_LOG_INFO("Exit GFX draw thread.");
}
#endif

static void _gfx_clean_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
    const lv_image_header_t *header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;

    uint8_t *address = draw_buf->data;
    uint32_t bytes_per_pixel = lv_color_format_get_size(cf);

    if (area == NULL)
    {
        dcache_clean_by_mva(address, header->h * stride);
        return;
    }

    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);
    int32_t bytes_to_flush_per_line = width * bytes_per_pixel;

    address = address + (area->x1 * bytes_per_pixel) + (stride * area->y1);

    for (int32_t i = 0; i < lines; i++)
    {
        dcache_clean_by_mva(address, bytes_to_flush_per_line);
        address += stride;
    }
}

static void _gfx_invalidate_cache(const lv_draw_buf_t *draw_buf,
                                  const lv_area_t *area)
{
    const lv_image_header_t *header = &draw_buf->header;
    uint32_t stride = header->stride;
    lv_color_format_t cf = header->cf;

    uint8_t *address = draw_buf->data;
    uint32_t bytes_per_pixel = lv_color_format_get_size(cf);

    if (area == NULL)
    {
        dcache_clean_invalidate_by_mva(address, header->h * stride);
        return;
    }

    int32_t width = lv_area_get_width(area);
    int32_t lines = lv_area_get_height(area);
    int32_t bytes_to_flush_per_line = width * bytes_per_pixel;

    /* Stride is in bytes */
    address = address + (area->x1 * bytes_per_pixel) + (stride * area->y1);

    for (int32_t i = 0; i < lines; i++)
    {
        dcache_clean_invalidate_by_mva(address, bytes_to_flush_per_line);
        address += stride;
    }
}

/**
 * @brief  Cache-aware buffer copy callback for direct-mode sync.
 * @note   In direct-refresh multi-buffered mode, LVGL's refr_sync_areas() copies
 *         previously-rendered dirty areas between framebuffers using this callback.
 *         - Small blocks (<= 512x512) and sub-rectangles: Use fast CPU copy (lv_memcpy)
 *           and clean the destination D-Cache line so updated pixels are immediately
 *           flushed to DDR for the LCD display controller.
 *         - Large blocks (> 512x512) with 16-byte alignment: Handled via 2D GFX Hardware
 *           Operation (gfx_blt) with GFX_XRGB8888 (bypassing GC520L silicon alpha-drop defect).
 */
static void _gfx_buf_copy_cb(lv_draw_buf_t *dest, const lv_area_t *dest_area,
                             const lv_draw_buf_t *src, const lv_area_t *src_area)
{
    LV_ASSERT_FORMAT_MSG(dest->header.cf == src->header.cf,
                         "Color format mismatch: %d != %d",
                         dest->header.cf, src->header.cf);

    int32_t width = dest_area ? lv_area_get_width(dest_area) : (int32_t)dest->header.w;
    int32_t height = dest_area ? lv_area_get_height(dest_area) : (int32_t)dest->header.h;
    if (width <= 0 || height <= 0)
        return;

    uint32_t bpp = lv_color_format_get_size(dest->header.cf);
    uint32_t line_bytes = (uint32_t)width * bpp;
    int32_t dest_x = dest_area ? dest_area->x1 : 0;
    int32_t src_x = src_area ? src_area->x1 : 0;

    bool is_large = ((uint32_t)width * height > 512 * 512);
    bool is_aligned = (((dest_x * bpp) & 15u) == 0) &&
                      (((src_x * bpp) & 15u) == 0) &&
                      ((line_bytes & 15u) == 0) &&
                      ((src->header.stride & 15u) == 0) &&
                      ((dest->header.stride & 15u) == 0);

    /* 1. Large aligned block: Accelerate via 2D GFX Hardware Operation (gfx_blt) */
    if (is_large && is_aligned && g_gfx_handle != NULL)
    {
        gfx_surface_t src_surface;
        gfx_surface_t dst_surface;
        lv_memzero(&src_surface, sizeof(src_surface));
        lv_memzero(&dst_surface, sizeof(dst_surface));

        if (dest->header.cf == LV_COLOR_FORMAT_RGB565)
        {
            src_surface.format = GFX_RGB565;
            dst_surface.format = GFX_RGB565;
        }
        else
        {
            /* Use GFX_XRGB8888 for 32bpp to bypass GC520L silicon alpha-masking defect */
            src_surface.format = GFX_XRGB8888;
            dst_surface.format = GFX_XRGB8888;
        }

        src_surface.width = src->header.w;
        src_surface.height = src->header.h;
        src_surface.stride = src->header.stride;
        src_surface.planes[0] = (int)((uintptr_t)src->data & 0xFFFFFFFFU);

        dst_surface.width = dest->header.w;
        dst_surface.height = dest->header.h;
        dst_surface.stride = dest->header.stride;
        dst_surface.planes[0] = (int)((uintptr_t)dest->data & 0xFFFFFFFFU);

        if (src_area)
        {
            src_surface.rect.tl.x = src_area->x1;
            src_surface.rect.tl.y = src_area->y1;
            src_surface.rect.br.x = src_area->x2 + 1;
            src_surface.rect.br.y = src_area->y2 + 1;
        }
        else
        {
            src_surface.rect.tl.x = 0;
            src_surface.rect.tl.y = 0;
            src_surface.rect.br.x = src->header.w;
            src_surface.rect.br.y = src->header.h;
        }

        if (dest_area)
        {
            dst_surface.rect.tl.x = dest_area->x1;
            dst_surface.rect.tl.y = dest_area->y1;
            dst_surface.rect.br.x = dest_area->x2 + 1;
            dst_surface.rect.br.y = dest_area->y2 + 1;
        }
        else
        {
            dst_surface.rect.tl.x = 0;
            dst_surface.rect.tl.y = 0;
            dst_surface.rect.br.x = dest->header.w;
            dst_surface.rect.br.y = dest->header.h;
        }

        /* Flush CPU dirty lines from src to DDR so GPU reads updated data */
        _gfx_clean_cache(src, src_area);

        /* Lock GPU and execute 2D BitBlt */
        gfx_osal_lock(GFX_OSAL_WAIT_FOREVER);
        gfx_disable(g_gfx_handle, GFX_BLEND);
        gfx_disable(g_gfx_handle, GFX_SRC_COLORKEY);
        gfx_disable(g_gfx_handle, GFX_DST_COLORKEY);
        gfx_blt(g_gfx_handle, &src_surface, &dst_surface);
        gfx_finish(g_gfx_handle);
        gfx_osal_unlock();

        /* Invalidate dest in CPU D-Cache so future reads fetch fresh GPU pixels from DDR */
        _gfx_invalidate_cache(dest, dest_area);
        return;
    }

    /* 2. Small (<= 512x512) or unaligned block: Fast CPU copy (lv_memcpy) */
    uint8_t *src_ptr = src_area
                       ? (uint8_t *)lv_draw_buf_goto_xy(src, src_area->x1, src_area->y1)
                       : src->data;
    uint8_t *dst_ptr = dest_area
                       ? (uint8_t *)lv_draw_buf_goto_xy(dest, dest_area->x1, dest_area->y1)
                       : dest->data;

    uint32_t src_stride = src->header.stride;
    uint32_t dst_stride = dest->header.stride;

    if (line_bytes == src_stride && line_bytes == dst_stride)
    {
        /* Contiguous block across full width (e.g. screen-sized or band copy) */
        uint32_t total_bytes = (uint32_t)height * line_bytes;
        lv_memcpy(dst_ptr, src_ptr, total_bytes);
        dcache_clean_by_mva(dst_ptr, total_bytes);
    }
    else
    {
        /* Rectangular sub-area copy line-by-line */
        for (int32_t y = 0; y < height; y++)
        {
            lv_memcpy(dst_ptr, src_ptr, line_bytes);
            dcache_clean_by_mva(dst_ptr, line_bytes);
            src_ptr += src_stride;
            dst_ptr += dst_stride;
        }
    }
}


#endif /*LV_USE_DRAW_GFX*/
