/**************************************************************************//**
 * @file     disp_ssd1963.c
 * @brief    ssd1963 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ssd1963.h"

int disp_ssd1963_init(void)
{
    SET_DISP_OFF;
    SET_BACKLIGHT_OFF;

    /* Initial control registers */
    SET_RST;
    ssd1963_delay_ms(10);     // Delay 5ms

    CLR_RST;
    ssd1963_delay_ms(40);    // Delay 20ms

    SET_RST;
    ssd1963_delay_ms(80);    // Delay 40ms

    ssd1963_send_cmd(0xe0);    //Start PLL. Before the start, the system was operated with the crystal oscillator or clock input.
    ssd1963_send_cmd_parameter(0x00);   //0: Disable PLL, 1:Enable PLL.

    ssd1963_send_cmd(0xe2);
    ssd1963_send_cmd_parameter(0x23);
    ssd1963_send_cmd_parameter(0x2);
    ssd1963_send_cmd_parameter(0x4);

    ssd1963_send_cmd(0xe0);    //Start PLL. Before the start, the system was operated with the crystal oscillator or clock input.
    ssd1963_send_cmd_parameter(0x01);   //0: Disable PLL, 1:Enable PLL.

    ssd1963_delay_ms(100);

    ssd1963_send_cmd(0xe0);
    ssd1963_send_cmd_parameter(0x03);   //0: Disable PLL, 1:Enable PLL. 3:Enable+LOCK PLL
    ssd1963_delay_ms(10);

    ssd1963_send_cmd(0x01);    //Software reset
    ssd1963_delay_ms(10);

    ssd1963_send_cmd(0xe6);    //SET pixel clock frequency
    ssd1963_send_cmd_parameter(0x01);
    ssd1963_send_cmd_parameter(0x99);
    ssd1963_send_cmd_parameter(0x9a);

    ssd1963_send_cmd(0xb0);    //SET LCD MODE  SET TFT MODE
    ssd1963_send_cmd_parameter(0x20);   //SET 24Bit, Disable TFT FRC & dithering, DCLK in falling edge, LLINE Active low, LFRAME Active low
    ssd1963_send_cmd_parameter(0x00);   //SET TFT mode
    ssd1963_send_cmd_parameter(0x01);   //SET horizontal size=480-1 HightByte
    ssd1963_send_cmd_parameter(0xdf);   //SET horizontal size=480-1 LowByte
    ssd1963_send_cmd_parameter(0x01);   //SET vertical size=272-1 HightByte
    ssd1963_send_cmd_parameter(0x0f);   //SET vertical size=272-1 LowByte
    ssd1963_send_cmd_parameter(0x00);   //SET even/odd line RGB seq.=RGB

    ssd1963_send_cmd(0xf0);    //SET pixel data I/F format=16bit(565 format)
    ssd1963_send_cmd_parameter(0x03);

    //ssd1963_send_cmd(0xf0);    //SET pixel data I/F format=8bit (666 format)
    //ssd1963_send_cmd_parameter(0x00);

    ssd1963_send_cmd(0x36);    // SET read from frame buffer to the display is RGB
    ssd1963_send_cmd_parameter(0x00);


    ssd1963_send_cmd(0xb4);    //SET HBP,
    ssd1963_send_cmd_parameter(0x02);   //SET HSYNC Tatol = 525
    ssd1963_send_cmd_parameter(0x0d);
    ssd1963_send_cmd_parameter(0x00);   //SET HBP = 20
    ssd1963_send_cmd_parameter(0x14);
    ssd1963_send_cmd_parameter(0x05);    //SET VBP 5
    ssd1963_send_cmd_parameter(0x00);    //SET Hsync pulse start position
    ssd1963_send_cmd_parameter(0x00);
    ssd1963_send_cmd_parameter(0x00);    //SET Hsync pulse subpixel start position

    ssd1963_send_cmd(0xb6);     //SET VBP,
    ssd1963_send_cmd_parameter(0x01);    //SET Vsync total 292
    ssd1963_send_cmd_parameter(0x24);
    ssd1963_send_cmd_parameter(0x00);    //SET VBP = 10
    ssd1963_send_cmd_parameter(0x0a);
    ssd1963_send_cmd_parameter(0x05);    //SET Vsync pulse 5
    ssd1963_send_cmd_parameter(0x00);    //SET Vsync pulse start position
    ssd1963_send_cmd_parameter(0x00);

    ssd1963_send_cmd(0x29);    //SET display on
    ssd1963_delay_ms(5);

    SET_DISP_ON;

    SET_BACKLIGHT_ON;

    return 0;
}

void ssd1963_fillrect(uint16_t *pixels, const lv_area_t *area)
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

    ssd1963_set_column(area->x1, area->x2);
    ssd1963_set_page(area->y1, area->y2);
    ssd1963_send_cmd(0x2c);

    ssd1963_send_pixels(pixels, h * w * sizeof(uint16_t));
}
