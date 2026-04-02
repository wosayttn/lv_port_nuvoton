/**************************************************************************//**
 * @file     ili9341_spi.c
 * @brief    ili9431 spi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
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

/**
 * @brief Write command byte to ILI9341 via SPI interface.
 *
 * Sets data/command line low, transfers 8-bit command via SPI, then sets
 * data/command line high for data phase.
 *
 * @param u8Cmd[in]  Command byte to send
 */
void DISP_WRITE_REG(uint8_t u8Cmd)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 8);

    /* Pull RS line low to indicate command transfer */
    DISP_CLR_RS;
    nu_spi_transfer(&s_NuSPI, (const void *)&u8Cmd, NULL, 1);
    /* Pull RS line high for subsequent data transfers */
    DISP_SET_RS;
}

/**
 * @brief Write data byte to ILI9341 via SPI interface.
 *
 * Transfers 8-bit data to the display controller via SPI.
 *
 * @param u8Dat[in]  Data byte to send
 */
void DISP_WRITE_DATA(uint8_t u8Dat)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 8);

    nu_spi_transfer(&s_NuSPI, (const void *)&u8Dat, NULL, 1);
}

/**
 * @brief Write 16-bit data to ILI9341 via SPI interface.
 *
 * Transfers 16-bit data to the display controller using 16-bit SPI mode.
 *
 * @param u16Dat[in]  16-bit data word to send
 */
static void DISP_WRITE_DATA_2B(uint16_t u16Dat)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_spi_transfer(&s_NuSPI, (const void *)&u16Dat, NULL, 2);
}

/**
 * @brief Set column address window for ILI9341 via SPI interface.
 *
 * Configures the horizontal address window using 16-bit data writes.
 *
 * @param StartCol[in]  Starting column address
 * @param EndCol[in]    Ending column address
 */
void disp_set_column(uint16_t StartCol, uint16_t EndCol)
{
    DISP_WRITE_REG(0x2A);
    DISP_WRITE_DATA_2B(StartCol);
    DISP_WRITE_DATA_2B(EndCol);
}

/**
 * @brief Set row/page address window for ILI9341 via SPI interface.
 *
 * Configures the vertical address window using 16-bit data writes.
 *
 * @param StartPage[in]  Starting row address
 * @param EndPage[in]    Ending row address
 */
void disp_set_page(uint16_t StartPage, uint16_t EndPage)
{
    DISP_WRITE_REG(0x2B);
    DISP_WRITE_DATA_2B(StartPage);
    DISP_WRITE_DATA_2B(EndPage);
}

/**
 * @brief Send pixel data to ILI9341 via SPI interface.
 *
 * Transfers pixel color data using 16-bit SPI transfers.
 *
 * @param pixels[in]    Pointer to pixel data array (16-bit RGB565)
 * @param byte_len[in]  Total length of data in bytes
 */
void disp_send_pixels(uint16_t *pixels, int byte_len)
{
    SPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_spi_transfer(&s_NuSPI, (const void *)pixels, NULL, byte_len);
}

static int ili9341_spi_send_then_recv(struct nu_spi *psNuSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw)
{
    SPI_SET_DATA_WIDTH(psNuSPI->base, dw * 8);

    if (psNuSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuSPI->ss_pin), NU_GET_PIN(psNuSPI->ss_pin)) = 0;
    }
    else
    {
        SPI_SET_SS_LOW(psNuSPI->base);
    }

    if (tx)
    {
        DISP_CLR_RS;
        int sent = 0;
        while (sent < tx_len)
        {
            sent += nu_spi_write(psNuSPI->base, tx + sent, dw);
        }
        while (SPI_IS_BUSY(psNuSPI->base));
    }

    DISP_SET_RS;

    /* Clear SPI RX FIFO */
    nu_spi_drain_rxfifo(psNuSPI->base);

    if (rx)
    {
        uint32_t dummy_tx = 0xFFFFFFFF;
        uint8_t *curr_rx = rx;
        int remaining = rx_len;
        int received = 0;

        while (received < rx_len)
        {
            if (remaining > 0)
            {
                int w = nu_spi_write(psNuSPI->base, (uint8_t *)&dummy_tx, dw);
                remaining -= w;
            }

            received += nu_spi_read(psNuSPI->base, curr_rx + received, dw);
        }
    }

    if (psNuSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuSPI->ss_pin), NU_GET_PIN(psNuSPI->ss_pin)) = 1;
    }
    else
    {
        SPI_SET_SS_HIGH(psNuSPI->base);
    }

    return 0;
}

typedef union
{
    uint32_t rgbx;
    struct
    {
        uint8_t x;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } S;
} ili9341_color;


static disp_area_t s_receive_area = {0};
void disp_ili9341_set_area(const disp_area_t *area)
{
    s_receive_area = *area;
}

void disp_receive_pixels(uint16_t *pixels, int byte_len)
{
    uint8_t cmd = 0x2E;

    int32_t w = (int32_t)(s_receive_area.x2 - s_receive_area.x1 + 1);
    int32_t h = (int32_t)(s_receive_area.y2 - s_receive_area.y1 + 1);
    int32_t x = (int32_t)(s_receive_area.x1);
    int32_t y = (int32_t)(s_receive_area.y1);

    disp_set_column(s_receive_area.x1, s_receive_area.x2);
    disp_set_page(s_receive_area.y1, s_receive_area.y2);

    // Slow down SPI clock frequency when reading.
    SPI_SetBusClock(s_NuSPI.base, CONFIG_DISP_SPI_CLOCK / 2);

    for (int j = y; j < y + h; j++)
    {
        for (int i = x; i < x + w; i++)
        {
            ili9341_color bgrx = {0};

            disp_set_column(i, i);
            disp_set_page(j, j);
            ili9341_spi_send_then_recv(&s_NuSPI, &cmd, 1, (void *)&bgrx, sizeof(bgrx), 1);

            *pixels = ((bgrx.S.r >> 3) << 11) | ((bgrx.S.g >> 2) << 5) | (bgrx.S.b >> 3);
            pixels++;
            //printf("x:%02x r:%02x g:%02x b:%02x\n", bgrx.S.x, bgrx.S.r, bgrx.S.g, bgrx.S.b);
            //printf("[%d] %08x -> %04x\n", i, bgrx.rgbx, pixels[i]);
        }
    }

    // Reset SPI clock frequency to default.
    SPI_SetBusClock(s_NuSPI.base, CONFIG_DISP_SPI_CLOCK);
}
