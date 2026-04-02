/**************************************************************************//**
 * @file     drv_spi.c
 * @brief    SPI high level driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "drv_spi.h"

/**
 * @brief Perform SPI transmission using polling method.
 *
 * Transmits and/or receives data over SPI interface using polling (no DMA).
 * Supports three modes: write-only, read-only (with dummy writes), or full-duplex.
 * Blocks until all data is transferred and SPI is idle.
 *
 * @param psNuSPI[in]   Pointer to SPI control structure
 * @param tx[in]        Pointer to transmit buffer (NULL for read-only)
 * @param rx[out]       Pointer to receive buffer (NULL for write-only)
 * @param length[in]    Number of data units to transfer
 * @param dw[in]        Data width per unit (bytes)
 *
 * @return Remaining bytes to transfer (0 on success)
 */
static int nu_spi_transmit_poll(struct nu_spi *psNuSPI, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    SPI_T *base = psNuSPI->base;

    /* ===== Write-only mode ===== */
    if ((tx != NULL) && (rx == NULL))
    {
        while (length > 0)
        {
            /* Write data to SPI TX FIFO */
            tx += nu_spi_write(base, tx, dw);
            length -= dw;
        }
    }
    /* ===== Read-only mode ===== */
    else if ((tx == NULL) && (rx != NULL))
    {
        psNuSPI->dummy = 0;
        while (length > 0)
        {
            /* Send dummy data to SPI TX FIFO to clock in RX data */
            length -= nu_spi_write(base, (const uint8_t *)&psNuSPI->dummy, dw);

            /* Read received data from RX FIFO */
            rx += nu_spi_read(base, rx, dw);
        }
    }
    /* ===== Full-duplex mode (simultaneous read/write) ===== */
    else
    {
        while (length > 0)
        {
            /* Write data to SPI TX FIFO */
            tx += nu_spi_write(base, tx, dw);
            length -= dw;

            /* Read data from RX FIFO */
            rx += nu_spi_read(base, rx, dw);
        }
    }

    /* ===== Wait for transmission completion ===== */
    if (rx)
    {
        /* Wait for SPI to complete transmission */
        while (SPI_IS_BUSY(base))
        {
            /* Drain any remaining data in RX FIFO while SPI is busy */
            while (!SPI_GET_RX_FIFO_EMPTY_FLAG(base))
            {
                rx += nu_spi_read(base, rx, dw);
            }
        }

        /* Read any final data remaining in RX FIFO after SPI goes idle */
        while (!SPI_GET_RX_FIFO_EMPTY_FLAG(base))
        {
            rx += nu_spi_read(base, rx, dw);
        }
    }
    else
    {
        /* If no receive buffer, just drain/clear the RX FIFO */
        nu_spi_drain_rxfifo(base);
    }

    return length;
}

/**
 * @brief Perform SPI send-then-receive operation.
 *
 * Transmits data first, then receives response data in separate phases.
 * Useful for command-response protocols where you send a command followed
 * by receiving the response without the SPI chip select being released.
 *
 * @param psNuSPI[in]   Pointer to SPI control structure
 * @param tx[in]        Pointer to transmit buffer (command data)
 * @param tx_len[in]    Number of bytes to transmit
 * @param rx[out]       Pointer to receive buffer (response data)
 * @param rx_len[in]    Number of bytes to receive
 * @param dw[in]        Data width per unit (bytes)
 *
 * @return Number of bytes received, or negative on error
 */
int nu_spi_send_then_recv(struct nu_spi *psNuSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw)
{
    /* Get actual data width from SPI controller */
    dw = SPI_GET_DATA_WIDTH(psNuSPI->base) / 8;

    /* Drive chip select low (either GPIO or SPI hardware CS) */
    if (psNuSPI->ss_pin > 0)
    {
        /* Use GPIO for chip select */
        GPIO_PIN_DATA(NU_GET_PORT(psNuSPI->ss_pin), NU_GET_PIN(psNuSPI->ss_pin)) = 0;
    }
    else
    {
        /* Use SPI hardware chip select */
        SPI_SET_SS_LOW(psNuSPI->base);
    }

    /* ===== Transmission phase ===== */
    if (tx)
    {
        while (tx_len > 0)
        {
            /* Write command/data to SPI TX FIFO */
            tx += nu_spi_write(psNuSPI->base, tx, dw);
            tx_len -= dw;
        }
    }

    /* Clear SPI RX FIFO to discard any received data during transmission phase */
    nu_spi_drain_rxfifo(psNuSPI->base);

    /* ===== Reception phase ===== */
    if (rx)
    {
        uint32_t dummy = 0xffffffff;
        uint8_t *curr = rx;
        uint32_t remain = rx_len;
        while (remain > 0)
        {
            /* Input data to SPI TX FIFO */
            remain -= nu_spi_write(psNuSPI->base, (const uint8_t *)&dummy, dw);
            curr += nu_spi_read(psNuSPI->base, curr, dw);
        }
        while (SPI_IS_BUSY(psNuSPI->base));
        while ((rx + rx_len) != curr)
        {
            curr += nu_spi_read(psNuSPI->base, curr, dw);
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


#if defined(CONFIG_SPI_USE_PDMA)

static void nu_pdma_spi_rx_cb_event(void *pvUserData, uint32_t u32EventFilter)
{
    struct nu_spi *psNuSPI = (struct nu_spi *)pvUserData;

    SPI_ASSERT(psNuSPI);

#if defined(__FREERTOS__)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(psNuSPI->m_psSemBus, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else
    psNuSPI->m_psSemBus = 1;
#endif
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

#if !defined(__FREERTOS__)
    psNuSPI->m_psSemBus = 0;
#endif

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
    SPI_ASSERT(result == 0);

    result = nu_pdma_spi_tx_config(psNuSPI, tx, length, dw);
    SPI_ASSERT(result == 0);

    /* Wait RX-PDMA transfer done */
#if defined(__FREERTOS__)
    xSemaphoreTake(psNuSPI->m_psSemBus, portMAX_DELAY);
#else
    while (psNuSPI->m_psSemBus == 0)
    {
    }
#endif

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

#if defined(__FREERTOS__)
    if (psNuSPI->m_psSemBus == NULL)
        psNuSPI->m_psSemBus = xSemaphoreCreateBinary();
#endif

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
