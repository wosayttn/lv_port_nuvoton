/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

static void lv_port_disp_partial_update(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    /* Update dirty region. */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_RECT_UPDATE, (void *)area) == 0);

    lv_disp_flush_ready(disp_drv);
}

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;  /*Descriptor of a display driver*/
    S_LCD_INFO sLcdInfo = {0};
    void *buf1;

    /* Initial display device */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = sLcdInfo.u32ResWidth;
    disp_drv.ver_res = sLcdInfo.u32ResHeight;
    disp_drv.user_data = &sLcdInfo;
    disp_drv.flush_cb = lv_port_disp_partial_update;

    buf1 = sLcdInfo.pvVramStartAddr;

    /*Initialize `disp_buf` with the buffer(s).*/
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, sLcdInfo.u32VramSize / sizeof(lv_color_t));

    LV_LOG_INFO("Use one screen-size shadow buffer, buf1: 0x%08x", buf1);

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}
