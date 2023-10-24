/**************************************************************************//**
 * @file     ili9341_spi.c
 * @brief    ili9431 spi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ili9341.h"
#include "drv_spi.h"

static struct nu_spi s_NuSPI =
{
    .base           = CONFIG_ILI9341_SPI,
    .ss_pin         = -1,
#if defined(CONFIG_SPI_USE_PDMA)
    .pdma_perp_tx   = CONFIG_PDMA_SPI_TX,
    .pdma_chanid_tx = -1,
    .pdma_perp_rx   = CONFIG_PDMA_SPI_RX,
    .pdma_chanid_rx = -1,
    .m_psSemBus     = 0,
#endif
};

static void ili9341_change_datawidth(int dw)
{
    static uint32_t u32LastDW = 0;

    if (u32LastDW != dw)
    {
        SPI_SET_DATA_WIDTH(CONFIG_ILI9341_SPI, dw * 8);
        u32LastDW = dw;
    }
}

void ili9341_send_cmd(uint8_t cmd)
{
    ili9341_change_datawidth(1);
    CLR_RS;
    nu_spi_transfer(&s_NuSPI, (const void *)&cmd, NULL, 1);
    SET_RS;
}

void ili9341_send_cmd_parameter(uint8_t data)
{
    ili9341_change_datawidth(1);
    nu_spi_transfer(&s_NuSPI, (const void *)&data, NULL, 1);
}

static void ili9341_write_data_16bit(uint16_t data)
{
    ili9341_change_datawidth(2);
    nu_spi_transfer(&s_NuSPI, (const void *)&data, NULL, 2);
}

void ili9341_send_pixels(uint16_t *pixels, int byte_len)
{
    ili9341_change_datawidth(2);
    nu_spi_transfer(&s_NuSPI, (const void *)pixels, NULL, byte_len);
}

void ili9341_set_column(uint16_t StartCol, uint16_t EndCol)
{
    ili9341_send_cmd(0x2A);
    ili9341_write_data_16bit(StartCol);
    ili9341_write_data_16bit(EndCol);
}

void ili9341_set_page(uint16_t StartPage, uint16_t EndPage)
{
    ili9341_send_cmd(0x2B);
    ili9341_write_data_16bit(StartPage);
    ili9341_write_data_16bit(EndPage);
}
