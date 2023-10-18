/**************************************************************************//**
 * @file     ili9341_uspi.c
 * @brief    ili9431 uspi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp_ili9341.h"

#define DEF_USPI_USE_PDMA
#define DEF_USPI_USE_PDMA_MIN_THRESHOLD (128)

#define SET_SS_ACTIVE_LOW   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_USPI_SS_PIN), NU_GET_PIN(CONFIG_ILI9341_USPI_SS_PIN)) = 0
#define CLR_SS_ACTIVE_LOW   GPIO_PIN_DATA(NU_GET_PORT(CONFIG_ILI9341_USPI_SS_PIN), NU_GET_PIN(CONFIG_ILI9341_USPI_SS_PIN)) = 1

#define USPI_GET_DATA_WIDTH(uspi)  ((((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos)==0?16:(((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos))

static uint32_t s_u32Dummy = 0;

static int nu_uspi_read(USPI_T *uspi, uint8_t *rx, int dw)
{
    // Read RX data
    if (!USPI_GET_RX_EMPTY_FLAG(uspi))
    {
        uint32_t val;
        // Read data from SPI RX FIFO
        switch (dw)
        {
        case 2:
            val = USPI_READ_RX(uspi);
            nu_set16_le(rx, val);
            break;
        case 1:
            *rx = USPI_READ_RX(uspi);
            break;
        default:
            LV_LOG_ERROR("Data length is not supported.");
            LV_ASSERT(0);
        }
    }

    return dw;
}

static int nu_uspi_write(USPI_T *uspi, const uint8_t *tx, int dw)
{
    // Wait SPI TX send data
    while (USPI_GET_TX_FULL_FLAG(uspi));

    // Input data to USPI TX
    switch (dw)
    {
    case 2:
        USPI_WRITE_TX(uspi, nu_get16_le(tx));
        break;
    case 1:
        USPI_WRITE_TX(uspi, *((uint8_t *)tx));
        break;
    default:
        LV_LOG_ERROR("Data length is not supported.");
        LV_ASSERT(0);
    }

    return dw;
}

void nu_uspi_drain_rxfifo(USPI_T *uspi)
{
    while (USPI_IS_BUSY(uspi));

    // Drain SPI RX FIFO, make sure RX FIFO is empty
    while (!USPI_GET_RX_EMPTY_FLAG(uspi))
    {
        USPI_ClearRxBuf(uspi);
    }
}

static int nu_uspi_transmit_poll(USPI_T *uspi, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    // Write-only
    if ((tx != NULL) && (rx == NULL))
    {
        while (length > 0)
        {
            tx += nu_uspi_write(uspi, tx, dw);
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
            length -= nu_uspi_write(uspi, (const uint8_t *)&s_u32Dummy, dw);

            /* Read data from RX FIFO */
            while (USPI_GET_RX_EMPTY_FLAG(uspi));
            rx += nu_uspi_read(uspi, rx, dw);
        }
    } // else if (tx == NULL && rx != NULL)
    // Read&Write
    else
    {
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            tx += nu_uspi_write(uspi, tx, dw);
            length -= dw;

            /* Read data from RX FIFO */
            while (USPI_GET_RX_EMPTY_FLAG(uspi));
            rx += nu_uspi_read(uspi, rx, dw);
        }
    } // else

    /* Wait RX or drain RX-FIFO */
    if (rx)
    {
        // Wait SPI transmission done
        while (USPI_IS_BUSY(uspi))
        {
            while (!USPI_GET_RX_EMPTY_FLAG(uspi))
            {
                rx += nu_uspi_read(uspi, rx, dw);
            }
        }

        while (!USPI_GET_RX_EMPTY_FLAG(uspi))
        {
            rx += nu_uspi_read(uspi, rx, dw);
        }
    }
    else
    {
        /* Clear SPI RX FIFO */
        nu_uspi_drain_rxfifo(uspi);
    }

    return length;
}

#if defined(DEF_USPI_USE_PDMA)
static int nu_uspi_transmit_pdma(USPI_T *uspi, const void *tx, void *rx, int length, int dw)
{
    // TODO
    return nu_uspi_transmit_poll(uspi, tx, rx, length, dw);
}
#endif

static int nu_uspi_transfer(USPI_T *uspi, const void *tx, void *rx, int length)
{
    int ret;
    int dw = USPI_GET_DATA_WIDTH(uspi) / 8;

#if defined(CONFIG_ILI9341_USPI_SS_PIN)
    SET_SS_ACTIVE_LOW;
#else
    USPI_SET_SS_LOW(uspi);
#endif

#if defined(DEF_USPI_USE_PDMA)
    /* DMA transfer constrains */
    if (!((uint32_t)tx % dw) &&
            !((uint32_t)rx % dw) &&
            (length >= DEF_USPI_USE_PDMA_MIN_THRESHOLD))
        ret = nu_uspi_transmit_pdma(uspi, tx, rx, length, dw);
    else
#endif
        ret = nu_uspi_transmit_poll(uspi, tx, rx, length, dw);

#if defined(CONFIG_ILI9341_USPI_SS_PIN)
    CLR_SS_ACTIVE_LOW;
#else
    USPI_SET_SS_HIGH(uspi);
#endif

    return ret;
}

static void ili9341_change_datawidth(int dw)
{
    static uint32_t u32LastDW = 0;

    if (u32LastDW != dw)
    {
        USPI_SET_DATA_WIDTH(CONFIG_ILI9341_USPI, dw * 8);
        u32LastDW = dw;
    }
}

void ili9341_send_cmd(uint8_t cmd)
{
    ili9341_change_datawidth(1);
    CLR_RS;
    nu_uspi_transfer(CONFIG_ILI9341_USPI, (const void *)&cmd, NULL, 1);
    SET_RS;
}

void ili9341_send_cmd_parameter(uint8_t data)
{
    ili9341_change_datawidth(1);
    nu_uspi_transfer(CONFIG_ILI9341_USPI, (const void *)&data, NULL, 1);
}

static void ili9341_write_data_16bit(uint16_t data)
{
    ili9341_change_datawidth(2);
    nu_uspi_transfer(CONFIG_ILI9341_USPI, (const void *)&data, NULL, 2);
}

void ili9341_send_pixels(uint16_t *pixels, int byte_len)
{
    ili9341_change_datawidth(2);
    nu_uspi_transfer(CONFIG_ILI9341_USPI, (const void *)pixels, NULL, byte_len);
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
