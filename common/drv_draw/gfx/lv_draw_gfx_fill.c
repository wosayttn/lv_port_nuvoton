/**
 * @file lv_draw_gfx_fill.c
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

#include "../../misc/lv_area_private.h"
#include "cache.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

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

void lv_draw_gfx_fill(lv_draw_task_t *t)
{
    if (t == NULL)
        return;

    lv_draw_fill_dsc_t *dsc = t->draw_dsc;
    if (dsc == NULL)
        return;

    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN)
        return;

    lv_layer_t *layer = t->target_layer;
    if (layer == NULL || layer->draw_buf == NULL)
        return;

    lv_draw_buf_t *draw_buf = layer->draw_buf;
    if (draw_buf->data == NULL)
        return;

    lv_area_t *coords = &t->area;

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, &t->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, &rel_coords, &rel_clip_area))
        return; /* Fully clipped, nothing to do */

    int32_t fill_w = lv_area_get_width(&blend_area);
    int32_t fill_h = lv_area_get_height(&blend_area);
    if (fill_w <= 0 || fill_h <= 0)
        return;

    uint8_t *dest_buf = draw_buf->data;
    int32_t dest_stride = draw_buf->header.stride;
    lv_color_format_t dest_cf = draw_buf->header.cf;

    /* Verify 16-byte alignment requirement of GC520L */
    if (dest_stride & 15u)
    {
        sysprintf("[GFX_FILL] Stride alignment error: dest_stride=%d\n", dest_stride);
        return;
    }

    gfx_surface_t dst_surface;
    lv_memzero(&dst_surface, sizeof(dst_surface));
    if (dest_cf == LV_COLOR_FORMAT_RGB565)
    {
        dst_surface.format = GFX_RGB565;
    }
    else
    {
        dst_surface.format = GFX_XRGB8888;
    }
    dst_surface.width = draw_buf->header.w;
    dst_surface.height = draw_buf->header.h;
    dst_surface.stride = dest_stride;
    dst_surface.planes[0] = (int)((uintptr_t)dest_buf & 0xFFFFFFFFU);

    dst_surface.rect.tl.x = blend_area.x1;
    dst_surface.rect.tl.y = blend_area.y1;
    dst_surface.rect.br.x = blend_area.x2 + 1;
    dst_surface.rect.br.y = blend_area.y2 + 1;

    uint32_t color_val;
    if (dest_cf == LV_COLOR_FORMAT_RGB565)
    {
        uint16_t c16 = lv_color_to_u16(dsc->color);
        color_val = (uint32_t)c16 | ((uint32_t)c16 << 16);
    }
    else
    {
        color_val = lv_color_to_u32(dsc->color) | 0xFF000000;
    }
    dst_surface.clrcolor = (int)color_val;

    uint32_t bpp = lv_color_format_get_size(dest_cf);
    uint32_t line_bytes = fill_w * bpp;
    uint8_t *dst_start = dest_buf + (blend_area.y1 * dest_stride) + (blend_area.x1 * bpp);

    /*
     * Cache Maintenance:
     * 1. Before GPU writes:
     *    Clean D-Cache so any pending CPU writes in or near this bounding box
     *    are written back to DDR. This ensures that:
     *    a) If alpha blending is used, GPU reads the latest pixels from DDR.
     *    b) Boundary cache lines shared with adjacent widgets are updated in DDR,
     *       preventing stale CPU cache from clobbering the GPU fill during later flushes!
     */
    if (line_bytes == (uint32_t)dest_stride)
    {
        dcache_clean_by_mva(dst_start, fill_h * dest_stride);
    }
    else
    {
        for (int32_t y = 0; y < fill_h; y++)
        {
            dcache_clean_by_mva(dst_start + y * dest_stride, line_bytes);
        }
    }

    /*
     * Critical Section:
     * gfx_fill() submits commands asynchronously (gfx_submit_async) and returns.
     * Use gfx_osal_lock/unlock to hold the GPU hardware mutex continuously across
     * submission and gfx_finish(), preventing other tasks from clobbering the GPU engine!
     */
    gfx_osal_lock(GFX_OSAL_WAIT_FOREVER);

    int ret = gfx_fill(g_gfx_handle, &dst_surface);
    if (ret != 0)
    {
        sysprintf("[GFX_FILL] gfx_fill failed ret=%d\n", ret);
    }

    gfx_finish(g_gfx_handle);

    /*
     * 2. After GPU writes:
     *    Clean & Invalidate CPU D-Cache so CPU reads newly filled pixels directly from DDR,
     *    while safely preserving neighboring CPU-rendered pixels sharing boundary cache lines.
     */
    if (line_bytes == (uint32_t)dest_stride)
    {
        dcache_clean_invalidate_by_mva(dst_start, fill_h * dest_stride);
    }
    else
    {
        for (int32_t y = 0; y < fill_h; y++)
        {
            dcache_clean_invalidate_by_mva(dst_start + y * dest_stride, line_bytes);
        }
    }

    gfx_osal_unlock();
}

#endif /*LV_USE_DRAW_GFX*/
