/**
 * @file lv_draw_bitblt_fill.c
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

void lv_draw_bitblt_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
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

        uint32_t u32Color = lv_color_to_u32(dsc->color);
        S_DRVBLT_DEST_FB sDestFB;
        S_DRVBLT_ARGB8   sARGB8;

        bltSetFillOP((E_DRVBLT_FILLOP) TRUE);

        sARGB8.u8Blue   = (u32Color & 0x000000FF);
        sARGB8.u8Green  = (u32Color & 0x0000FF00) >> 8;
        sARGB8.u8Red    = (u32Color & 0x00FF0000) >> 16;
        sARGB8.u8Alpha  = (u32Color & 0xFF000000) >> 24;
        bltSetARGBFillColor(sARGB8);

        sDestFB.i32Stride  = dest_stride;
        sDestFB.i16Width   = dest_area->x2 + 1 - dest_area->x1;
        sDestFB.i16Height  = dest_area->y2 + 1 - dest_area->y1;

        sDestFB.u32FrameBufAddr = (uint32_t)dest_buf + dest_area->y1 * dest_stride + dest_area->x1 * px_size;

        sDestFB.i32XOffset = 0;
        sDestFB.i32YOffset = 0;

        bltSetDestFrameBuf(sDestFB);

        if (px_size == 2)
            bltSetDisplayFormat(eDRVBLT_DEST_RGB565);
        else
            bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);

        bltSetFillAlpha(0);

        bltTrigger();   // Trigger FILL operation.

        bltFlush();   // Wait for complete.
    }


}

#endif /*LV_USE_DRAW_BITBLT*/
