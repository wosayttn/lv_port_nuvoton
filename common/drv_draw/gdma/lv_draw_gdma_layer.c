/**
 * @file lv_draw_gdma_layer.c
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

#if LV_USE_PARALLEL_DRAW_DEBUG
    #include "../../core/lv_global.h"
#endif

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

#if LV_USE_PARALLEL_DRAW_DEBUG
    #define _draw_info LV_GLOBAL_DEFAULT()->draw_info
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


void lv_draw_gdma_layer(lv_draw_task_t *t)
{
    lv_draw_image_dsc_t *draw_dsc = t->draw_dsc;

    lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;
    const lv_draw_buf_t *draw_buf = layer_to_draw->draw_buf;

    /* It can happen that nothing was draw on a layer and therefore its buffer is not allocated.
     * In this case just return.
     */
    if (draw_buf == NULL)
        return;

    const lv_area_t area_to_draw =
    {
        .x1 = 0,
        .y1 = 0,
        .x2 = draw_buf->header.w - 1,
        .y2 = draw_buf->header.h - 1
    };
    lv_draw_buf_invalidate_cache(draw_buf, &area_to_draw);

    lv_draw_image_dsc_t new_draw_dsc = *draw_dsc;
    new_draw_dsc.src = draw_buf;
    t->draw_dsc = &new_draw_dsc;
    lv_draw_gdma_image(t);
    t->draw_dsc = draw_dsc;

}

#endif /*LV_USE_DRAW_GDMA*/
