/**************************************************************************//**
 * @file     ssd1963_ebi.c
 * @brief    ili9431 ebi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ssd1963.h"
#if defined(CONFIG_SSD1963_EBI_USE_PDMA)
    #include "drv_pdma.h"
#endif

#if !defined(CONFIG_SSD1963_EBI_ADDR)
    #error "Please specify CONFIG_SSD1963_EBI_ADDR in lv_conf.h"
#endif

#define SSD1963_ADDR_CMD  0x0
#define SSD1963_ADDR_DATA 0x0

#define SSD1963_WRITE_REG(u32RegAddr) \
        { \
            (*((volatile uint16_t *)(CONFIG_SSD1963_EBI_ADDR+(SSD1963_ADDR_CMD))) = (u32RegAddr)); \
            __DSB(); \
        }

#define SSD1963_WRITE_DATA(u32Data) \
        { \
            (*((volatile uint16_t *)(CONFIG_SSD1963_EBI_ADDR+(SSD1963_ADDR_DATA))) = (u32Data)); \
            __DSB(); \
        }

#define SSD1963_READ_DATA(u32Data) *((volatile uint16_t *)(CONFIG_SSD1963_EBI_ADDR+(SSD1963_ADDR_DATA)))

void ssd1963_send_cmd(uint8_t cmd)
{
    CLR_RS;
    SSD1963_WRITE_REG(cmd);
    SET_RS;
}

void ssd1963_send_cmd_parameter(uint8_t data)
{
    SSD1963_WRITE_DATA(data);
}

void ssd1963_send_pixel_data(uint16_t color)
{
    SSD1963_WRITE_DATA(color);
}

void ssd1963_send_pixels(uint16_t *pixels, int len)
{
    int count = len / sizeof(uint16_t);

#if defined(CONFIG_FSA506_EBI_USE_PDMA)
    if (count < 1024)
    {
        // CPU feed
        int i = 0;
        while (i < count)
        {
            SSD1963_WRITE_DATA(pixels[i]);
            i++;
        }
    }
    else
#endif
    {
        // PDMA-M2M feed
        nu_pdma_mempush((void *)(CONFIG_SSD1963_EBI_ADDR + (SSD1963_ADDR_DATA)), (void *)pixels, 16, count);
    }
}

void ssd1963_set_column(uint16_t StartCol, uint16_t EndCol)
{
    ssd1963_send_cmd(0x2A);
    SSD1963_WRITE_DATA((StartCol >> 8) & 0xFF);
    SSD1963_WRITE_DATA(StartCol & 0xFF);
    SSD1963_WRITE_DATA((EndCol >> 8) & 0xFF);
    SSD1963_WRITE_DATA(EndCol & 0xFF);
}

void ssd1963_set_page(uint16_t StartPage, uint16_t EndPage)
{
    ssd1963_send_cmd(0x2B);
    SSD1963_WRITE_DATA((StartPage >> 8) & 0xFF);
    SSD1963_WRITE_DATA(StartPage & 0xFF);
    SSD1963_WRITE_DATA((EndPage >> 8) & 0xFF);
    SSD1963_WRITE_DATA(EndPage & 0xFF);
}

void ssd1963_lcd_get_pixel(uint8_t *color, int x, int y)
{
    uint16_t red = 0;
    uint16_t green = 0;
    uint16_t blue = 0;

    ssd1963_set_column(x, x);
    ssd1963_set_page(y, y);
    ssd1963_send_cmd(0x2E);

    SSD1963_READ_DATA();           // Dummy
    red = SSD1963_READ_DATA();     // Red[4:0]@D15~D11
    blue = SSD1963_READ_DATA();    // Blue[4:0]@D15~D11
    green = SSD1963_READ_DATA();   // Green[5:0]@D15~D10

    //BGR565: B in High byte, R in low byte
    //*(uint16_t *)color = (((blue >> 11) << 11) | ((green >> 10) << 5) | (red >> 11));

    //RGB565: R in High byte, B in low byte
    *(uint16_t *)color = (((red >> 11) << 11) | ((green >> 10) << 5) | (blue >> 11));
}
