/**************************************************************************//**
 * @file     ili9341_spi.c
 * @brief    ili9431 spi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ili9341.h"

#define DEF_SPI_USE_PDMA
#define DEF_SPI_USE_PDMA_MIN_THRESHOLD (128)

#define SPI_GET_DATA_WIDTH(spi)  (((spi)->CTL & SPI_CTL_DWIDTH_Msk) >> SPI_CTL_DWIDTH_Pos)
static uint32_t s_u32Dummy = 0;

static int nu_spi_read(SPI_T *spi, uint8_t *rx, int dw)
{
    // Read RX data
    if (!SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
    {
        uint32_t val;
        // Read data from SPI RX FIFO
        switch (dw)
        {
        case 4:
            val = SPI_READ_RX(spi);
            nu_set32_le(rx, val);
            break;
        case 3:
            val = SPI_READ_RX(spi);
            nu_set24_le(rx, val);
            break;
        case 2:
            val = SPI_READ_RX(spi);
            nu_set16_le(rx, val);
            break;
        case 1:
            *rx = SPI_READ_RX(spi);
            break;
        default:
            LV_LOG_ERROR("Data length is not supported.");
            LV_ASSERT(0);
        }
    }

    return dw;
}

static int nu_spi_write(SPI_T *spi, const uint8_t *tx, int dw)
{
    // Wait SPI TX send data
    while (SPI_GET_TX_FIFO_FULL_FLAG(spi));

    // Input data to SPI TX
    switch (dw)
    {
    case 4:
        SPI_WRITE_TX(spi, nu_get32_le(tx));
        break;
    case 3:
        SPI_WRITE_TX(spi, nu_get24_le(tx));
        break;
    case 2:
        SPI_WRITE_TX(spi, nu_get16_le(tx));
        break;
    case 1:
        SPI_WRITE_TX(spi, *((uint8_t *)tx));
        break;
    default:
        LV_LOG_ERROR("Data length is not supported.");
        LV_ASSERT(0);
    }

    return dw;
}

void nu_spi_drain_rxfifo(SPI_T *spi)
{
    while (SPI_IS_BUSY(spi));

    // Drain SPI RX FIFO, make sure RX FIFO is empty
    while (!SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
    {
        SPI_ClearRxFIFO(spi);
    }
}

static int nu_spi_transmit_poll(SPI_T *spi, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    // Write-only
    if ((tx != NULL) && (rx == NULL))
    {
        while (length > 0)
        {
            tx += nu_spi_write(spi, tx, dw);
            length -= dw;
        }
    } // if (tx != NULL && rx == NULL)
    // Read-only
    else if ((tx == NULL) && (rx != NULL))
    {
        s_u32Dummy = 0;
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            length -= nu_spi_write(spi, (const uint8_t *)&s_u32Dummy, dw);

            /* Read data from RX FIFO */
            rx += nu_spi_read(spi, rx, dw);
        }
    } // else if (tx == NULL && rx != NULL)
    // Read&Write
    else
    {
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            tx += nu_spi_write(spi, tx, dw);
            length -= dw;

            /* Read data from RX FIFO */
            rx += nu_spi_read(spi, rx, dw);
        }
    } // else

    /* Wait RX or drain RX-FIFO */
    if (rx)
    {
        // Wait SPI transmission done
        while (SPI_IS_BUSY(spi))
        {
            while (!SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
            {
                rx += nu_spi_read(spi, rx, dw);
            }
        }

        while (!SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
        {
            rx += nu_spi_read(spi, rx, dw);
        }
    }
    else
    {
        /* Clear SPI RX FIFO */
        nu_spi_drain_rxfifo(spi);
    }

    return length;
}

#if defined(DEF_SPI_USE_PDMA)
static int nu_spi_transmit_pdma(SPI_T *spi, const void *tx, void *rx, int length, int dw)
{
    // TODO
    return nu_spi_transmit_poll(spi, tx, rx, length, dw);
}
#endif

static int nu_spi_transfer(SPI_T *spi, const void *tx, void *rx, int length)
{
    int ret;
    int dw = SPI_GET_DATA_WIDTH(spi) / 8;

#if defined(CONFIG_ILI9341_SPI_SS_PIN)
    SET_SS_ACTIVE_LOW;
#else
    SPI_SET_SS_LOW(spi);
#endif

#if defined(DEF_SPI_USE_PDMA)

    /* DMA transfer constrains */
    if (!((uint32_t)tx % dw) &&
            !((uint32_t)rx % dw) &&
            (dw != 3) &&
            (length >= DEF_SPI_USE_PDMA_MIN_THRESHOLD))
        ret = nu_spi_transmit_pdma(spi, tx, rx, length, dw);
    else
#endif
        ret = nu_spi_transmit_poll(spi, tx, rx, length, dw);

#if defined(CONFIG_ILI9341_SPI_SS_PIN)
    CLR_SS_ACTIVE_LOW;
#else
    SPI_SET_SS_HIGH(spi);
#endif

    return ret;
}

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
    nu_spi_transfer(CONFIG_ILI9341_SPI, (const void *)&cmd, NULL, 1);
    SET_RS;
}

void ili9341_send_cmd_parameter(uint8_t data)
{
    ili9341_change_datawidth(1);
    nu_spi_transfer(CONFIG_ILI9341_SPI, (const void *)&data, NULL, 1);
}

static void ili9341_write_data_16bit(uint16_t data)
{
    ili9341_change_datawidth(2);
    nu_spi_transfer(CONFIG_ILI9341_SPI, (const void *)&data, NULL, 2);
}

void ili9341_send_pixels(uint16_t *pixels, int byte_len)
{
    ili9341_change_datawidth(2);
    nu_spi_transfer(CONFIG_ILI9341_SPI, (const void *)pixels, NULL, byte_len);
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
