/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    /* Update dirty region. */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_RECT_UPDATE, (void *)area) == 0);

    lv_disp_flush_ready(disp);
}

void lv_port_disp_init(void)
{
    lv_display_t *disp;
    static S_LCD_INFO sLcdInfo = {0};

    /* Initial display device */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

    LV_LOG_INFO("Use one screen-size shadow buffer: 0x%08x", sLcdInfo.pvVramStartAddr);

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

		/*Set user-data*/
    lv_display_set_driver_data(disp, &sLcdInfo);

		/*Set a flush callback to draw to the display*/
    lv_display_set_flush_cb(disp, lv_port_disp_partial);
		
		/*Set an initialized buffer*/
    lv_display_set_buffers(disp, sLcdInfo.pvVramStartAddr, NULL, sLcdInfo.u32VramSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
