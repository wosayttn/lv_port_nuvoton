/**************************************************************************//**
 * @file     drv_pdma.c
 * @brief    PDMA high level driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __DRV_SPI_H__
#define __DRV_SPI_H__

#include "lv_glue.h"

#if (LV_USE_OS==LV_OS_FREERTOS)
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
#if (LV_USE_OS==LV_OS_FREERTOS)
    SemaphoreHandle_t m_psSemBus;
#else
    volatile uint32_t m_psSemBus;
#endif

#endif
};
typedef struct nu_spi *nu_spi_t;

int nu_spi_transfer(struct nu_spi *psNuSPI, const void *tx, void *rx, int length);
int nu_spi_send_then_recv(SPI_T *spi, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw);

#endif //__DRV_SPI_H__
