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
static uint32_t fill_color[8] __attribute__((aligned(DCACHE_LINE_SIZE)));

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
        uint8_t *dest_buf = draw_buf->data;
        const lv_area_t *dest_area = &blend_area;
        int32_t dest_stride = draw_buf->header.stride;
        lv_color_format_t dest_cf = draw_buf->header.cf;

        int32_t dest_x = dest_area->x1;
        int32_t dest_y = dest_area->y1;
        int32_t dest_w = lv_area_get_width(dest_area);
        int32_t dest_h = lv_area_get_height(dest_area);
        uint8_t px_size = lv_color_format_get_size(dest_cf);
        fill_color[0] = (px_size == 2) ? (uint32_t)lv_color_to_u16(dsc->color) : lv_color_to_u32(dsc->color);

        volatile void *src_start_buf = (volatile void *)&fill_color[0];
        void *dest_start_buf = (void *)(dest_buf + dest_y * dest_stride + dest_x * px_size);

        /* Flush fill_color integrr buffer from cache to memory. */
        SCB_CleanDCache_by_Addr(src_start_buf, 4);
        int32_t UseBlockingWait = 1; /*(dest_w * dest_h) < 1200 ? 1 : 0;*/

        {
            enum dma350_lib_error_t lib_err;
            /**
             * \brief 2D Copy from canvas (area within a source bitmap) to within a
             *        destination bitmap, while applying various possible transformations.
             *        If the destination size is larger than the source, the source image
             *        will be wrapped (repeated).
             *
             * \param[in] dev             DMA350 channel device struct \ref dma350_ch_dev_t
             * \param[in] src             Source address, top left corner
             * \param[in] des             Destination address, top left corner
             * \param[in] src_width       Source width
             * \param[in] src_height      Source height
             * \param[in] src_line_width  Source line width
             * \param[in] des_width       Destination width
             * \param[in] des_height      Destination height
             * \param[in] des_line_width  Destination line width
             * \param[in] pixelsize       Size of a pixel as in \ref dma350_ch_transize_t
             * \param[in] transform       Transform type as in \ref dma350_lib_transform_t
             * \param[in] exec_type       Execution type as in \ref dma350_lib_exec_type_t
             *
             * \return Result of the operation \ref dma350_lib_error_t
             *
             * \note Destination width and height denote the area which will be filled at
             *       the destination address. The copy always starts from the top left
             *       corner of the source. If the requested destination size does not match
             *       the source, the resulting image will be repeated / cropped.
             */
            lib_err = dma350_draw_from_canvas(
                          GDMA_CH_DEV_S[0],
                          (const void *)src_start_buf, dest_start_buf,
                          1, 1, 1,
                          dest_w,  dest_h, (dest_stride / px_size),
                          (px_size == 2) ? DMA350_CH_TRANSIZE_16BITS : DMA350_CH_TRANSIZE_32BITS,
                          DMA350_LIB_TRANSFORM_NONE,
                          UseBlockingWait ? DMA350_LIB_EXEC_BLOCKING : DMA350_LIB_EXEC_IRQ);

            /* Check transfer result */
            if (lib_err != DMA350_LIB_ERR_NONE)
            {
                printf("\nGDMA transfer error !!\n");
                return;
            }

            if (!UseBlockingWait)
            {
                void gdmaWaitForCompletion(void);
                gdmaWaitForCompletion();
            }
        }
    }

}

#endif /*LV_USE_DRAW_GDMA*/
