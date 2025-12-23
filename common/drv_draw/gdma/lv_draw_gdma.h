/**
 * @file lv_draw_gdma.h
 *
 */

#ifndef LV_DRAW_GDMA_H
#define LV_DRAW_GDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_GDMA
#include "../sw/lv_draw_sw_private.h"

#include "NuMicro.h"
#include "dma350_ch_drv.h"
#include "dma350_lib.h"

#if defined(__ICCARM__)
#include "arm_cmse.h" // patch from EWARM 9.50.2 service pack
#else
#include <arm_cmse.h>
#endif

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
} lv_draw_gdma_unit_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the GDMA renderer. Called in internally.
 */
void lv_draw_gdma_init(void);

/**
 * Deinitialize the GDMA renderers
 */
void lv_draw_gdma_deinit(void);

/**
 * Fill an area using gdma render. Handle gradient and radius.
 * @param t         pointer to a task unit
 */
void lv_draw_gdma_fill(lv_draw_task_t *t);

/**
 * Draw an image with gdma render. It handles tiling.
 * @param t         pointer to a task unit
 */
void lv_draw_gdma_image(lv_draw_task_t *t);

/**
 * Draw an layer with gdma render.
 * @param t         pointer to a task unit
 */
void lv_draw_gdma_layer(lv_draw_task_t *t);

/***********************
 * GLOBAL VARIABLES
 ***********************/
extern struct dma350_ch_dev_t *const GDMA_CH_DEV_S[];
/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_GDMA*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_GDMA_H*/

