/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define DEF_FRAMEBUFFER_SIZE    (LV_HOR_RES_MAX*LV_VER_RES_MAX*(LV_COLOR_DEPTH/8))
static uint8_t s_au8FrameBuf[DEF_FRAMEBUFFER_SIZE] __attribute__((aligned(DCACHE_LINE_SIZE)));

static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_disp_flush_ready(disp);
}

void lv_port_disp_init(void)
{
    lv_display_t *disp;

    disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    LV_ASSERT(disp != NULL);

    /*Set a flush callback to draw to the display*/
    lv_display_set_flush_cb(disp, lv_port_disp_partial);

    /*Set an initialized buffer*/
    lv_display_set_buffers(disp, s_au8FrameBuf, NULL, DEF_FRAMEBUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
