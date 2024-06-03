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

void _dump_area(lv_area_t *a, int line)
{
    //sysprintf("[%d](%d, %d) -> (%d, %d)\n", line, a->x1, a->y1, a->x2, a->y2);
}

void lv_draw_2dge_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                       const lv_area_t *coords)
{
    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;

    sysprintf("================================================\n");

    lv_area_t rel_coords;
    lv_area_copy(&rel_coords, coords);

    lv_area_move(&rel_coords, -layer->buf_area.x1, -layer->buf_area.y1);
    _dump_area(&rel_coords, __LINE__);

    lv_area_t rel_clip_area;
    lv_area_copy(&rel_clip_area, draw_unit->clip_area);
    lv_area_move(&rel_clip_area, -layer->buf_area.x1, -layer->buf_area.y1);
    _dump_area(&rel_clip_area, __LINE__);

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

        sysprintf("opa: %d\n",  dsc->opa);
        sysprintf("radius: %d\n", dsc->radius);
        sysprintf("grad.dir: %d\n", dsc->grad.dir);
        sysprintf("color: %08x\n", dsc->color);
        sysprintf("lv_color_to_u32(color): %08x\n", lv_color_to_u32(dsc->color));
        sysprintf("lv_color_to_u16(color): %08x\n", lv_color_to_u16(dsc->color));
        sysprintf("dest_buf: %08x\n", dest_buf);
        sysprintf("px_size: %d %d\n", px_size, px_size << 3);
        sysprintf("size: %d\n", lv_area_get_size(dest_area));

        // Enter GE2D ->
        ge2dInit(px_size << 3, dest_stride, dest_h, (void *)dest_buf);

        ge2dClip_SetClip(dest_area->x1, dest_area->y1, dest_area->x2, dest_area->y2);

        if (px_size == 4)
            ge2dFill_Solid(dest_area->x1, dest_area->y1, dest_w, dest_h, lv_color_to_u32(dsc->color));
        else if (px_size == 2)
            ge2dFill_Solid_RGB565(dest_area->x1, dest_area->y1, dest_w, dest_h, lv_color_to_u16(dsc->color));

        ge2dClip_SetClip(-1, 0, 0, 0);
        // -> Leave GE2D
    }

    sysprintf("================================================\n");

}

#endif /*LV_USE_DRAW_2DGE*/
