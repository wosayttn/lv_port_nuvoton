/**
 * @file lv_draw_gfx.h
 *
 */

#ifndef LV_DRAW_GFX_H
#define LV_DRAW_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#include "../../misc/lv_area_private.h"

#if LV_USE_DRAW_GFX
#include "../sw/lv_draw_sw_private.h"
#include "libgfx.h"

extern void *g_gfx_handle;

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
} lv_draw_gfx_unit_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the GFX renderer. Called internally.
 */
void lv_draw_gfx_init(void);

/**
 * Deinitialize the GFX renderer.
 */
void lv_draw_gfx_deinit(void);

/**
 * Fill an area using GFX render. Handles solid fill.
 * @param t             pointer to a draw task
 */
void lv_draw_gfx_fill(lv_draw_task_t *t);

/**
 * Draw an image with GFX render. Handles blit and alpha blend.
 * @param t             pointer to a draw task
 */
void lv_draw_gfx_image(lv_draw_task_t *t);

/**
 * Blend a layer with GFX render.
 * @param t             pointer to a draw task
 */
void lv_draw_gfx_layer(lv_draw_task_t *t);

/**
 * Draw a label with GFX render. Handles A8 glyph batching.
 * @param t             pointer to a draw task
 */
void lv_draw_gfx_label(lv_draw_task_t *t);

/**
 * Draw a single letter with GFX render. Handles A8 glyph.
 * @param t             pointer to a draw task
 */
void lv_draw_gfx_letter(lv_draw_task_t *t);

/**
 * Deinitialize label drawing resources (e.g. glyph atlas).
 */
void lv_draw_gfx_label_deinit(void);

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_GFX*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_GFX_H*/
