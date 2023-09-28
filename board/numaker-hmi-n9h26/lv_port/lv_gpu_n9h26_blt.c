/**************************************************************************//**
 * @file     lv_gpu_n9h26_blt.c
 * @brief    lvgl gpu supporting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
/**
 * @file lv_gpu_n9h26_blt.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "sys_cache_ex.h"

#include "lv_gpu_n9h26_blt.h"
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

static void lv_draw_n9h26_blt_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area, lv_color_t color);

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
void lv_draw_n9h26_blt_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    lv_draw_sw_init_ctx(drv, draw_ctx);

    lv_draw_n9h26_blt_ctx_t *ge2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    ge2d_draw_ctx->blend = lv_draw_n9h26_blt_blend;
    ge2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_n9h26_blt_wait_cb;
}

void lv_draw_n9h26_blt_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}

void lv_draw_n9h26_blt_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
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

        lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area) * sizeof(lv_color_t);

        const lv_color_t *src_buf = dsc->src_buf;

        if (src_buf)
        {
            // TODO: lv_draw_n9h26_blt_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_coord_t dest_stride, const lv_color_t *src_buf, lv_coord_t src_stride, lv_opa_t opa)
        }
        else if (dsc->opa >= LV_OPA_MAX) //Fill
        {
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);

            lv_draw_n9h26_blt_blend_fill(draw_ctx->buf, dest_stride, &blend_area, dsc->color);

            done = true;
        }
    }

    if (!done)
    {
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}

static void lv_draw_n9h26_blt_blend_fill(lv_color_t *dest_buf, lv_coord_t dest_stride, const lv_area_t *fill_area, lv_color_t color)
{
    int32_t fill_area_w = lv_area_get_width(fill_area);
    int32_t fill_area_h = lv_area_get_height(fill_area);
    lv_color_t *start_buf = dest_buf + (fill_area->y1 * dest_stride);

    LV_LOG_INFO("[%s] %d %d %08x color=%08x@%08x %d %d %dx%d", __func__, dest_stride, lv_area_get_size(fill_area), lv_color_to32(color),  color, dest_buf, fill_area->x1, fill_area->y1, fill_area_w, fill_area_h);

    sys_cache_clean_invalidated_dcache((uint32_t)start_buf, dest_stride * fill_area_h);

    // Fill color is non-premultiplied alpha.
    {
        S_FI_FILLOP clr_op;
        uint32_t u32Color = lv_color_to32(color);

        clr_op.sRect.i16Xmin = fill_area->x1;
        clr_op.sRect.i16Ymin = fill_area->y1;
        clr_op.sRect.i16Xmax = fill_area->x2 + 1;
        clr_op.sRect.i16Ymax = fill_area->y2 + 1;

        clr_op.sARGB8.u8Blue  = (u32Color & 0x000000FF);
        clr_op.sARGB8.u8Green = (u32Color & 0x0000FF00) >> 8;
        clr_op.sARGB8.u8Red   = (u32Color & 0x00FF0000) >> 16;
        clr_op.sARGB8.u8Alpha = (u32Color & 0xFF000000) >> 24;

        clr_op.u32FBAddr = (uint32_t)dest_buf;
        clr_op.i32Stride = dest_stride;

#if (LV_COLOR_DEPTH == 16)
        clr_op.eDisplayFmt = eDRVBLT_DEST_RGB565;
        clr_op.i32Blend = 0;    // No alpha blending.
#elif (LV_COLOR_DEPTH == 32)
        clr_op.eDisplayFmt = eDRVBLT_DEST_ARGB8888;
        clr_op.i32Blend = 0;    // No alpha blending.
#endif

        bltFIFill(clr_op);
    }
}

void lv_gpu_n9h26_blt_wait_cb(lv_draw_ctx_t *draw_ctx)
{
    lv_draw_sw_wait_for_finish(draw_ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
