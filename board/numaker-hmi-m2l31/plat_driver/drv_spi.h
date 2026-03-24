/**************************************************************************//**
 * @file     drv_pdma.c
 * @brief    PDMA high level driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DRV_SPI_H__
#define __DRV_SPI_H__

#include "disp_glue.h"

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#if defined(CONFIG_SPI_USE_PDMA)
    #include "drv_pdma.h"
    #define CONFIG_SPI_USE_PDMA_MIN_THRESHOLD (128)
#endif

struct nu_spi
{
    SPI_T *base;
    int32_t  ss_pin;
    uint32_t dummy;
#if defined(CONFIG_SPI_USE_PDMA)
    int16_t pdma_perp_tx;
    int8_t  pdma_chanid_tx;
    int16_t pdma_perp_rx;
    int8_t  pdma_chanid_rx;
#if defined(__FREERTOS__)
    SemaphoreHandle_t m_psSemBus;
#else
    volatile uint32_t m_psSemBus;
#endif

#endif
};
typedef struct nu_spi *nu_spi_t;

#define SPI_ASSERT(expr)             \
    do {                             \
        if(!(expr)) {                \
            while(1);                \
        }                            \
    } while(0)

#define SPI_GET_DATA_WIDTH(spi)  (((spi)->CTL & SPI_CTL_DWIDTH_Msk) >> SPI_CTL_DWIDTH_Pos)

__STATIC_INLINE int nu_spi_read(SPI_T *spi, uint8_t *rx, int dw)
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
            SPI_ASSERT(0);
        }
    }
    else
        return 0;

    return dw;
}

__STATIC_INLINE int nu_spi_write(SPI_T *spi, const uint8_t *tx, int dw)
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
        SPI_ASSERT(0);
    }

    return dw;
}

__STATIC_INLINE void nu_spi_drain_rxfifo(SPI_T *spi)
{
    while (SPI_IS_BUSY(spi));

    // Drain SPI RX FIFO, make sure RX FIFO is empty
    while (!SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
    {
        SPI_ClearRxFIFO(spi);
    }
}

int nu_spi_transfer(struct nu_spi *psNuSPI, const void *tx, void *rx, int length);
int nu_spi_send_then_recv(struct nu_spi *psNuSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw);

#endif //__DRV_SPI_H__
