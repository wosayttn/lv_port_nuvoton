/**************************************************************************//**
 * @file     ili9341_uspi.c
 * @brief    ili9431 uspi interface glue
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "numaker_disp.h"
#include "drv_uspi.h"

static struct nu_uspi s_NuUSPI =
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
    USPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 8);

    DISP_CLR_RS;
    nu_uspi_transfer(&s_NuUSPI, (const void *)&u8Cmd, NULL, 1);
    DISP_SET_RS;
}

void DISP_WRITE_DATA(uint8_t u8Dat)
{
    nu_uspi_transfer(&s_NuUSPI, (const void *)&u8Dat, NULL, 1);
}

static void DISP_WRITE_DATA_2B(uint16_t u16Dat)
{
    USPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);

    nu_uspi_transfer(&s_NuUSPI, (const void *)&u16Dat, NULL, 2);
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
    USPI_SET_DATA_WIDTH(CONFIG_DISP_SPI, 16);
    nu_uspi_transfer(&s_NuUSPI, (const void *)pixels, NULL, byte_len);
}

static int ili9341_uspi_send_then_recv(struct nu_uspi *psNuUSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw)
{
    USPI_SET_DATA_WIDTH(psNuUSPI->base, dw * 8);

    if (psNuUSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuUSPI->ss_pin), NU_GET_PIN(psNuUSPI->ss_pin)) = 0;
    }
    else
    {
        USPI_SET_SS_LOW(psNuUSPI->base);
    }

    if (tx)
    {
        DISP_CLR_RS;
        int sent = 0;
        while (sent < tx_len)
        {
            sent += nu_uspi_write(psNuUSPI->base, tx + sent, dw);
        }
        while (USPI_IS_BUSY(psNuUSPI->base));
    }

    DISP_SET_RS;

    /* Clear USPI RX FIFO */
    nu_uspi_drain_rxfifo(psNuUSPI->base);

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
                int w = nu_uspi_write(psNuUSPI->base, (uint8_t *)&dummy_tx, dw);
                remaining -= w;
            }

            received += nu_uspi_read(psNuUSPI->base, curr_rx + received, dw);
        }
    }

    if (psNuUSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuUSPI->ss_pin), NU_GET_PIN(psNuUSPI->ss_pin)) = 1;
    }
    else
    {
        USPI_SET_SS_HIGH(psNuUSPI->base);
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
    USPI_SetBusClock(s_NuUSPI.base, CONFIG_DISP_SPI_CLOCK / 2);

    for (int j = y; j < y + h; j++)
    {
        for (int i = x; i < x + w; i++)
        {
            ili9341_color bgrx = {0};

            disp_set_column(i, i);
            disp_set_page(j, j);
            ili9341_uspi_send_then_recv(&s_NuUSPI, &cmd, 1, (void *)&bgrx, sizeof(bgrx), 1);

            *pixels = ((bgrx.S.r >> 3) << 11) | ((bgrx.S.g >> 2) << 5) | (bgrx.S.b >> 3);
            pixels++;
            //printf("x:%02x r:%02x g:%02x b:%02x\n", bgrx.S.x, bgrx.S.r, bgrx.S.g, bgrx.S.b);
            //printf("[%d] %08x -> %04x\n", i, bgrx.rgbx, pixels[i]);
        }
    }

    // Reset SPI clock frequency to default.
    USPI_SetBusClock(s_NuUSPI.base, CONFIG_DISP_SPI_CLOCK);
}
