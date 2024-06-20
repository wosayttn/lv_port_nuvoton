/**
 * @file lv_draw_gdma_fill.c
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

#include "blend/lv_draw_sw_blend.h"
#include "lv_draw_sw_gradient.h"
#include "../../misc/lv_math.h"
#include "../../misc/lv_text_ap.h"
#include "../../core/lv_refr.h"
#include "../../misc/lv_assert.h"
#include "../../stdlib/lv_string.h"
#include "../lv_draw_mask.h"

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

void lv_draw_gdma_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                       const lv_area_t *coords)
{

    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);

    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, draw_unit->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);

    lv_area_t blend_area;
    if (!_lv_area_intersect(&blend_area, &rel_coords, &rel_clip_area))
        return; /*Fully clipped, nothing to do*/

    {
        const lv_area_t *dest_area = &blend_area;
        int32_t dest_stride = draw_buf->header.stride;
        lv_color_format_t dest_cf = draw_buf->header.cf;

        uint8_t px_size = lv_color_format_get_size(dest_cf);

        int32_t dest_x = dest_area->x1;
        int32_t dest_y = dest_area->y1;
        int32_t dest_w = lv_area_get_width(dest_area);
        int32_t dest_h = lv_area_get_height(dest_area);
        uint8_t *dest_buf = draw_buf->data + (dest_y * dest_stride + dest_x * px_size);
        uint32_t fill_color = (px_size == 2) ? (uint32_t)lv_color_to_u16(dsc->color) : lv_color_to_u32(dsc->color);

        int32_t UseBlockingWait = (dest_w * dest_h) < 120 ? 1 : 0;
        {
            enum dma350_lib_error_t lib_err;
            enum dma350_ch_transize_t pixelsize = (px_size == 2) ? DMA350_CH_TRANSIZE_16BITS : DMA350_CH_TRANSIZE_32BITS;
            enum dma350_lib_exec_type_t exec_type = UseBlockingWait ? DMA350_LIB_EXEC_BLOCKING : DMA350_LIB_EXEC_IRQ;
            struct dma350_ch_dev_t *dev = GDMA_CH_DEV_S[0];

            lib_err = verify_dma350_ch_dev_ready(dev);
            LV_ASSERT(lib_err == DMA350_LIB_ERR_NONE);

            lib_err = dma350_lib_set_des(dev, &dest_buf[0]);
            LV_ASSERT(lib_err == DMA350_LIB_ERR_NONE);

            dma350_ch_set_xaddr_inc(dev, 1, 1);
            dma350_ch_set_xsize32(dev, 0, dest_w);
            dma350_ch_set_ysize16(dev, 0, dest_h);
            dma350_ch_set_yaddrstride(dev, 0, (dest_stride / px_size));

            dma350_ch_set_transize(dev, pixelsize);
            dma350_ch_set_xtype(dev, DMA350_CH_XTYPE_FILL);
            dma350_ch_set_ytype(dev, DMA350_CH_YTYPE_FILL);
            dma350_ch_set_fill_value(dev, fill_color);

            void gdmaWaitForCompletion(struct dma350_ch_dev_t *dev, enum dma350_lib_exec_type_t exec_type);
            gdmaWaitForCompletion(dev, exec_type);
        }
    }

}

#endif /*LV_USE_DRAW_GDMA*/
