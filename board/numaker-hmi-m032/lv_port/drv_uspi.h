/**************************************************************************//**
 * @file     drv_uspi.c
 * @brief    USPI high level driver for M031 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "lv_glue.h"

#ifndef __DRV_USPI_H__
#define __DRV_USPI_H__

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
    volatile uint32_t m_psSemBus;
#endif
};
typedef struct nu_uspi *nu_uspi_t;

int nu_uspi_transfer(struct nu_uspi *psNuUSPI, const void *tx, void *rx, int length);

#endif //__DRV_USPI_H__
