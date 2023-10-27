/**************************************************************************//**
 * @file     lv_gpu_n9h20_blt.c
 * @brief    lvgl gpu supporting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
/**
 * @file lv_gpu_n9h20_blt.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "lv_gpu_n9h20_blt.h"
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

static bool lv_draw_n9h20_blt_blend_fill(lv_color_t *dest_buf, const lv_area_t *dest_area, const lv_area_t *fill_area, lv_color_t color);
static bool lv_draw_n9h20_blt_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_color_t *src_buf, lv_coord_t src_stride, const lv_area_t *src_area, lv_opa_t opa);

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
void lv_draw_n9h20_blt_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    lv_draw_sw_init_ctx(drv, draw_ctx);

    lv_draw_n9h20_blt_ctx_t *ge2d_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;

    ge2d_draw_ctx->blend = lv_draw_n9h20_blt_blend;
    ge2d_draw_ctx->base_draw.wait_for_finish = lv_gpu_n9h20_blt_wait_cb;
}

void lv_draw_n9h20_blt_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
    LV_UNUSED(drv);
    LV_UNUSED(draw_ctx);
}

void lv_draw_n9h20_blt_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
{
    lv_area_t blend_area;
    bool bAlignedWord, done = false;

    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

#if (LV_COLOR_DEPTH == 16)
    {
        uint32_t blend_area_w, blend_area_stride;
        blend_area_w = lv_area_get_width(&blend_area);
        blend_area_stride = blend_area_w * sizeof(lv_color_t);

        bAlignedWord = ((blend_area_stride & 0x3) == 0) &&
                       ((blend_area.x1 * (LV_COLOR_DEPTH / 8) & 0x3) == 0) ? true : false;
    }
#else
    bAlignedWord = true;
#endif

    if ((lv_area_get_size(&blend_area) > 6400) &&
            bAlignedWord &&
            (dsc->mask_buf == NULL) &&
            (dsc->blend_mode == LV_BLEND_MODE_NORMAL))
    {
        if (dsc->src_buf)
        {
            lv_color_t *src_buf = (lv_color_t *)dsc->src_buf + (lv_area_get_width(dsc->blend_area) * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1));
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);
            done = lv_draw_n9h20_blt_blend_map(draw_ctx->buf, draw_ctx->buf_area, (lv_color_t *)src_buf, (lv_coord_t)lv_area_get_width(dsc->blend_area), &blend_area, dsc->opa);
        }
        else if (dsc->opa >= LV_OPA_MAX) //Fill
        {
            lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);

            done = lv_draw_n9h20_blt_blend_fill(draw_ctx->buf, draw_ctx->buf_area, &blend_area, dsc->color);
        }
    }

    if (!done)
    {
        lv_draw_sw_blend_basic(draw_ctx, dsc);
    }
}

static bool lv_draw_n9h20_blt_blend_fill(lv_color_t *dest_buf, const lv_area_t *dest_area, const lv_area_t *fill_area, lv_color_t color)
{
#if 1
    int32_t fill_area_w = lv_area_get_width(fill_area);
    int32_t fill_area_h = lv_area_get_height(fill_area);
    lv_coord_t dest_stride = lv_area_get_width(dest_area);

    lv_color_t *start_buf = dest_buf + (fill_area->y1 * dest_stride);

    sysCleanInvalidatedDcache((UINT32)start_buf, (UINT32)(dest_stride * sizeof(lv_color_t) * fill_area_h));

    // Fill color is non-premultiplied alpha.
    {
        uint32_t u32Color = lv_color_to32(color);


        S_DRVBLT_DEST_FB sDestFB;
        S_DRVBLT_ARGB8   sARGB8;

        bltSetFillOP((E_DRVBLT_FILLOP) TRUE);

        sARGB8.u8Blue   = (u32Color & 0x000000FF);
        sARGB8.u8Green  = (u32Color & 0x0000FF00) >> 8;
        sARGB8.u8Red    = (u32Color & 0x00FF0000) >> 16;
        sARGB8.u8Alpha  = (u32Color & 0xFF000000) >> 24;
        bltSetARGBFillColor(sARGB8);

        sDestFB.i32Stride  = dest_stride * sizeof(lv_color_t);
        sDestFB.i16Width   = fill_area->x2 + 1 - fill_area->x1;
        sDestFB.i16Height  = fill_area->y2 + 1 - fill_area->y1;

        sDestFB.u32FrameBufAddr = (uint32_t)dest_buf + fill_area->y1 * (dest_stride * sizeof(lv_color_t)) + fill_area->x1 * sizeof(lv_color_t);

        sDestFB.i32XOffset = 0;
        sDestFB.i32YOffset = 0;

        bltSetDestFrameBuf(sDestFB);

#if (LV_COLOR_DEPTH == 16)
        bltSetDisplayFormat(eDRVBLT_DEST_RGB565);
#elif (LV_COLOR_DEPTH == 32)
        bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);
#endif

        bltSetFillAlpha(0);

        bltTrigger();   // Trigger FILL operation.

        //bltFlush();   // Wait for complete.

    }

    return true;

#else
    return false;
#endif
}

static bool lv_draw_n9h20_blt_blend_map(lv_color_t *dest_buf, const lv_area_t *dest_area, lv_color_t *src_buf, lv_coord_t src_stride, const lv_area_t *src_area, lv_opa_t opa)
{
#if 1
    int32_t src_x = src_area->x1;
    int32_t src_y = src_area->y1;
    int32_t src_w = lv_area_get_width(src_area);
    int32_t src_h = lv_area_get_height(src_area);

    int32_t dest_x = dest_area->x1;
    int32_t dest_y = dest_area->y1;
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);

    const lv_color_t *dest_start_buf = dest_buf + (dest_y * dest_w) + dest_x;
    const lv_color_t *src_start_buf = src_buf;


    bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.

#if (LV_COLOR_DEPTH == 16)
    bltSetDisplayFormat(eDRVBLT_DEST_RGB565);   // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_RGB565);        // Set source image format to RGB565.
#elif (LV_COLOR_DEPTH == 32)
    bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);   // Set destination format.
    bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);      // Set source image format to RGB888/ARGB8888.
#endif

    bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.

    {
        // Set transform matrix to identify matrix. So no scaling, no rotation, no shearing, etc.
        S_DRVBLT_MATRIX xform_mx;

        xform_mx.a  =   0x10000;
        xform_mx.b  =   0;
        xform_mx.c  =   0;
        xform_mx.d  =   0x10000;

        bltSetTransformMatrix(xform_mx);
    }

    {
        // Set color multiplier for color transform.
        S_DRVBLT_ARGB16 color_multiplier;

        color_multiplier.i16Blue    =   0x100;
        color_multiplier.i16Green   =   0x100;
        color_multiplier.i16Red     =   0x100;
        color_multiplier.i16Alpha   = (int16_t)(opa);

        bltSetColorMultiplier(color_multiplier);
    }

    {
        // Set color offset for color transform
        S_DRVBLT_ARGB16 color_offset;

        color_offset.i16Blue    =   0;
        color_offset.i16Green   =   0;
        color_offset.i16Red     =   0;
        color_offset.i16Alpha   =   0;

        bltSetColorOffset(color_offset);
    }

    // Apply color transformation on all 4 channels.
    if (opa < LV_OPA_MAX)
    {
        bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
    }
    else
    {
        bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
    }
    bltSetFillStyle((E_DRVBLT_FILL_STYLE)(eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH));  // No smoothing.

    {
        // Set source image.
        S_DRVBLT_SRC_IMAGE src_img = {0};

        src_img.u32SrcImageAddr = (UINT32)src_buf;
        src_img.i32XOffset = -src_x * 0x10000;     // 16.16
        src_img.i32YOffset = -src_y * 0x10000;     // 16.16
        src_img.i16Width   = src_w;
        src_img.i16Height  = src_h;
        src_img.i32Stride  = src_stride * sizeof(lv_color_t);

        bltSetSrcImage(src_img);
    }

    {
        // Set destination buffer.
        S_DRVBLT_DEST_FB dst_img = {0};

        dst_img.u32FrameBufAddr = (UINT32)dest_buf;
        dst_img.i16Width   = dest_w;
        dst_img.i16Height  = dest_h;
        dst_img.i32Stride  = dest_w * sizeof(lv_color_t);

        bltSetDestFrameBuf(dst_img);
    }

    sysCleanInvalidatedDcache((UINT32)dest_start_buf, sizeof(lv_color_t) * src_stride * src_h);
    sysCleanDcache((uint32_t)src_start_buf, sizeof(lv_color_t) * (src_stride * src_h));

    bltTrigger();   // Trigger Blit operation.

    //bltFlush();   // Wait for complete.

    return true;
#else
    return false;
#endif
}

void lv_gpu_n9h20_blt_wait_cb(lv_draw_ctx_t *draw_ctx)
{
    bltFlush();     // Wait for triggerd FILL or BLIT action.
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
