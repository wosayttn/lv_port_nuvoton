/**************************************************************************//**
 * @file     disp_ili9341.c
 * @brief    ili9431 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"

int disp_init(void)
{
    DISP_SET_RST;
    DISP_CLR_BACKLIGHT;

    /* Hardware reset */
    DISP_SET_RST;
    disp_delay_ms(5);     // Delay 5ms

    DISP_CLR_RST;
    disp_delay_ms(20);    // Delay 20ms

    DISP_SET_RST;
    disp_delay_ms(40);    // Delay 40ms

    /* Initial control registers */
    DISP_WRITE_REG(0xCB);
    DISP_WRITE_DATA(0x39);
    DISP_WRITE_DATA(0x2C);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x34);
    DISP_WRITE_DATA(0x02);

    DISP_WRITE_REG(0xCF);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0xC1);
    DISP_WRITE_DATA(0x30);

    DISP_WRITE_REG(0xE8);
    DISP_WRITE_DATA(0x85);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x78);

    DISP_WRITE_REG(0xEA);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x00);

    DISP_WRITE_REG(0xED);
    DISP_WRITE_DATA(0x64);
    DISP_WRITE_DATA(0x03);
    DISP_WRITE_DATA(0x12);
    DISP_WRITE_DATA(0x81);

    DISP_WRITE_REG(0xF7);
    DISP_WRITE_DATA(0x20);

    DISP_WRITE_REG(0xC0);
    DISP_WRITE_DATA(0x23);

    DISP_WRITE_REG(0xC1);
    DISP_WRITE_DATA(0x10);

    DISP_WRITE_REG(0xC5);
    DISP_WRITE_DATA(0x3e);
    DISP_WRITE_DATA(0x28);

    DISP_WRITE_REG(0xC7);
    DISP_WRITE_DATA(0x86);

    DISP_WRITE_REG(0x36);

    if (LV_HOR_RES_MAX == 240)
        DISP_WRITE_DATA(0x48); // for 240x320
    else
        DISP_WRITE_DATA(0xE8); // for 320x240

    DISP_WRITE_REG(0x3A);
    DISP_WRITE_DATA(0x55);

    DISP_WRITE_REG(0xB1);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x18);

    DISP_WRITE_REG(0xB6);
    DISP_WRITE_DATA(0x08);
    DISP_WRITE_DATA(0x82);
    DISP_WRITE_DATA(0x27);

    DISP_WRITE_REG(0xF2);
    DISP_WRITE_DATA(0x00);

    DISP_WRITE_REG(0x26);
    DISP_WRITE_DATA(0x01);

    DISP_WRITE_REG(0xE0);
    DISP_WRITE_DATA(0x0F);
    DISP_WRITE_DATA(0x31);
    DISP_WRITE_DATA(0x2B);
    DISP_WRITE_DATA(0x0C);
    DISP_WRITE_DATA(0x0E);
    DISP_WRITE_DATA(0x08);
    DISP_WRITE_DATA(0x4E);
    DISP_WRITE_DATA(0xF1);
    DISP_WRITE_DATA(0x37);
    DISP_WRITE_DATA(0x07);
    DISP_WRITE_DATA(0x10);
    DISP_WRITE_DATA(0x03);
    DISP_WRITE_DATA(0x0E);
    DISP_WRITE_DATA(0x09);
    DISP_WRITE_DATA(0x00);

    DISP_WRITE_REG(0xE1);
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x0E);
    DISP_WRITE_DATA(0x14);
    DISP_WRITE_DATA(0x03);
    DISP_WRITE_DATA(0x11);
    DISP_WRITE_DATA(0x07);
    DISP_WRITE_DATA(0x31);
    DISP_WRITE_DATA(0xC1);
    DISP_WRITE_DATA(0x48);
    DISP_WRITE_DATA(0x08);
    DISP_WRITE_DATA(0x0F);
    DISP_WRITE_DATA(0x0C);
    DISP_WRITE_DATA(0x31);
    DISP_WRITE_DATA(0x36);
    DISP_WRITE_DATA(0x0F);

    DISP_WRITE_REG(0x11);

    disp_delay_ms(120);   // Delay 120ms

    DISP_WRITE_REG(0x29);  //Display on

    DISP_SET_BACKLIGHT;

    return 0;
}

void disp_fillrect(uint16_t *pixels, const lv_area_t *area)
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

    disp_set_column(area->x1, area->x2);
    disp_set_page(area->y1, area->y2);
    DISP_WRITE_REG(0x2c);

    disp_send_pixels(pixels, h * w * sizeof(uint16_t));
}
