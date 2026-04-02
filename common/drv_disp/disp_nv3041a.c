/**************************************************************************//**
 * @file     disp_nv3041a.c
 * @brief    nv3041a driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"

/**
 * @brief Initialize NV3041A display controller.
 *
 * Performs hardware reset sequence, unlocks extended commands, and configures
 * all critical registers for:
 * - Tearing effect signal (TE) output
 * - Memory access control and pixel format (RGB565)
 * - Power supply voltage trim and clamp control
 * - Gate and source timing for panel scanning
 * - Gamma correction curves (positive and negative)
 * - Display window configuration
 *
 * @return  0 on success
 */
int disp_init(void)
{
    DISP_SET_RST;
    DISP_CLR_BACKLIGHT;

    /* Perform hardware reset: pull low then high with delays */
    DISP_CLR_RST;
    disp_delay_ms(100);    /* Hold reset low for 100ms */

    DISP_SET_RST;
    disp_delay_ms(500);    /* Wait 500ms for initialization */

    /* Unlock extended command set for configuration */
    disp_write_reg(0xFF, 0xA5);

    /* Configure tearing effect signal output and display control */
    disp_write_reg(0xE7, 0x10); /* TE output enable */
    disp_write_reg(0x35, 0x00); /* Tearing Effect Line signal enable */
    disp_write_reg(0x36, 0x00); /* Memory access control: scan direction, RGB order */
    disp_write_reg(0x3A, 0x01); /* Pixel format: 16-bit RGB565 */
    disp_write_reg(0x41, 0x03); /* Bus width: 16-bit interface */
    disp_write_reg(0x44, 0x15); /* Vertical back porch */
    disp_write_reg(0x45, 0x15); /* Vertical front porch */
    disp_write_reg(0x7D, 0x03); /* Internal power supply voltage trim */

    /* Configure power supply voltages and clamp levels */
    disp_write_reg(0xC1, 0xBB); /* AVDD/AVCL clamp control */
    disp_write_reg(0xC2, 0x05); /* VGH clamp control */
    disp_write_reg(0xC3, 0x10); /* VGL clamp control */
    disp_write_reg(0xC6, 0x3E); /* Power voltage ratio control */
    disp_write_reg(0xC7, 0x25); /* Charge pump clock control */
    disp_write_reg(0xC8, 0x11); /* VGL clock control */
    disp_write_reg(0x7A, 0x5F); /* User VGSP voltage setting */
    disp_write_reg(0x6F, 0x44); /* User GVDD voltage setting */
    disp_write_reg(0x78, 0x70); /* User GVCL voltage setting */
    disp_write_reg(0xC9, 0x00); /* MV clock clamp control */
    disp_write_reg(0x67, 0x21); /* Internal tuning */

    /* Configure gate scan timing (for odd and even frames) */
    disp_write_reg(0x51, 0x0A); /* Gate start (odd frame) */
    disp_write_reg(0x52, 0x76); /* Gate end (odd frame) */
    disp_write_reg(0x53, 0x0A); /* Gate start (even frame) */
    disp_write_reg(0x54, 0x76); /* Gate end (even frame) */

    /* Configure source/column timing */
    disp_write_reg(0x46, 0x0A); /* Horizontal back porch (odd) */
    disp_write_reg(0x47, 0x2A); /* Horizontal front porch (odd) */
    disp_write_reg(0x48, 0x0A); /* Horizontal back porch (even) */
    disp_write_reg(0x49, 0x1A); /* Horizontal front porch (even) */

    /* Configure data line and charge pump timing */
    disp_write_reg(0x56, 0x43); /* LD (Data Line) width/start */
    disp_write_reg(0x57, 0x42); /* CS (Column Select) enable/start */
    disp_write_reg(0x58, 0x3C); /* CS positive width */
    disp_write_reg(0x59, 0x64); /* CS negative width */

    /* Configure precharge timing (for odd and even frames) */
    disp_write_reg(0x5A, 0x41); /* Precharge start (odd frame) */
    disp_write_reg(0x5B, 0x3C); /* Precharge pulse width (odd) */
    disp_write_reg(0x5C, 0x02); /* Precharge start (even frame) */
    disp_write_reg(0x5D, 0x3C); /* Precharge pulse width (even) */
    disp_write_reg(0x5E, 0x1F); /* Polarity switch control */

    /* Configure output window timing */
    disp_write_reg(0x60, 0x80); /* Output start (odd frame) */
    disp_write_reg(0x61, 0x3F); /* Output start (even frame) */
    disp_write_reg(0x62, 0x21); /* Output end (MSB) */
    disp_write_reg(0x63, 0x07); /* Output end (odd frame LSB) */
    disp_write_reg(0x64, 0xE0); /* Output end (even frame LSB) */
    disp_write_reg(0x65, 0x02); /* Chopper control */

    /* MUX timing control for power supply switching (AVDD/AVCL/VGH) */
    disp_write_reg(0xCA, 0x20);
    disp_write_reg(0xCB, 0x52);
    disp_write_reg(0xCC, 0x10);
    disp_write_reg(0xCD, 0x42);
    disp_write_reg(0xD0, 0x20);
    disp_write_reg(0xD1, 0x52);
    disp_write_reg(0xD2, 0x10);
    disp_write_reg(0xD3, 0x42);
    disp_write_reg(0xD4, 0x0A);
    disp_write_reg(0xD5, 0x32);

    /* Test mode configuration */
    disp_write_reg(0xF8, 0x03);
    disp_write_reg(0xF9, 0x20);

    /* Configure gamma correction curves for positive and negative half-tones */
    /* Positive gamma curve (first voltage dividing point) */
    disp_write_reg(0x80, 0x00);
    disp_write_reg(0xA0, 0x00);
    /* Positive gamma curve (second voltage dividing point) */
    disp_write_reg(0x81, 0x05);
    disp_write_reg(0xA1, 0x05);
    /* Positive gamma curve (third voltage dividing point) */
    disp_write_reg(0x82, 0x04);
    disp_write_reg(0xA2, 0x03);
    /* Positive gamma curve (middle point) */
    disp_write_reg(0x86, 0x25);
    disp_write_reg(0xA6, 0x1C);
    /* Positive gamma curve (additional points) */
    disp_write_reg(0x87, 0x2A);
    disp_write_reg(0xA7, 0x2A);
    disp_write_reg(0x83, 0x1D);
    disp_write_reg(0xA3, 0x1D);
    disp_write_reg(0x84, 0x1E);
    disp_write_reg(0xA4, 0x1E);
    /* Positive gamma curve (final point) */
    disp_write_reg(0x85, 0x3F);
    disp_write_reg(0xA5, 0x3F);
    /* Positive gamma curve (fine-tuning) */
    disp_write_reg(0x88, 0x0B);
    disp_write_reg(0xA8, 0x0B);
    disp_write_reg(0x89, 0x14);
    disp_write_reg(0xA9, 0x13);
    disp_write_reg(0x8A, 0x1A);
    disp_write_reg(0xAA, 0x1A);
    disp_write_reg(0x8B, 0x0A);
    disp_write_reg(0xAB, 0x0A);
    disp_write_reg(0x8C, 0x1C);
    disp_write_reg(0xAC, 0x0C);
    disp_write_reg(0x8D, 0x1F);
    disp_write_reg(0xAD, 0x0B);
    disp_write_reg(0x8E, 0x1F);
    disp_write_reg(0xAE, 0x0A);
    disp_write_reg(0x8F, 0x1F);
    disp_write_reg(0xAF, 0x07);
    disp_write_reg(0x90, 0x06);
    disp_write_reg(0xB0, 0x06);
    disp_write_reg(0x91, 0x0D);
    disp_write_reg(0xB1, 0x0D);
    disp_write_reg(0x92, 0x17);
    disp_write_reg(0xB2, 0x17);

    disp_write_reg(0xFF, 0x00); // Lock command set

    DISP_WRITE_REG(0x11); // Exit Sleep Mode
    disp_delay_ms(100);

    DISP_WRITE_REG(0x29); // Display ON
    disp_delay_ms(100);

    DISP_SET_BACKLIGHT;

    return 0;
}

void disp_fillrect(uint16_t *pixels, const disp_area_t *area)
{
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    disp_set_column(area->x1, area->x2);
    disp_set_page(area->y1, area->y2);
    DISP_WRITE_REG(0x2C);

    disp_send_pixels(pixels, h * w * sizeof(uint16_t));
}

void disp_readrect(uint16_t *pixels, const disp_area_t *area)
{
    uint16_t u16SysID[2] = {0};

    DISP_WRITE_REG(0x4);
    u16SysID[0] = DISP_READ_DATA();
    DISP_READ_DATA();
    u16SysID[1] = DISP_READ_DATA();
    DISP_READ_DATA();

    printf("[%s]SysID: %02X%02X -- Not supported\n", __func__, u16SysID[0], u16SysID[1]);
}
