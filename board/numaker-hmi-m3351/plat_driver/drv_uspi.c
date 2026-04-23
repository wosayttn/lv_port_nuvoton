/**************************************************************************//**
 * @file     drv_uspi.c
 * @brief    USPI high level driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "drv_uspi.h"

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

int nu_uspi_send_then_recv(struct nu_uspi *psNuUSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw)
{
    dw = USPI_GET_DATA_WIDTH(psNuUSPI->base) / 8;

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
        while (tx_len > 0)
        {
            tx += nu_uspi_write(psNuUSPI->base, tx, dw);

            while (USPI_IS_BUSY(psNuUSPI->base));
					
            tx_len -= dw;
        }
    }

    /* Clear SPI RX FIFO */
    nu_uspi_drain_rxfifo(psNuUSPI->base);

    if (rx)
    {
        uint32_t dummy = 0xffffffff;
        uint8_t *curr = rx;
        uint32_t remain = rx_len;
        while (remain > 0)
        {
            /* Input data to SPI TX FIFO */
            remain -= nu_uspi_write(psNuUSPI->base, (const uint8_t *)&dummy, dw);
            while ((rx + rx_len) != curr)
            {
                while (USPI_GET_RX_EMPTY_FLAG(psNuUSPI->base));				
                curr += nu_uspi_read(psNuUSPI->base, curr, dw);
            }
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


#if defined(CONFIG_SPI_USE_PDMA)

static void nu_pdma_uspi_rx_cb_event(void *pvUserData, uint32_t u32EventFilter)
{
    struct nu_uspi *psNuUSPI = (struct nu_uspi *)pvUserData;

    USPI_ASSERT(psNuUSPI);

#if defined(__FREERTOS__)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(psNuUSPI->m_psSemBus, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else
    psNuUSPI->m_psSemBus = 1;
#endif
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

#if !defined(__FREERTOS__)
    psNuUSPI->m_psSemBus = 0;
#endif

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
    USPI_ASSERT(result == 0);

    result = nu_pdma_uspi_tx_config(psNuUSPI, tx, length, dw);
    USPI_ASSERT(result == 0);

    /* Wait RX-PDMA transfer done */
#if defined(__FREERTOS__)
    xSemaphoreTake(psNuUSPI->m_psSemBus, portMAX_DELAY);
#else
    while (psNuUSPI->m_psSemBus == 0)
    {
    }
#endif

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

#if defined(__FREERTOS__)
    if (psNuUSPI->m_psSemBus == NULL)
        psNuUSPI->m_psSemBus = xSemaphoreCreateBinary();
#endif

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
