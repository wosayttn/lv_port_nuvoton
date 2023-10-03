/**************************************************************************//**
 * @file     lv_gpu_n9h30_2dge.c
 * @brief    lvgl gpu supporting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
/**
 * @file lv_gpu_n9h30_2dge.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "sys_cache_ex.h"
#include "2d.h"

#include "lv_gpu_n9h30_2dge.h"
/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_16_SWAP
    #error "Can't use GE2D with LV_COLOR_16_SWAP 1"
#endif

#if !((LV_COLOR_DEPTH == 16) || (LV_COLOR_DEPTH == 32))
    /*Can't use GPU with other formats*/
    #error "Can't use GPU with other formats"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool lv_draw_n9h30_2dge_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area, lv_color_t color);

static bool lv_draw_n9h30_2dge_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride, const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
void lv_draw_n9h30_2dge_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    lv_draw_sw_init_ctx(drv, draw_ctx);

    lv_draw_n9h30_2dge_ctx_t *ge2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    ge2d_draw_ctx->blend = lv_draw_n9h30_2dge_blend;
    ge2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_n9h30_2dge_wait_cb;
}

void lv_draw_n9h30_2dge_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}

void lv_draw_n9h30_2dge_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
{
    lv_area_t blend_area;
    uint32_t blend_area_w, blend_area_stride, word_aligned;
    bool done = false;

    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

    blend_area_w = lv_area_get_width(&blend_area);
    blend_area_stride = blend_area_w * sizeof(lv_color_t);

    word_aligned = ((blend_area_stride & 0x3) == 0) &&
                   ((blend_area.x1 * (LV_COLOR_DEPTH / 8) & 0x3) == 0) ? 1 : 0;

    LV_LOG_INFO("[%s] %d %d %d",     __func__, lv_area_get_size(&blend_area), (blend_area_stride & 0x3), (blend_area.x1 & 0x2));

    if ((lv_area_get_size(&blend_area) > 7200) &&
            word_aligned &&
            (dsc->mask_buf == NULL) &&
            (dsc->blend_mode == LV_BLEND_MODE_NORMAL))
    {
        lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

        lv_color_t *dest_buf = draw_ctx->buf;
        dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);

        const lv_color_t *src_buf = dsc->src_buf;
        if (src_buf)
        {
#if 0
            lv_coord_t src_stride = lv_area_get_width(dsc->blend_area);
            src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 -  dsc->blend_area->x1);
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            done = lv_draw_n9h30_2dge_blend_map(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa);
#else
            done = false;
#endif
        }
        else if (dsc->opa >= LV_OPA_MAX)
        {
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            done = lv_draw_n9h30_2dge_blend_fill(dest_buf, dest_stride, &blend_area, dsc->color);
        }
    }


    if (!done)
    {
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}

static bool lv_draw_n9h30_2dge_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area, lv_color_t color)
{
    int32_t fill_area_w = lv_area_get_width(fill_area);
    int32_t fill_area_h = lv_area_get_height(fill_area);
    lv_color_t *start_buf = dest_buf - (fill_area->y1 * dest_stride) - fill_area->x1;

    LV_LOG_INFO("[%s] %d %d %08x color=%08x@%08x %d %d %dx%d", __func__, dest_stride, lv_area_get_size(fill_area), lv_color_to32(color),  color, dest_buf, fill_area->x1, fill_area->y1, fill_area_w, fill_area_h);

    sys_cache_clean_invalidated_dcache((uint32_t)start_buf, dest_stride * fill_area_h);

    /*Hardware filling*/
    // Enter GE2D ->
    ge2dInit(LV_COLOR_DEPTH, dest_stride, fill_area->y2, (void *)start_buf);

    ge2dClip_SetClip(fill_area->x1, fill_area->y1, fill_area->x2, fill_area->y2);

#if (LV_COLOR_DEPTH == 32)
    ge2dFill_Solid(fill_area->x1, fill_area->y1, fill_area_w, fill_area_h, color.full);
#elif (LV_COLOR_DEPTH == 16)
    ge2dFill_Solid_RGB565(fill_area->x1, fill_area->y1, fill_area_w, fill_area_h, color.full);
#endif

    ge2dClip_SetClip(-1, 0, 0, 0);
    // -> Leave GE2D

    return true;
}


static bool lv_draw_n9h30_2dge_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride,
        const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa)
{
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);
    int32_t dest_x = dest_area->x1;
    int32_t dest_y = dest_area->y1;
    const lv_color_t *dest_start_buf = dest_buf - (dest_y * dest_stride) - dest_x;

    // Enter GE2D ->
    ge2dInit(LV_COLOR_DEPTH, dest_stride, dest_area->y2, (void *)dest_start_buf);

    if (opa >= LV_OPA_MAX)
    {
        ge2dBitblt_SetAlphaMode(0, 0, 0);
        ge2dBitblt_SetDrawMode(0, 0, 0);
    }
    else
    {
        ge2dBitblt_SetAlphaMode(1, opa, opa);
    }

    sys_cache_clean_dcache((uint32_t)src_buf, sizeof(lv_color_t) * (src_stride * dest_h + dest_w));

    ge2dSpriteBlt_Screen(dest_x, dest_y, dest_w, dest_h, (void *)src_buf);
    // -> Leave GE2D

    return true;
}



void lv_gpu_n9h30_2dge_wait_cb(lv_draw_ctx_t *draw_ctx)
{
    lv_draw_sw_wait_for_finish(draw_ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
