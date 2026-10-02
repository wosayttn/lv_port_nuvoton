/**************************************************************************//**
 * @file     disp_ili9341_spi.c
 * @brief    ILI9341 SPI Display Driver with Async PDMA support
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"
#include "drv_spi.h"

static struct nu_spi s_NuSPI =
{
    .base           = CONFIG_DISP_SPI,
#if defined(CONFIG_DISP_SPI_SS_PIN)
    .ss_pin         = CONFIG_DISP_SPI_SS_PIN,
#else
    .ss_pin         = -1,
#endif
#if defined(CONFIG_DISP_USE_PDMA)
    .pdma_perp_tx   = CONFIG_PDMA_SPI_TX,
    .pdma_chanid_tx = -1,
    .pdma_perp_rx   = CONFIG_PDMA_SPI_RX,
    .pdma_chanid_rx = -1,
    .m_psSemBus     = 0,
#endif
};

void DISP_WRITE_REG(uint8_t u8Cmd)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 8);

    DISP_CLR_RS;
    nu_spi_transfer(&s_NuSPI, (const void *)&u8Cmd, NULL, 1);
    DISP_SET_RS;
}

void DISP_WRITE_DATA(uint8_t u8Dat)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 8);

    nu_spi_transfer(&s_NuSPI, (const void *)&u8Dat, NULL, 1);
}

static void DISP_WRITE_DATA_2B(uint16_t u16Dat)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_spi_transfer(&s_NuSPI, (const void *)&u16Dat, NULL, 2);
}

void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    DISP_WRITE_REG(0x2A);
    DISP_WRITE_DATA_2B(StartCol);
    DISP_WRITE_DATA_2B(EndCol);
}

void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    DISP_WRITE_REG(0x2B);
    DISP_WRITE_DATA_2B(StartPage);
    DISP_WRITE_DATA_2B(EndPage);
}

void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_spi_transfer(&s_NuSPI, (const void *)pixels, NULL, byte_len);
}

void disp_send_pixels_async(uint16_t *pixels, int byte_len, void (*cb)(void *), void *pvUserData)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_spi_transfer_async(&s_NuSPI, (const void *)pixels, NULL, byte_len, cb, pvUserData);
}

int disp_init(void)
{
    /* Set reset pin high and turn off backlight initially */
    DISP_SET_RST;
    DISP_CLR_BACKLIGHT;

    /* ===== Hardware Reset Sequence ===== */
    DISP_SET_RST;
    disp_delay_ms(5);

    DISP_CLR_RST;
    disp_delay_ms(20);

    DISP_SET_RST;
    disp_delay_ms(40);

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

    if (DISP_HOR_RES_MAX == 240)
        DISP_WRITE_DATA(0x48); /* 240x320 */
    else
        DISP_WRITE_DATA(0xE8); /* 320x240 */

    DISP_WRITE_REG(0x3A);
    DISP_WRITE_DATA(0x55); /* RGB565 */

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

    /* ===== Gamma Correction ===== */
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

    DISP_WRITE_REG(0x11); /* Exit sleep */
    disp_delay_ms(120);

    DISP_WRITE_REG(0x29); /* Display ON */

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

void disp_fillrect_async(uint16_t *pixels, const disp_area_t *area, void (*cb)(void *), void *pvUserData)
{
    int32_t w = (int32_t)(area->x2 - area->x1 + 1);
    int32_t h = (int32_t)(area->y2 - area->y1 + 1);

    disp_set_column(area->x1, area->x2);
    disp_set_page(area->y1, area->y2);
    DISP_WRITE_REG(0x2C);

    disp_send_pixels_async(pixels, h * w * sizeof(uint16_t), cb, pvUserData);
}

void disp_readrect(uint16_t *pixels, const disp_area_t *area)
{
    /* Not supported on write-only SPI */
}
