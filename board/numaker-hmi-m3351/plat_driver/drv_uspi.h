/**************************************************************************//**
 * @file     drv_uspi.c
 * @brief    USPI high level driver for M3351 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DRV_USPI_H__
#define __DRV_USPI_H__

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

struct nu_uspi
{
    USPI_T *base;
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
typedef struct nu_uspi *nu_uspi_t;

#define USPI_ASSERT(expr)             \
    do {                             \
        if(!(expr)) {                \
            while(1);                \
        }                            \
    } while(0)

#define USPI_GET_DATA_WIDTH(uspi)  ((((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos)==0?16:(((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos))

__STATIC_INLINE int nu_uspi_read(USPI_T *uspi, uint8_t *rx, int dw)
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
            USPI_ASSERT(0);
        }
    }

    return dw;
}

__STATIC_INLINE int nu_uspi_write(USPI_T *uspi, const uint8_t *tx, int dw)
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
        USPI_ASSERT(0);
    }

    return dw;
}

__STATIC_INLINE void nu_uspi_drain_rxfifo(USPI_T *uspi)
{
    while (USPI_IS_BUSY(uspi));

    // Drain SPI RX FIFO, make sure RX FIFO is empty
    while (!USPI_GET_RX_EMPTY_FLAG(uspi))
    {
        USPI_ClearRxBuf(uspi);
    }
}

int nu_uspi_transfer(struct nu_uspi *psNuUSPI, const void *tx, void *rx, int length);
int nu_uspi_send_then_recv(struct nu_uspi *psNuUSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw);

#endif //__DRV_USPI_H__
