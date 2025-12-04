/**
 * @file lv_draw_bitblt.h
 *
 */

#ifndef LV_DRAW_BITBLT_H
#define LV_DRAW_BITBLT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_BITBLT
#include "../sw/lv_draw_sw_private.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    lv_draw_unit_t base_unit;
#if LV_USE_OS
    lv_draw_sw_thread_dsc_t thread_dsc;
#else
    lv_draw_task_t *task_act;
#endif
    uint32_t idx;
} lv_draw_bitblt_unit_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the BITBLT renderer. Called in internally.
 */
void lv_draw_bitblt_init(void);

/**
 * Deinitialize the BITBLT renderers
 */
void lv_draw_bitblt_deinit(void);

/**
 * Fill an area using SW render. Handle gradient and radius.
 * @param t             pointer to a task unit
 */
void lv_draw_bitblt_fill(lv_draw_task_t *t);

/**
 * Draw an image with bitblt render. It handles image decoding, tiling, transformations, and recoloring.
 * @param t             pointer to a task unit
 */
void lv_draw_bitblt_image(lv_draw_task_t *t);

/**
 * Blend a layer with bitblt render
 * @param t             pointer to a task unit
 */
void lv_draw_bitblt_layer(lv_draw_task_t *t);

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_BITBLT*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_BITBLT_H*/

