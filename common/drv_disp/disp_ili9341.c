/**************************************************************************//**
 * @file     disp_ili9341.c
 * @brief    ili9431 driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/**
 * @brief Initialize ILI9341 display controller.
 *
 * Performs hardware reset and sends the complete initialization sequence for ILI9341.
 * Configures power settings, display timing, color format (RGB565), gamma correction,
 * and enables the display output with backlight.
 *
 * @return 0 on success
 */
int disp_init(void)
{
    /* Set reset pin high and turn off backlight initially */
    DISP_SET_RST;
    DISP_CLR_BACKLIGHT;

    /* ===== Hardware Reset Sequence ===== */
    DISP_SET_RST;
    disp_delay_ms(5);     /* Wait 5ms */

    DISP_CLR_RST;
    disp_delay_ms(20);    /* Wait 20ms for reset */

    DISP_SET_RST;
    disp_delay_ms(40);    /* Wait 40ms for stabilization */

    /* ===== Initial Control Registers ===== */
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

    /* ===== Power Control ===== */
    DISP_WRITE_REG(0xC0);
    DISP_WRITE_DATA(0x23);

    DISP_WRITE_REG(0xC1);
    DISP_WRITE_DATA(0x10);

    DISP_WRITE_REG(0xC5);
    DISP_WRITE_DATA(0x3e);
    DISP_WRITE_DATA(0x28);

    DISP_WRITE_REG(0xC7);
    DISP_WRITE_DATA(0x86);

    /* ===== Display Configuration ===== */
    DISP_WRITE_REG(0x36);

    /* Set display orientation based on resolution */
    if (DISP_HOR_RES_MAX == 240)
        DISP_WRITE_DATA(0x48); /* Orientation for 240x320 */
    else
        DISP_WRITE_DATA(0xE8); /* Orientation for 320x240 */

    /* Set pixel format to RGB565 (16-bit color) */
    DISP_WRITE_REG(0x3A);
    DISP_WRITE_DATA(0x55);

    /* Frame rate control */
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

    /* ===== Gamma Correction (Positive) ===== */
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

    /* ===== Gamma Correction (Negative) ===== */
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

    /* Exit sleep mode */
    DISP_WRITE_REG(0x11);
    disp_delay_ms(120);   /* Wait 120ms for display to wake up */

    /* Turn display on */
    DISP_WRITE_REG(0x29);

    /* Enable backlight */
    DISP_SET_BACKLIGHT;

    return 0;
}

/**
 * @brief Fill a rectangular display area with pixel data (ILI9341).
 *
 * Sets the column and page address window and sends pixel data.
 *
 * @param pixels[in]  Pointer to pixel data buffer (RGB565 format)
 * @param area[in]    Pointer to rectangular area structure
 */
void disp_fillrect(uint16_t *pixels, const disp_area_t *area)
{
    /* Calculate width and height */
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    /* Set column address */
    disp_set_column(area->x1, area->x2);
    /* Set page/row address */
    disp_set_page(area->y1, area->y2);
    /* Write memory command */
    DISP_WRITE_REG(0x2C);

    /* Send pixel data */
    disp_send_pixels(pixels, h * w * sizeof(uint16_t));
}

/**
 * @brief Read a rectangular display area from VRAM (ILI9341).
 *
 * Sets the address window and reads pixel data from display memory.
 *
 * @param pixels[out]  Pointer to buffer to receive pixel data
 * @param area[in]     Pointer to rectangular area structure
 */
void disp_readrect(uint16_t *pixels, const disp_area_t *area)
{
    void disp_ili9341_set_area(const disp_area_t *area);
    /* Configure address window */
    disp_ili9341_set_area(area);

    /* Calculate dimensions */
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    /* Read pixel data from display memory */
    disp_receive_pixels(pixels, h * w * sizeof(uint16_t));
}
