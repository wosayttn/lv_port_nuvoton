/**************************************************************************//**
 * @file     drv_pdma.c
 * @brief    PDMA high level driver for M460 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "drv_spi.h"

#define SPI_GET_DATA_WIDTH(spi)  (((spi)->CTL & SPI_CTL_DWIDTH_Msk) >> SPI_CTL_DWIDTH_Pos)

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

static int nu_spi_transmit_poll(struct nu_spi *psNuSPI, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    SPI_T *base = psNuSPI->base;
    // Write-only
    if ((tx != NULL) && (rx == NULL))
    {
        while (length > 0)
        {
            tx += nu_spi_write(base, tx, dw);
            length -= dw;
        }
    } // if (tx != NULL && rx == NULL)
    // Read-only
    else if ((tx == NULL) && (rx != NULL))
    {
        psNuSPI->dummy = 0;
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            length -= nu_spi_write(base, (const uint8_t *)&psNuSPI->dummy, dw);

            /* Read data from RX FIFO */
            rx += nu_spi_read(base, rx, dw);
        }
    } // else if (tx == NULL && rx != NULL)
    // Read&Write
    else
    {
        while (length > 0)
        {
            /* Input data to SPI TX FIFO */
            tx += nu_spi_write(base, tx, dw);
            length -= dw;

            /* Read data from RX FIFO */
            rx += nu_spi_read(base, rx, dw);
        }
    } // else

    /* Wait RX or drain RX-FIFO */
    if (rx)
    {
        // Wait SPI transmission done
        while (SPI_IS_BUSY(base))
        {
            while (!SPI_GET_RX_FIFO_EMPTY_FLAG(base))
            {
                rx += nu_spi_read(base, rx, dw);
            }
        }

        while (!SPI_GET_RX_FIFO_EMPTY_FLAG(base))
        {
            rx += nu_spi_read(base, rx, dw);
        }
    }
    else
    {
        /* Clear SPI RX FIFO */
        nu_spi_drain_rxfifo(base);
    }

    return length;
}

#if defined(CONFIG_SPI_USE_PDMA)

static void nu_pdma_spi_rx_cb_event(void *pvUserData, uint32_t u32EventFilter)
{
    struct nu_spi *psNuSPI = (struct nu_spi *)pvUserData;

    LV_ASSERT(psNuSPI);

    psNuSPI->m_psSemBus = 1;
}

static void nu_pdma_spi_tx_cb_trigger(void *pvUserData, uint32_t u32UserData)
{
    /* Get base address of spi register */
    SPI_T *base = (SPI_T *)pvUserData;

    /* Trigger TX/RX PDMA transfer. */
    SPI_TRIGGER_TX_RX_PDMA(base);
}

static void nu_pdma_spi_rx_cb_disable(void *pvUserData, uint32_t u32UserData)
{
    /* Get base address of spi register */
    SPI_T *base = (SPI_T *)pvUserData;

    /* Stop TX/RX DMA transfer. */
    SPI_DISABLE_TX_RX_PDMA(base);
}

static int nu_pdma_spi_rx_config(struct nu_spi *psNuSPI, uint8_t *pu8Buf, int32_t i32RcvLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;

    int result;
    uint8_t *dst_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    /* Get base address of spi register */
    SPI_T *base = psNuSPI->base;

    uint8_t spi_pdma_rx_chid = psNuSPI->pdma_chanid_rx;

    nu_pdma_filtering_set(spi_pdma_rx_chid, NU_PDMA_EVENT_TRANSFER_DONE);

    /* Register ISR callback function */
    sChnCB.m_eCBType = eCBType_Event;
    sChnCB.m_pfnCBHandler = nu_pdma_spi_rx_cb_event;
    sChnCB.m_pvUserData = (void *)psNuSPI;
    result = nu_pdma_callback_register(spi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_spi_rx_config;
    }

    /* Register Disable engine dma trigger callback function */
    sChnCB.m_eCBType = eCBType_Disable;
    sChnCB.m_pfnCBHandler = nu_pdma_spi_rx_cb_disable;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(spi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_spi_rx_config;
    }

    if (pu8Buf == NULL)
    {
        memctrl  = eMemCtl_SrcFix_DstFix;
        dst_addr = (uint8_t *) &psNuSPI->dummy;
    }
    else
    {
        memctrl  = eMemCtl_SrcFix_DstInc;
        dst_addr = pu8Buf;
    }

    result = nu_pdma_channel_memctrl_set(spi_pdma_rx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_spi_rx_config;
    }

    psNuSPI->m_psSemBus = 0;

    result = nu_pdma_transfer(spi_pdma_rx_chid,
                              bytes_per_word * 8,
                              (uint32_t)&base->RX,
                              (uint32_t)dst_addr,
                              i32RcvLen / bytes_per_word,
                              0);
exit_nu_pdma_spi_rx_config:

    return result;
}

static int nu_pdma_spi_tx_config(struct nu_spi *psNuSPI, const uint8_t *pu8Buf, int32_t i32SndLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;

    int result;
    uint8_t *src_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    /* Get base address of spi register */
    SPI_T *base = psNuSPI->base;

    uint8_t spi_pdma_tx_chid = psNuSPI->pdma_chanid_tx;

    if (pu8Buf == NULL)
    {
        psNuSPI->dummy = 0;
        memctrl = eMemCtl_SrcFix_DstFix;
        src_addr = (uint8_t *)&psNuSPI->dummy;
    }
    else
    {
        memctrl = eMemCtl_SrcInc_DstFix;
        src_addr = (uint8_t *)pu8Buf;
    }

    /* Register Disable engine dma trigger callback function */
    sChnCB.m_eCBType = eCBType_Trigger;
    sChnCB.m_pfnCBHandler = nu_pdma_spi_tx_cb_trigger;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(spi_pdma_tx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_spi_tx_config;
    }

    result = nu_pdma_channel_memctrl_set(spi_pdma_tx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_spi_tx_config;
    }

    result = nu_pdma_transfer(spi_pdma_tx_chid,
                              bytes_per_word * 8,
                              (uint32_t)src_addr,
                              (uint32_t)&base->TX,
                              i32SndLen / bytes_per_word,
                              0);
exit_nu_pdma_spi_tx_config:

    return result;
}


/**
 * SPI PDMA transfer
 */
static int nu_spi_transmit_pdma(struct nu_spi *psNuSPI, const void *tx, void *rx, int length, int dw)
{
    int result = 0;

    result = nu_pdma_spi_rx_config(psNuSPI, rx, length, dw);
    LV_ASSERT(result == 0);

    result = nu_pdma_spi_tx_config(psNuSPI, tx, length, dw);
    LV_ASSERT(result == 0);

    /* Wait RX-PDMA transfer done */
    while (psNuSPI->m_psSemBus == 0)
    {
    }

    return length;
}
#endif

int nu_spi_transfer(struct nu_spi *psNuSPI, const void *tx, void *rx, int length)
{
    int ret, dw;

#if defined(CONFIG_SPI_USE_PDMA)
    if ((psNuSPI->pdma_perp_tx > 0) && (psNuSPI->pdma_chanid_tx < 0))
        psNuSPI->pdma_chanid_tx = nu_pdma_channel_allocate(psNuSPI->pdma_perp_tx);

    if ((psNuSPI->pdma_perp_rx > 0) && (psNuSPI->pdma_chanid_rx < 0))
        psNuSPI->pdma_chanid_rx = nu_pdma_channel_allocate(psNuSPI->pdma_perp_rx);
#endif

    dw = SPI_GET_DATA_WIDTH(psNuSPI->base) / 8;

    if (psNuSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuSPI->ss_pin), NU_GET_PIN(psNuSPI->ss_pin)) = 0;
    }
    else
    {
        SPI_SET_SS_LOW(psNuSPI->base);
    }

#if defined(CONFIG_SPI_USE_PDMA)
    /* DMA transfer constrains */
    if ((psNuSPI->pdma_chanid_tx != -1) &&
            (psNuSPI->pdma_chanid_rx != -1) &&
            !((uint32_t)tx % dw) &&
            !((uint32_t)rx % dw) &&
            (dw != 3) &&
            (length >= CONFIG_SPI_USE_PDMA_MIN_THRESHOLD))
        ret = nu_spi_transmit_pdma(psNuSPI, tx, rx, length, dw);
    else
#endif
        ret = nu_spi_transmit_poll(psNuSPI, tx, rx, length, dw);

    if (psNuSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuSPI->ss_pin), NU_GET_PIN(psNuSPI->ss_pin)) = 1;
    }
    else
    {
        SPI_SET_SS_HIGH(psNuSPI->base);
    }

    return ret;
}
