/**
 * @file lv_draw_2dge.h
 *
 */

#ifndef LV_DRAW_2DGE_H
#define LV_DRAW_2DGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_draw.h"

#if LV_USE_DRAW_2DGE

#include "../../misc/lv_area.h"
#include "../../misc/lv_color.h"
#include "../../display/lv_display.h"
#include "../../osal/lv_os.h"

#include "../../draw/lv_draw_vector.h"

#include "sys.h"
#include "2d.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t base_unit;
    lv_draw_task_t * task_act;
#if LV_USE_OS
    lv_thread_sync_t sync;
    lv_thread_t thread;
    volatile bool inited;
    volatile bool exit_status;
#endif
    uint32_t idx;
} lv_draw_2dge_unit_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the 2DGE renderer. Called in internally.
 */
void lv_draw_2dge_init(void);

/**
 * Deinitialize the 2DGE renderers
 */
void lv_draw_2dge_deinit(void);

/**
 * Fill an area using SW render. Handle gradient and radius.
 * @param draw_unit     pointer to a draw unit
 * @param dsc           the draw descriptor
 * @param coords        the coordinates of the rectangle
 */
void lv_draw_2dge_fill(lv_draw_unit_t * draw_unit, const lv_draw_fill_dsc_t * dsc, const lv_area_t * coords);

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_2DGE*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_2DGE_H*/

