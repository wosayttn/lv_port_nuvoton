/**
 * @file lv_draw_2dge_fill.c
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
void lv_draw_2dge_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
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
        uint8_t *dest_buf = draw_buf->data;
        const lv_area_t *dest_area = &blend_area;
        int32_t dest_stride = draw_buf->header.stride;
        lv_color_format_t dest_cf = draw_buf->header.cf;

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
        void ge2dInit(int bpp, int width, int height, void *destination);
        ge2dInit(px_size << 3,  dest_stride / px_size, dest_h, (void *)dest_buf);

        /**
          * @brief Set the clip rectangle. (top-left to down-right).
          * @param[in] x1 is top-left x position
          * @param[in] y1 is top-left y position
          * @param[in] x2 is down-right x position
          * @param[in] y2 is down-right y position
          * @return none
          */
        void ge2dClip_SetClip(int x1, int y1, int x2, int y2);
        ge2dClip_SetClip(dest_area->x1, dest_area->y1, dest_area->x2, dest_area->y2);

        /**
          * @brief Rectangle solid color fill with RGB565 color.
          * @param[in] dx x position
          * @param[in] dy y position
          * @param[in] width is display width
          * @param[in] height is display height
          * @param[in] color is RGB565 color of foreground
          * @return none
          */
        void ge2dFill_Solid_RGB565(int dx, int dy, int width, int height, int color);
        void ge2dFill_Solid(int dx, int dy, int width, int height, int color);

        if (px_size == 4)
            ge2dFill_Solid(dest_area->x1, dest_area->y1, dest_w, dest_h, lv_color_to_u32(dsc->color));
        else if (px_size == 2)
            ge2dFill_Solid_RGB565(dest_area->x1, dest_area->y1, dest_w, dest_h, lv_color_to_u16(dsc->color));

        ge2dClip_SetClip(-1, 0, 0, 0);
        // -> Leave GE2D
    }


}

#endif /*LV_USE_DRAW_2DGE*/
