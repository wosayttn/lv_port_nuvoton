/**************************************************************************//**
 * @file     drv_qspi.h
 * @brief    QSPI high level driver with PDMA support
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DRV_QSPI_H__
#define __DRV_QSPI_H__

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"
#include "nu_bitutil.h"
#include "nu_pin.h"

#if !defined(GPIO_PIN_DATA)
    #define GPIO_PIN_DATA GPIO_PIN_DATA_S
#endif

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#ifndef CONFIG_QSPI_USE_PDMA
    #define CONFIG_QSPI_USE_PDMA 1
#endif

#if defined(CONFIG_QSPI_USE_PDMA)
    #include "drv_pdma.h"
    #ifndef CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD
        #define CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD (128)
    #endif
#endif

struct nu_qspi
{
    QSPI_T   *base;
    int32_t   ss_pin;
    uint32_t  dummy;
#if defined(CONFIG_QSPI_USE_PDMA)
    int16_t   pdma_perp_tx;
    int8_t    pdma_chanid_tx;
    int16_t   pdma_perp_rx;
    int8_t    pdma_chanid_rx;
#if defined(__FREERTOS__)
    SemaphoreHandle_t m_psSemBus;
#endif
    volatile uint32_t m_u32Done;

    void (*pfnTransferDoneCb)(void *pvUserData);
    void *pvUserData;
    volatile uint8_t bAsyncBusy;
#endif
};
typedef struct nu_qspi *nu_qspi_t;

#define QSPI_ASSERT(expr)            \
    do {                             \
        if(!(expr)) {                \
            while(1);                \
        }                            \
    } while(0)

#define QSPI_GET_DATA_WIDTH(qspi)  (((qspi)->CTL & QSPI_CTL_DWIDTH_Msk) >> QSPI_CTL_DWIDTH_Pos)

__STATIC_INLINE int nu_qspi_read(QSPI_T *qspi, uint8_t *rx, int dw)
{
    if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(qspi))
    {
        uint32_t val;
        switch (dw)
        {
        case 4:
            val = QSPI_READ_RX(qspi);
            nu_set32_le(rx, val);
            break;
        case 3:
            val = QSPI_READ_RX(qspi);
            nu_set24_le(rx, val);
            break;
        case 2:
            val = QSPI_READ_RX(qspi);
            nu_set16_le(rx, val);
            break;
        case 1:
            *rx = (uint8_t)QSPI_READ_RX(qspi);
            break;
        default:
            QSPI_ASSERT(0);
        }
    }
    else
        return 0;

    return dw;
}

__STATIC_INLINE int nu_qspi_write(QSPI_T *qspi, const uint8_t *tx, int dw)
{
    while (QSPI_GET_TX_FIFO_FULL_FLAG(qspi));

    switch (dw)
    {
    case 4:
        QSPI_WRITE_TX(qspi, nu_get32_le(tx));
        break;
    case 3:
        QSPI_WRITE_TX(qspi, nu_get24_le(tx));
        break;
    case 2:
        QSPI_WRITE_TX(qspi, nu_get16_le(tx));
        break;
    case 1:
        QSPI_WRITE_TX(qspi, *((const uint8_t *)tx));
        break;
    default:
        QSPI_ASSERT(0);
    }

    return dw;
}

__STATIC_INLINE void nu_qspi_drain_rxfifo(QSPI_T *qspi)
{
    while (QSPI_IS_BUSY(qspi));

    while (!QSPI_GET_RX_FIFO_EMPTY_FLAG(qspi))
    {
        QSPI_ClearRxFIFO(qspi);
    }
}

int  nu_qspi_init_pdma(struct nu_qspi *psNuQSPI);
int  nu_qspi_transfer(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length);
int  nu_qspi_transfer_async(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length, void (*cb)(void *), void *pvUserData);
void nu_qspi_transfer_wait(struct nu_qspi *psNuQSPI);
int  nu_qspi_send_then_recv(struct nu_qspi *psNuQSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw);
int  nu_qspi_transmit_pdma(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length, int dw);
int  nu_qspi_transmit_poll(struct nu_qspi *psNuQSPI, const uint8_t *tx, uint8_t *rx, int length, int dw);
int  nu_qspi_read_pdma(struct nu_qspi *psNuQSPI, void *rx, int length);
int  nu_qspi_write_pdma(struct nu_qspi *psNuQSPI, const void *tx, int length);

#endif /* __DRV_QSPI_H__ */
