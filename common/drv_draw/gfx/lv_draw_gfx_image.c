/**
 * @file lv_draw_gfx_image.c
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
void lv_draw_gfx_image(lv_draw_task_t *t)
{
    lv_draw_image_dsc_t *dsc = t->draw_dsc;

    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN)
        return;

    lv_layer_t *layer = t->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    const lv_image_dsc_t *img_dsc = dsc->src;
    lv_area_t *coords = &t->_real_area;

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t clip_area;
    lv_area_copy(&clip_area, &t->clip_area);
    lv_area_move(&clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!lv_area_intersect(&blend_area, &rel_coords, &clip_area))
        return; /* Fully clipped, nothing to do */

    const uint8_t *src_buf = img_dsc->data;

    lv_area_t src_area;
    src_area.x1 = blend_area.x1 - (coords->x1 - layer->buf_area.x1);
    src_area.y1 = blend_area.y1 - (coords->y1 - layer->buf_area.y1);
    src_area.x2 = src_area.x1 + lv_area_get_width(coords) - 1;
    src_area.y2 = src_area.y1 + lv_area_get_height(coords) - 1;
    int32_t src_stride = img_dsc->header.stride;
    lv_color_format_t src_cf = img_dsc->header.cf;

    uint8_t *dest_buf = draw_buf->data;
    int32_t dest_stride = draw_buf->header.stride;
    lv_color_format_t dest_cf = draw_buf->header.cf;

    int32_t dest_w = lv_area_get_width(&blend_area);
    int32_t dest_h = lv_area_get_height(&blend_area);

    gfx_surface_t src_surface;
    gfx_surface_t dst_surface;

    lv_memzero(&src_surface, sizeof(src_surface));
    lv_memzero(&dst_surface, sizeof(dst_surface));

    if (src_cf == LV_COLOR_FORMAT_RGB565 && dest_cf == LV_COLOR_FORMAT_RGB565)
    {
        src_surface.format = GFX_RGB565;
        dst_surface.format = GFX_RGB565;
    }
    else
    {
        src_surface.format = GFX_ARGB8888;
        dst_surface.format = GFX_ARGB8888;
    }

    src_surface.width = img_dsc->header.w;
    src_surface.height = img_dsc->header.h;
    src_surface.stride = (src_stride == 0) ? lv_draw_buf_width_to_stride(img_dsc->header.w, src_cf) : src_stride;
    src_surface.planes[0] = (int)((uintptr_t)src_buf & 0xFFFFFFFFU);

    bool has_scale = (dsc->scale_x != LV_SCALE_NONE || dsc->scale_y != LV_SCALE_NONE);
    if (has_scale)
    {
        src_surface.rect.tl.x = 0;
        src_surface.rect.tl.y = 0;
        src_surface.rect.br.x = img_dsc->header.w;
        src_surface.rect.br.y = img_dsc->header.h;
    }
    else
    {
        src_surface.rect.tl.x = src_area.x1;
        src_surface.rect.tl.y = src_area.y1;
        src_surface.rect.br.x = src_area.x1 + dest_w;
        src_surface.rect.br.y = src_area.y1 + dest_h;
    }

    dst_surface.width = draw_buf->header.w;
    dst_surface.height = draw_buf->header.h;
    dst_surface.stride = dest_stride;
    dst_surface.planes[0] = (int)((uintptr_t)dest_buf & 0xFFFFFFFFU);

    dst_surface.rect.tl.x = blend_area.x1;
    dst_surface.rect.tl.y = blend_area.y1;
    dst_surface.rect.br.x = blend_area.x1 + dest_w;
    dst_surface.rect.br.y = blend_area.y1 + dest_h;

    dcache_clean_by_mva((const void *)src_buf,
                        src_surface.height * src_surface.stride);

    gfx_osal_lock(GFX_OSAL_WAIT_FOREVER);

    dcache_clean_by_mva((const void *)dest_buf,
                        dst_surface.height * dst_surface.stride);

    int ret;
    if (src_cf == LV_COLOR_FORMAT_ARGB8888 || dsc->opa < LV_OPA_MAX)
    {
        /* Alpha blend: set blend factors on surfaces */
        src_surface.blendfunc = GFX_SRC_ALPHA;
        dst_surface.blendfunc = GFX_ONE_MINUS_SRC_ALPHA;

        gfx_enable(g_gfx_handle, GFX_BLEND);

        if (dsc->opa < LV_OPA_MAX)
        {
            src_surface.global_alpha = dsc->opa;
            gfx_enable(g_gfx_handle, GFX_GLOBAL_ALPHA);
        }

        ret = gfx_blt(g_gfx_handle, &src_surface, &dst_surface);

        if (dsc->opa < LV_OPA_MAX)
        {
            gfx_disable(g_gfx_handle, GFX_GLOBAL_ALPHA);
        }
        gfx_disable(g_gfx_handle, GFX_BLEND);
    }
    else
    {
        ret = gfx_blt(g_gfx_handle, &src_surface, &dst_surface);
    }

    if (ret != 0)
    {
        LV_LOG_ERROR("gfx_blt returned %d", ret);
    }

    gfx_finish(g_gfx_handle);

    dcache_invalidate_by_mva((const void *)dest_buf,
                             dst_surface.height * dst_surface.stride);

    gfx_osal_unlock();
}

#endif /*LV_USE_DRAW_GFX*/
