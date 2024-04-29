/**************************************************************************//**
 * @file     disp_ssd1963.c
 * @brief    ssd1963 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"

int disp_init(void)
{
    //DISP_CLR_DISPLAY;
    DISP_CLR_BACKLIGHT;

    /* Initial control registers */
    DISP_SET_RST;
    disp_delay_ms(10);     // Delay 5ms
    DISP_CLR_RST;
    disp_delay_ms(40);    // Delay 20ms
    DISP_SET_RST;
    disp_delay_ms(80);    // Delay 40ms

    DISP_WRITE_REG(0xe0);    //Start PLL. Before the start, the system was operated with the crystal oscillator or clock input.
    DISP_WRITE_DATA(0x00);   //0: Disable PLL, 1:Enable PLL.

    DISP_WRITE_REG(0xe2);
    DISP_WRITE_DATA(0x23);
    DISP_WRITE_DATA(0x2);
    DISP_WRITE_DATA(0x4);

    DISP_WRITE_REG(0xe0);    //Start PLL. Before the start, the system was operated with the crystal oscillator or clock input.
    DISP_WRITE_DATA(0x01);   //0: Disable PLL, 1:Enable PLL.

    disp_delay_ms(100);

    DISP_WRITE_REG(0xe0);
    DISP_WRITE_DATA(0x03);   //0: Disable PLL, 1:Enable PLL. 3:Enable+LOCK PLL
    disp_delay_ms(10);

    DISP_WRITE_REG(0x01);    //Software reset
    disp_delay_ms(10);

    DISP_WRITE_REG(0xe6);    //SET pixel clock frequency
    DISP_WRITE_DATA(0x01);
    DISP_WRITE_DATA(0x99);
    DISP_WRITE_DATA(0x9a);

    DISP_WRITE_REG(0xb0);    //SET LCD MODE  SET TFT MODE
    DISP_WRITE_DATA(0x20);   //SET 24Bit, Disable TFT FRC & dithering, DCLK in falling edge, LLINE Active low, LFRAME Active low
    DISP_WRITE_DATA(0x00);   //SET TFT mode
    DISP_WRITE_DATA(0x01);   //SET horizontal size=480-1 HightByte
    DISP_WRITE_DATA(0xdf);   //SET horizontal size=480-1 LowByte
    DISP_WRITE_DATA(0x01);   //SET vertical size=272-1 HightByte
    DISP_WRITE_DATA(0x0f);   //SET vertical size=272-1 LowByte
    DISP_WRITE_DATA(0x00);   //SET even/odd line RGB seq.=RGB

    DISP_WRITE_REG(0xf0);    //SET pixel data I/F format=16bit(565 format)
    DISP_WRITE_DATA(0x03);

    //DISP_WRITE_REG(0xf0);    //SET pixel data I/F format=8bit (666 format)
    //DISP_WRITE_DATA(0x00);

    DISP_WRITE_REG(0x36);    // SET read from frame buffer to the display is RGB
    DISP_WRITE_DATA(0x00);


    DISP_WRITE_REG(0xb4);    //SET HBP,
    DISP_WRITE_DATA(0x02);   //SET HSYNC Tatol = 525
    DISP_WRITE_DATA(0x0d);
    DISP_WRITE_DATA(0x00);   //SET HBP = 20
    DISP_WRITE_DATA(0x14);
    DISP_WRITE_DATA(0x05);    //SET VBP 5
    DISP_WRITE_DATA(0x00);    //SET Hsync pulse start position
    DISP_WRITE_DATA(0x00);
    DISP_WRITE_DATA(0x00);    //SET Hsync pulse subpixel start position

    DISP_WRITE_REG(0xb6);     //SET VBP,
    DISP_WRITE_DATA(0x01);    //SET Vsync total 292
    DISP_WRITE_DATA(0x24);
    DISP_WRITE_DATA(0x00);    //SET VBP = 10
    DISP_WRITE_DATA(0x0a);
    DISP_WRITE_DATA(0x05);    //SET Vsync pulse 5
    DISP_WRITE_DATA(0x00);    //SET Vsync pulse start position
    DISP_WRITE_DATA(0x00);

    DISP_WRITE_REG(0x29);     //SET display on
    disp_delay_ms(5);

    //DISP_SET_DISPLAY;
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
