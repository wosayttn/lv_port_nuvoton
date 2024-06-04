/**************************************************************************//**
 * @file     drv_pdma.c
 * @brief    PDMA high level driver for M460 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "drv_uspi.h"
#define USPI_GET_DATA_WIDTH(uspi)  ((((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos)==0?16:(((uspi)->LINECTL & USPI_LINECTL_DWIDTH_Msk) >> USPI_LINECTL_DWIDTH_Pos))
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

static int nu_uspi_transmit_poll(struct nu_uspi *psNuUSPI, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    USPI_T *base = psNuUSPI->base;
    // Write-only
    if ((tx != NULL) && (rx == NULL))
    {
        while (length > 0)
        {
            tx += nu_uspi_write(base, tx, dw);
            length -= dw;
        }
    } // if (tx != NULL && rx == NULL)
    // Read-only
    else if ((tx == NULL) && (rx != NULL))
    {
        psNuUSPI->dummy = 0;
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            length -= nu_uspi_write(base, (const uint8_t *)&psNuUSPI->dummy, dw);

            /* Read data from RX FIFO */
            while (USPI_GET_RX_EMPTY_FLAG(base));
            rx += nu_uspi_read(base, rx, dw);
        }
    } // else if (tx == NULL && rx != NULL)
    // Read&Write
    else
    {
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            tx += nu_uspi_write(base, tx, dw);
            length -= dw;

            /* Read data from RX FIFO */
            while (USPI_GET_RX_EMPTY_FLAG(base));
            rx += nu_uspi_read(base, rx, dw);
        }
    } // else

    /* Wait RX or drain RX-FIFO */
    if (rx)
    {
        // Wait SPI transmission done
        while (USPI_IS_BUSY(base))
        {
            while (!USPI_GET_RX_EMPTY_FLAG(base))
            {
                rx += nu_uspi_read(base, rx, dw);
            }
        }

        while (!USPI_GET_RX_EMPTY_FLAG(base))
        {
            rx += nu_uspi_read(base, rx, dw);
        }
    }
    else
    {
        /* Clear SPI RX FIFO */
        nu_uspi_drain_rxfifo(base);
    }

    return length;
}

#if defined(CONFIG_SPI_USE_PDMA)

static void nu_pdma_uspi_rx_cb_event(void *pvUserData, uint32_t u32EventFilter)
{
    struct nu_uspi *psNuUSPI = (struct nu_uspi *)pvUserData;

    LV_ASSERT(psNuUSPI);

    psNuUSPI->m_psSemBus = 1;
}

static void nu_pdma_uspi_tx_cb_trigger(void *pvUserData, uint32_t u32UserData)
{
    /* Get base address of spi register */
    USPI_T *base = (USPI_T *)pvUserData;

    /* Trigger TX/RX PDMA transfer. */
    USPI_TRIGGER_TX_RX_PDMA(base);
}

static void nu_pdma_uspi_rx_cb_disable(void *pvUserData, uint32_t u32UserData)
{
    /* Get base address of spi register */
    USPI_T *base = (USPI_T *)pvUserData;

    /* Stop TX/RX DMA transfer. */
    USPI_DISABLE_TX_RX_PDMA(base);
}

static int nu_pdma_uspi_rx_config(struct nu_uspi *psNuUSPI, uint8_t *pu8Buf, int32_t i32RcvLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;

    int result;
    uint8_t *dst_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    /* Get base address of spi register */
    USPI_T *base = psNuUSPI->base;

    uint8_t spi_pdma_rx_chid = psNuUSPI->pdma_chanid_rx;

    nu_pdma_filtering_set(spi_pdma_rx_chid, NU_PDMA_EVENT_TRANSFER_DONE);

    /* Register ISR callback function */
    sChnCB.m_eCBType = eCBType_Event;
    sChnCB.m_pfnCBHandler = nu_pdma_uspi_rx_cb_event;
    sChnCB.m_pvUserData = (void *)psNuUSPI;
    result = nu_pdma_callback_register(spi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_uspi_rx_config;
    }

    /* Register Disable engine dma trigger callback function */
    sChnCB.m_eCBType = eCBType_Disable;
    sChnCB.m_pfnCBHandler = nu_pdma_uspi_rx_cb_disable;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(spi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_uspi_rx_config;
    }

    if (pu8Buf == NULL)
    {
        memctrl  = eMemCtl_SrcFix_DstFix;
        dst_addr = (uint8_t *) &psNuUSPI->dummy;
    }
    else
    {
        memctrl  = eMemCtl_SrcFix_DstInc;
        dst_addr = pu8Buf;
    }

    result = nu_pdma_channel_memctrl_set(spi_pdma_rx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_uspi_rx_config;
    }

    psNuUSPI->m_psSemBus = 0;

    result = nu_pdma_transfer(spi_pdma_rx_chid,
                              bytes_per_word * 8,
                              (uint32_t)&base->RXDAT,
                              (uint32_t)dst_addr,
                              i32RcvLen / bytes_per_word,
                              0);
exit_nu_pdma_uspi_rx_config:

    return result;
}

static int nu_pdma_uspi_tx_config(struct nu_uspi *psNuUSPI, const uint8_t *pu8Buf, int32_t i32SndLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;

    int result;
    uint8_t *src_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    /* Get base address of spi register */
    USPI_T *base = psNuUSPI->base;

    uint8_t spi_pdma_tx_chid = psNuUSPI->pdma_chanid_tx;

    if (pu8Buf == NULL)
    {
        psNuUSPI->dummy = 0;
        memctrl = eMemCtl_SrcFix_DstFix;
        src_addr = (uint8_t *)&psNuUSPI->dummy;
    }
    else
    {
        memctrl = eMemCtl_SrcInc_DstFix;
        src_addr = (uint8_t *)pu8Buf;
    }

    /* Register Disable engine dma trigger callback function */
    sChnCB.m_eCBType = eCBType_Trigger;
    sChnCB.m_pfnCBHandler = nu_pdma_uspi_tx_cb_trigger;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(spi_pdma_tx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_uspi_tx_config;
    }

    result = nu_pdma_channel_memctrl_set(spi_pdma_tx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_uspi_tx_config;
    }

    result = nu_pdma_transfer(spi_pdma_tx_chid,
                              bytes_per_word * 8,
                              (uint32_t)src_addr,
                              (uint32_t)&base->TXDAT,
                              i32SndLen / bytes_per_word,
                              0);
exit_nu_pdma_uspi_tx_config:

    return result;
}


/**
 * SPI PDMA transfer
 */
static int nu_uspi_transmit_pdma(struct nu_uspi *psNuUSPI, const void *tx, void *rx, int length, int dw)
{
    int result = 0;

    result = nu_pdma_uspi_rx_config(psNuUSPI, rx, length, dw);
    LV_ASSERT(result == 0);

    result = nu_pdma_uspi_tx_config(psNuUSPI, tx, length, dw);
    LV_ASSERT(result == 0);

    /* Wait RX-PDMA transfer done */
    while (psNuUSPI->m_psSemBus == 0)
    {
    }

    return length;
}
#endif

int nu_uspi_transfer(struct nu_uspi *psNuUSPI, const void *tx, void *rx, int length)
{
    int ret, dw;

#if defined(CONFIG_SPI_USE_PDMA)
    if ((psNuUSPI->pdma_perp_tx > 0) && (psNuUSPI->pdma_chanid_tx < 0))
        psNuUSPI->pdma_chanid_tx = nu_pdma_channel_allocate(psNuUSPI->pdma_perp_tx);

    if ((psNuUSPI->pdma_perp_rx > 0) && (psNuUSPI->pdma_chanid_rx < 0))
        psNuUSPI->pdma_chanid_rx = nu_pdma_channel_allocate(psNuUSPI->pdma_perp_rx);
#endif

    dw = USPI_GET_DATA_WIDTH(psNuUSPI->base) / 8;

    if (psNuUSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuUSPI->ss_pin), NU_GET_PIN(psNuUSPI->ss_pin)) = 0;
    }
    else
    {
        USPI_SET_SS_LOW(psNuUSPI->base);
    }

#if defined(CONFIG_SPI_USE_PDMA)
    /* DMA transfer constrains */
    if ((psNuUSPI->pdma_chanid_tx != -1) &&
            (psNuUSPI->pdma_chanid_rx != -1) &&
            !((uint32_t)tx % dw) &&
            !((uint32_t)rx % dw) &&
            (dw != 3) &&
            (length >= CONFIG_SPI_USE_PDMA_MIN_THRESHOLD))
        ret = nu_uspi_transmit_pdma(psNuUSPI, tx, rx, length, dw);
    else
#endif
        ret = nu_uspi_transmit_poll(psNuUSPI, tx, rx, length, dw);

    if (psNuUSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuUSPI->ss_pin), NU_GET_PIN(psNuUSPI->ss_pin)) = 1;
    }
    else
    {
        USPI_SET_SS_HIGH(psNuUSPI->base);
    }

    return ret;
}
