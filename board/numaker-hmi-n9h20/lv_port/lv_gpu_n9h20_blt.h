/**************************************************************************//**
 * @file     lv_gpu_n9h20_blt.h
 * @brief    lvgl gpu supporting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#ifndef LV_GPU_N9H20_BLT_H
#define LV_GPU_N9H20_BLT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../misc/lv_color.h"
#include "../../hal/lv_hal_disp.h"
#include "../sw/lv_draw_sw.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
struct _lv_disp_drv_t;
typedef lv_draw_sw_ctx_t lv_draw_n9h20_blt_ctx_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
void lv_draw_n9h20_blt_init(void);

void lv_draw_n9h20_blt_ctx_init(struct _lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx);

void lv_draw_n9h20_blt_ctx_deinit(struct _lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx);

void lv_draw_n9h20_blt_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc);

void lv_gpu_n9h20_blt_wait_cb(lv_draw_ctx_t *draw_ctx);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GPU_N9H20_BLT_H*/

