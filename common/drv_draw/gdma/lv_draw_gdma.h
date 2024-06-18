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
#include "../lv_draw.h"

#if LV_USE_DRAW_GDMA

#include "../../misc/lv_area.h"
#include "../../misc/lv_color.h"
#include "../../display/lv_display.h"
#include "../../osal/lv_os.h"

#include "../../draw/lv_draw_vector.h"

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
    lv_draw_task_t *task_act;
#if LV_USE_OS
    lv_thread_sync_t sync;
    lv_thread_t thread;
    volatile bool inited;
    volatile bool exit_status;
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
 * @param draw_unit     pointer to a draw unit
 * @param dsc           the draw descriptor
 * @param coords        the coordinates of the rectangle
 */
void lv_draw_gdma_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);

/**
 * Draw an image with gdma render. It handles image decoding, tiling, transformations, and recoloring.
 * @param draw_unit     pointer to a draw unit
 * @param dsc           the draw descriptor
 * @param coords        the coordinates of the image
 */
void lv_draw_gdma_image(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);

/**
 * Blend a layer with gdma render
 * @param draw_unit     pointer to a draw unit
 * @param dsc           the draw descriptor
 * @param coords        the coordinates of the rectangle
 */
void lv_draw_gdma_layer(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);

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

