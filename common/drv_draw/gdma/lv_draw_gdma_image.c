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

#include "lv_draw_gdma.h"

#if LV_USE_DRAW_GDMA

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
void lv_draw_gdma_image(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc,
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
    const uint8_t *src_buf = img_dsc->data + (src_y * src_stride + src_x * src_px_size);

    int32_t dest_x = blend_area.x1;
    int32_t dest_y = blend_area.y1;
    int32_t dest_w = lv_area_get_width(&blend_area);
    int32_t dest_h = lv_area_get_height(&blend_area);
    int32_t dest_stride = draw_buf->header.stride;
    uint8_t dest_px_size = lv_color_format_get_size(draw_buf->header.cf);
    uint8_t *dest_buf = draw_buf->data + (dest_y * dest_stride + dest_x * dest_px_size);

    int32_t UseBlockingWait = (dest_w * dest_h) < 120 ? 1 : 0;
    {
        enum dma350_lib_error_t lib_err;
        struct dma350_ch_dev_t *dev = GDMA_CH_DEV_S[0];

        lib_err = verify_dma350_ch_dev_ready(dev);
        LV_ASSERT(lib_err == DMA350_LIB_ERR_NONE);

        lib_err = dma350_lib_set_src(dev, src_buf);
        LV_ASSERT(lib_err == DMA350_LIB_ERR_NONE);

        lib_err = dma350_lib_set_des(dev, dest_buf);
        LV_ASSERT(lib_err == DMA350_LIB_ERR_NONE);

        dma350_ch_set_xaddr_inc(dev, 1, 1);
        dma350_ch_set_xsize32(dev, src_w, dest_w);
        dma350_ch_set_ysize16(dev, src_h, dest_h);
        dma350_ch_set_yaddrstride(dev, src_stride / src_px_size, dest_stride / dest_px_size);

        dma350_ch_set_transize(dev, (dest_px_size == 2) ? DMA350_CH_TRANSIZE_16BITS : DMA350_CH_TRANSIZE_32BITS);
        dma350_ch_set_xtype(dev, DMA350_CH_XTYPE_CONTINUE);
        dma350_ch_set_ytype(dev, DMA350_CH_YTYPE_CONTINUE);

        void gdmaWaitForCompletion(struct dma350_ch_dev_t *dev, enum dma350_lib_exec_type_t exec_type);
        gdmaWaitForCompletion(dev, UseBlockingWait ? DMA350_LIB_EXEC_BLOCKING : DMA350_LIB_EXEC_IRQ);
    }
}

#endif /*LV_USE_DRAW_GDMA*/
