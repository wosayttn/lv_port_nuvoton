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

#include "lv_draw_2dge.h"

#if LV_USE_DRAW_2DGE

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
void lv_draw_2dge_image(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc,
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

    {
        lv_area_t *dest_area = &blend_area;
        int32_t src_w = lv_area_get_width(&src_area);
        int32_t src_h = lv_area_get_height(&src_area);
        int32_t dest_w = lv_area_get_width(dest_area);
        int32_t dest_h = lv_area_get_height(dest_area);
        uint8_t px_size = lv_color_format_get_size(dest_cf);

        // Enter GE2D ->
        /**
          * @brief Graphics engine initialization.
          * @param[in] bpp bit per pixel
          * @param[in] width is width of display memory
          * @param[in] height is height of display memory
          * @param[in] destination is pointer of destination buffer address
          * @return none
          */
        ge2dInit(px_size * 8, dest_stride / px_size, dest_h, (void *)dest_buf);

        if (dsc->opa >= LV_OPA_MAX)
        {
            ge2dBitblt_SetAlphaMode(0, 0, 0);
            ge2dBitblt_SetDrawMode(0, 0, 0);
        }
        else
        {
            ge2dBitblt_SetAlphaMode(1, dsc->opa, dsc->opa);
        }

        //ge2dSpriteBlt_Screen(dest_area->x1, dest_area->y1, dest_w, dest_h, (void *)src_buf);
        ge2dSpriteBltx_Screen(dest_area->x1, dest_area->y1, src_area.x1, src_area.y1, dest_w, dest_h, src_w, src_h, (void *)src_buf);
        // -> Leave GE2D
    }
}

#endif /*LV_USE_DRAW_2DGE*/
