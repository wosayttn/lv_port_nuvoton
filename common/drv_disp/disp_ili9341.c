/**************************************************************************//**
 * @file     disp_ili9341.c
 * @brief    ili9431 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ili9341.h"

int disp_ili9341_init(void)
{
    SET_RS;
    SET_RST;
    SET_BACKLIGHT_OFF;

    /* Hardware reset */
    SET_RST;
    ili9341_delay_ms(5);     // Delay 5ms

    CLR_RST;
    ili9341_delay_ms(20);    // Delay 20ms

    SET_RST;
    ili9341_delay_ms(40);    // Delay 40ms

    /* Initial control registers */
    ili9341_send_cmd(0xCB);
    ili9341_send_cmd_parameter(0x39);
    ili9341_send_cmd_parameter(0x2C);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0x34);
    ili9341_send_cmd_parameter(0x02);

    ili9341_send_cmd(0xCF);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0xC1);
    ili9341_send_cmd_parameter(0x30);

    ili9341_send_cmd(0xE8);
    ili9341_send_cmd_parameter(0x85);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0x78);

    ili9341_send_cmd(0xEA);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0x00);

    ili9341_send_cmd(0xED);
    ili9341_send_cmd_parameter(0x64);
    ili9341_send_cmd_parameter(0x03);
    ili9341_send_cmd_parameter(0x12);
    ili9341_send_cmd_parameter(0x81);

    ili9341_send_cmd(0xF7);
    ili9341_send_cmd_parameter(0x20);

    ili9341_send_cmd(0xC0);
    ili9341_send_cmd_parameter(0x23);

    ili9341_send_cmd(0xC1);
    ili9341_send_cmd_parameter(0x10);

    ili9341_send_cmd(0xC5);
    ili9341_send_cmd_parameter(0x3e);
    ili9341_send_cmd_parameter(0x28);

    ili9341_send_cmd(0xC7);
    ili9341_send_cmd_parameter(0x86);

    ili9341_send_cmd(0x36);

    if (LV_HOR_RES_MAX == 240)
        ili9341_send_cmd_parameter(0x48); // for 240x320
    else
        ili9341_send_cmd_parameter(0xE8); // for 320x240

    ili9341_send_cmd(0x3A);
    ili9341_send_cmd_parameter(0x55);

    ili9341_send_cmd(0xB1);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0x18);

    ili9341_send_cmd(0xB6);
    ili9341_send_cmd_parameter(0x08);
    ili9341_send_cmd_parameter(0x82);
    ili9341_send_cmd_parameter(0x27);

    ili9341_send_cmd(0xF2);
    ili9341_send_cmd_parameter(0x00);

    ili9341_send_cmd(0x26);
    ili9341_send_cmd_parameter(0x01);

    ili9341_send_cmd(0xE0);
    ili9341_send_cmd_parameter(0x0F);
    ili9341_send_cmd_parameter(0x31);
    ili9341_send_cmd_parameter(0x2B);
    ili9341_send_cmd_parameter(0x0C);
    ili9341_send_cmd_parameter(0x0E);
    ili9341_send_cmd_parameter(0x08);
    ili9341_send_cmd_parameter(0x4E);
    ili9341_send_cmd_parameter(0xF1);
    ili9341_send_cmd_parameter(0x37);
    ili9341_send_cmd_parameter(0x07);
    ili9341_send_cmd_parameter(0x10);
    ili9341_send_cmd_parameter(0x03);
    ili9341_send_cmd_parameter(0x0E);
    ili9341_send_cmd_parameter(0x09);
    ili9341_send_cmd_parameter(0x00);

    ili9341_send_cmd(0xE1);
    ili9341_send_cmd_parameter(0x00);
    ili9341_send_cmd_parameter(0x0E);
    ili9341_send_cmd_parameter(0x14);
    ili9341_send_cmd_parameter(0x03);
    ili9341_send_cmd_parameter(0x11);
    ili9341_send_cmd_parameter(0x07);
    ili9341_send_cmd_parameter(0x31);
    ili9341_send_cmd_parameter(0xC1);
    ili9341_send_cmd_parameter(0x48);
    ili9341_send_cmd_parameter(0x08);
    ili9341_send_cmd_parameter(0x0F);
    ili9341_send_cmd_parameter(0x0C);
    ili9341_send_cmd_parameter(0x31);
    ili9341_send_cmd_parameter(0x36);
    ili9341_send_cmd_parameter(0x0F);

    ili9341_send_cmd(0x11);

    ili9341_delay_ms(120);   // Delay 120ms

    ili9341_send_cmd(0x29);  //Display on

    SET_BACKLIGHT_ON;

    return 0;
}

void ili9341_fillrect(uint16_t *pixels, const lv_area_t *area)
{
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    LV_LOG_INFO("%08x WxH=%dx%d (%d, %d) (%d, %d)",
                pixels,
                lv_area_get_width(area),
                lv_area_get_height(area),
                area->x1,
                area->y1,
                area->x2,
                area->y2);

    ili9341_set_column(area->x1, area->x2);
    ili9341_set_page(area->y1, area->y2);
    ili9341_send_cmd(0x2c);

    ili9341_send_pixels(pixels, h * w * sizeof(uint16_t));
}
