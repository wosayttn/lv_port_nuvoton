/**
 * @file lv_draw_2dge_img.c
 *
 */

/**
 * Copyright 2020-2024 Nuvoton
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_bitblt.h"

#if LV_USE_DRAW_BITBLT

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
static bool _do_bitblt_draw_img_supported(const lv_draw_image_dsc_t *draw_dsc)
{
    const lv_image_dsc_t *img_dsc = draw_dsc->src;

    bool has_recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);

    bool has_transform = (draw_dsc->rotation != 0 || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE);
    bool has_opa = (draw_dsc->opa < (lv_opa_t)LV_OPA_MAX);
    bool src_has_alpha = (img_dsc->header.cf == LV_COLOR_FORMAT_ARGB8888);

    LV_LOG_USER("draw_dsc->rotation: %d", draw_dsc->rotation);
    LV_LOG_USER("src_has_alpha: %d", src_has_alpha);
    LV_LOG_USER("scale_x: %d, scale_y: %d", draw_dsc->scale_x, draw_dsc->scale_y);

    /* Recolor and transformation are not supported at the same time. */
    if (has_recolor || has_transform)
        return false;

    return true;
}

void lv_draw_bitblt_image(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc,
                          const lv_area_t *coords)
{
    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN)
        return;

    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    const lv_image_dsc_t *img_dsc = dsc->src;

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);
    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, draw_unit->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!_lv_area_intersect(&blend_area, &rel_coords, &rel_clip_area))
        return; /*Fully clipped, nothing to do*/

    lv_area_t src_area;
    src_area.x1 = blend_area.x1 - (coords->x1 - layer->buf_area.x1);
    src_area.y1 = blend_area.y1 - (coords->y1 - layer->buf_area.y1);
    src_area.x2 = src_area.x1 + lv_area_get_width(coords) - 1;
    src_area.y2 = src_area.y1 + lv_area_get_height(coords) - 1;

    int32_t src_x = src_area.x1;
    int32_t src_y = src_area.y1;
    int32_t src_w = lv_area_get_width(&src_area);
    int32_t src_h = lv_area_get_height(&src_area);
    int32_t src_stride = img_dsc->header.stride;
    uint8_t src_px_size = lv_color_format_get_size(img_dsc->header.cf);
    uint8_t src_cf = img_dsc->header.cf;
    const uint8_t *src_buf = img_dsc->data;
    bool src_has_alpha = (img_dsc->header.cf == LV_COLOR_FORMAT_ARGB8888);

    int32_t dest_x = blend_area.x1;
    int32_t dest_y = blend_area.y1;
    int32_t dest_w = lv_area_get_width(&blend_area);
    int32_t dest_h = lv_area_get_height(&blend_area);
    int32_t dest_stride = draw_buf->header.stride;
    uint8_t dest_px_size = lv_color_format_get_size(draw_buf->header.cf);
    uint8_t dest_cf = draw_buf->header.cf;
    uint8_t *dest_buf = draw_buf->data + (dest_y * dest_stride + dest_x * dest_px_size);

    _do_bitblt_draw_img_supported(dsc);

    LV_LOG_USER("src@%08x, stride: %d, x: %d, y: %d, w: %d, h: %d, cf: %d, px_size: %d", src_buf, src_stride, src_x, src_y, src_w, src_h, src_cf, src_px_size);
    LV_LOG_USER("dest@%08x, stride: %d, x: %d, y: %d, w: %d, h: %d, cf: %d, px_size: %d", dest_buf, dest_stride, dest_x, dest_y, dest_w, dest_h, dest_cf, dest_px_size);

    {
        bltSetFillOP((E_DRVBLT_FILLOP) FALSE);  // Blit operation.

        switch (src_cf)
        {
        case LV_COLOR_FORMAT_RGB565:
            bltSetSrcFormat(eDRVBLT_SRC_RGB565);
            break;

        case LV_COLOR_FORMAT_XRGB8888:
            bltSetRevealAlpha(eDRVBLT_NO_EFFECTIVE);    // Source image format is non-premultiplied alpha.

        case LV_COLOR_FORMAT_ARGB8888:
            bltSetSrcFormat(eDRVBLT_SRC_ARGB8888);
            break;

        default:
            return;
        }

        switch (dest_cf)
        {
        case LV_COLOR_FORMAT_RGB565:
            bltSetDisplayFormat(eDRVBLT_DEST_RGB565);
            break;

        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
            bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);
            break;

        default:
            return;
        }

        {
            /* Set transform matrix to identify matrix. So no scaling, no rotation, no shearing, etc. */
            S_DRVBLT_MATRIX xform_mx = {0};

            xform_mx.a = 0x10000;
            xform_mx.b = 0;
            xform_mx.c = 0;
            xform_mx.d = 0x10000;

            bltSetTransformMatrix(xform_mx);
        }

        {
            /* Set color multiplier for color transform. */
            S_DRVBLT_ARGB16 color_multiplier = {0};

            color_multiplier.i16Blue  = 0x100;
            color_multiplier.i16Green = 0x100;
            color_multiplier.i16Red   = 0x100;
            color_multiplier.i16Alpha = (int16_t)(dsc->opa);

            bltSetColorMultiplier(color_multiplier);
        }

        {
            /* Set color offset for color transform */
            S_DRVBLT_ARGB16 color_offset = {0};

            color_offset.i16Blue  = 0;
            color_offset.i16Green = 0;
            color_offset.i16Red   = 0;
            color_offset.i16Alpha = 0;

            bltSetColorOffset(color_offset);
        }

        /* Apply color transformation on all 4 channels. */
        if (src_has_alpha)
        {
            bltSetTransformFlag(eDRVBLT_HASTRANSPARENCY | eDRVBLT_HASCOLORTRANSFORM);
        }
        else
        {
            bltSetTransformFlag(eDRVBLT_HASCOLORTRANSFORM);
        }
        bltSetFillStyle((E_DRVBLT_FILL_STYLE)(eDRVBLT_NONE_FILL | eDRVBLT_NOTSMOOTH));  // No smoothing.

        {
            /* Set source image. */
            S_DRVBLT_SRC_IMAGE src_img = {0};

            src_img.u32SrcImageAddr = (UINT32)src_buf;
            src_img.i32XOffset = src_x * 0x10000; // 16.16
            src_img.i32YOffset = src_y * 0x10000; // 16.16
            src_img.i16Width   = src_w;
            src_img.i16Height  = src_h;
            src_img.i32Stride  = src_stride;

            bltSetSrcImage(src_img);
        }

        {
            /* Set destination buffer. */
            S_DRVBLT_DEST_FB dst_img = {0};

            dst_img.u32FrameBufAddr = (UINT32)dest_buf;
            dst_img.i16Width   = dest_w;
            dst_img.i16Height  = dest_h;
            dst_img.i32Stride  = dest_stride;

            bltSetDestFrameBuf(dst_img);
        }

        /* Trigger Blit operation. */
        bltTrigger();

        void bitbltWaitForCompletion(void);
        bitbltWaitForCompletion();
    }
}

#endif /*LV_USE_DRAW_BITBLT*/
