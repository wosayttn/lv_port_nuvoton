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

        int32_t dest_x = dest_area->x1;
        int32_t dest_y = dest_area->y1;
        int32_t dest_w = lv_area_get_width(dest_area);
        int32_t dest_h = lv_area_get_height(dest_area);
        uint8_t dest_px_size = lv_color_format_get_size(dest_cf);

#if 0
        sysprintf("fill dsc->opa: %d\n", dsc->opa);
        sysprintf("dest_buf@%08x, stride: %d, x: %d, y: %d, w: %d, h: %d, cf: %d, px_size: %d\n", dest_buf, dest_stride, dest_x, dest_y, dest_w, dest_h, dest_cf, dest_px_size);
#endif

        uint32_t u32Color = lv_color_to_u32(dsc->color);
        {
            S_DRVBLT_DEST_FB sDestFB = {0};
            S_DRVBLT_ARGB8   sARGB8;

            bltSetFillOP((E_DRVBLT_FILLOP) TRUE);

            sARGB8.u8Blue   = (u32Color & 0x000000FF);
            sARGB8.u8Green  = (u32Color & 0x0000FF00) >> 8;
            sARGB8.u8Red    = (u32Color & 0x00FF0000) >> 16;
            sARGB8.u8Alpha  = dsc->opa;

            bltSetARGBFillColor(sARGB8);

            sDestFB.i32Stride  = dest_stride;
            sDestFB.i16Width   = dest_w;
            sDestFB.i16Height  = dest_h;

            sDestFB.u32FrameBufAddr = (uint32_t)dest_buf + dest_y * dest_stride + dest_x * dest_px_size;

            bltSetDestFrameBuf(sDestFB);
        }

        if (dest_px_size == 2)
            bltSetDisplayFormat(eDRVBLT_DEST_RGB565);
        else
            bltSetDisplayFormat(eDRVBLT_DEST_ARGB8888);

        bltSetFillAlpha(1);

        bltTrigger();

        void bitbltWaitForCompletion(void);
        bitbltWaitForCompletion();
    }
}

#endif /*LV_USE_DRAW_BITBLT*/
