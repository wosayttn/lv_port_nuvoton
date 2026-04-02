/**************************************************************************//**
 * @file     disp_fsa506.c
 * @brief    fsa506 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"


/**
 * @brief Initialize FSA506 display controller.
 *
 * Performs hardware reset and configures all necessary registers for FSA506
 * display controller operation including PLL, panel timing, memory addressing,
 * and output format settings. Finally enables backlight.
 *
 * @return 0 on success
 *
 * @note This initialization sequence must be called before any display operations
 */
int disp_init(void)
{
    /* Set reset pin high initially */
    DISP_SET_RST;
    /* Turn off backlight */
    DISP_CLR_BACKLIGHT;

    /* ===== Hardware Reset Sequence ===== */
    /* Pull reset pin low to initiate reset */
    DISP_CLR_RST;
    disp_delay_ms(100);    /* Wait 100ms during reset */

    /* Release reset pin to high */
    DISP_SET_RST;
    disp_delay_ms(500);    /* Wait 500ms for controller to stabilize */

    /* ===== PLL Configuration ===== */
    disp_write_reg(0x40, 0x12);   /* PLL control: 20~100MHz, 8mA output, fast slew rate */
    disp_write_reg(0x41, 0x05);   /* PLL pre-divider: 5 */
    disp_write_reg(0x42, 0x06);   /* PLL loop divider: 6 */

    /* ===== Panel Size Configuration ===== */
    disp_write_reg(0x08, (DISP_HOR_RES_MAX >> 8)); /* Panel X size - high byte */
    disp_write_reg(0x09, (DISP_HOR_RES_MAX));      /* Panel X size - low byte */

    /* ===== Memory Write Address Configuration ===== */
    disp_write_reg(0x0a, 0x00);   /* Memory write start address [17:16] */
    disp_write_reg(0x0b, 0x00);   /* Memory write start address [15:8] */
    disp_write_reg(0x0c, 0x00);   /* Memory write start address [7:0] */

    /* ===== Clock and Pixel Format Configuration ===== */
    disp_write_reg(0x10, 0x0D);   /* 20MHz clock, parallel panel, normal operation */
    disp_write_reg(0x11, 0x05);   /* RGB format */

    /* ===== TFT Output Timing Configuration (Horizontal Sync) ===== */
    disp_write_reg(0x12, 0x00);   /* Hsync start position - high byte */
    disp_write_reg(0x13, 0x00);   /* Hsync start position - low byte */
    disp_write_reg(0x14, (41 >> 8));        /* Hsync pulse width - high byte */
    disp_write_reg(0x15, (41));             /* Hsync pulse width - low byte */

    disp_write_reg(0x16, (43 >> 8));        /* DE pulse start position - high byte */
    disp_write_reg(0x17, (43));             /* DE pulse start position - low byte */
    disp_write_reg(0x18, (DISP_HOR_RES_MAX >> 8)); /* DE pulse width - high byte */
    disp_write_reg(0x19, (DISP_HOR_RES_MAX));     /* DE pulse width - low byte */
    disp_write_reg(0x1a, (525 >> 8));       /* Hsync total clocks - high byte */
    disp_write_reg(0x1b, (525));            /* Hsync total clocks - low byte */

    /* ===== TFT Output Timing Configuration (Vertical Sync) ===== */
    disp_write_reg(0x1c, 0x00);   /* Vsync start position - high byte */
    disp_write_reg(0x1d, 0x00);   /* Vsync start position - low byte */
    disp_write_reg(0x1e, (10 >> 8));        /* Vsync pulse width - high byte */
    disp_write_reg(0x1f, (10));             /* Vsync pulse width - low byte */
    disp_write_reg(0x20, (12 >> 8));        /* Vertical DE pulse start - high byte */
    disp_write_reg(0x21, (12));             /* Vertical DE pulse start - low byte */
    disp_write_reg(0x22, (DISP_VER_RES_MAX >> 8)); /* Vertical active width - high byte */
    disp_write_reg(0x23, (DISP_VER_RES_MAX));     /* Vertical active width - low byte */
    disp_write_reg(0x24, (286 >> 8));       /* Vertical total width - high byte */
    disp_write_reg(0x25, (286));            /* Vertical total width - low byte */

    /* ===== Memory Read Address Configuration ===== */
    disp_write_reg(0x26, 0x00);   /* Memory read start address [17:16] */
    disp_write_reg(0x27, 0x00);   /* Memory read start address [15:8] */
    disp_write_reg(0x28, 0x00);   /* Memory read start address [7:0] */
    disp_write_reg(0x29, 0x01);   /* Load timing settings to take effect */

    /* ===== Output Control and Rotation ===== */
    /* Bit[7:4]: Reserved, Bit[3]: Output pin control, Bit[2]: Clock inversion (inverted),
       Bit[1:0]: Image rotation (00=0°, 01=90°, 10=270°, 11=180°) */
    disp_write_reg(0x2d, (1 << 2) | 0x08);

    /* ===== Horizontal and Vertical Offset Configuration ===== */
    disp_write_reg(0x30, 0x00);   /* Horizontal offset - high byte */
    disp_write_reg(0x31, 0x00);   /* Horizontal offset - low byte */
    disp_write_reg(0x32, 0x00);   /* Vertical offset - high byte */
    disp_write_reg(0x33, 0x00);   /* Vertical offset - low byte */
    disp_write_reg(0x34, (DISP_HOR_RES_MAX >> 8));  /* Horizontal definition - high byte */
    disp_write_reg(0x35, (DISP_HOR_RES_MAX));       /* Horizontal definition - low byte */
    disp_write_reg(0x36, ((2 * DISP_VER_RES_MAX) >> 8)); /* Vertical resolution - high byte */
    disp_write_reg(0x37, (2 * DISP_VER_RES_MAX));   /* Vertical resolution - low byte */

    /* Enable backlight to display content */
    DISP_SET_BACKLIGHT;

    return 0;
}

/**
 * @brief Fill a rectangular display area with pixel data.
 *
 * Sets the column and page (row) address window and sends pixel data
 * to fill the specified rectangular region on the display.
 *
 * @param pixels[in]  Pointer to pixel color data (RGB565 format)
 * @param area[in]    Pointer to display area structure defining rectangle boundaries
 */
void disp_fillrect(uint16_t *pixels, const disp_area_t *area)
{
    /* Calculate rectangle dimensions */
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    /* Set column address window (horizontal) */
    disp_set_column(area->x1, area->x2);
    /* Set page/row address window (vertical) */
    disp_set_page(area->y1, area->y2);
    /* Send pixel data to fill the defined area */
    disp_send_pixels(pixels, h * w * sizeof(uint16_t));
}

void disp_readrect(uint16_t *pixels, const disp_area_t *area)
{
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    disp_set_column(area->x1, area->x2);
    disp_set_page(area->y1, area->y2);
    disp_receive_pixels(pixels, h * w * sizeof(uint16_t));
}
