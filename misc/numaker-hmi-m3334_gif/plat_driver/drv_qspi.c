/**************************************************************************//**
 * @file     drv_qspi.c
 * @brief    QSPI high level driver with PDMA support
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "drv_qspi.h"

int nu_qspi_transmit_poll(struct nu_qspi *psNuQSPI, const uint8_t *tx, uint8_t *rx, int length, int dw)
{
    QSPI_T *base = psNuQSPI->base;
    int tx_cnt = 0;
    int rx_cnt = 0;

    // Write-only
    if ((tx != NULL) && (rx == NULL))
    {
        while (tx_cnt < length)
        {
            while (QSPI_GET_TX_FIFO_FULL_FLAG(base));
            tx += nu_qspi_write(base, tx, dw);
            tx_cnt += dw;
        }
        nu_qspi_drain_rxfifo(base);
    }
    // Read-only
    else if ((tx == NULL) && (rx != NULL))
    {
        psNuQSPI->dummy = 0;
        while (rx_cnt < length)
        {
            if ((tx_cnt - rx_cnt < 8 * dw) && !QSPI_GET_TX_FIFO_FULL_FLAG(base) && (tx_cnt < length))
            {
                nu_qspi_write(base, (const uint8_t *)&psNuQSPI->dummy, dw);
                tx_cnt += dw;
            }
            if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(base))
            {
                rx += nu_qspi_read(base, rx, dw);
                rx_cnt += dw;
            }
        }
    }
    // Full duplex (Read & Write)
    else if ((tx != NULL) && (rx != NULL))
    {
        while (rx_cnt < length)
        {
            if ((tx_cnt - rx_cnt < 8 * dw) && !QSPI_GET_TX_FIFO_FULL_FLAG(base) && (tx_cnt < length))
            {
                tx += nu_qspi_write(base, tx, dw);
                tx_cnt += dw;
            }
            if (!QSPI_GET_RX_FIFO_EMPTY_FLAG(base))
            {
                rx += nu_qspi_read(base, rx, dw);
                rx_cnt += dw;
            }
        }
    }

    while (QSPI_IS_BUSY(base));
    return length;
}

int nu_qspi_send_then_recv(struct nu_qspi *psNuQSPI, const uint8_t *tx, int tx_len, uint8_t *rx, int rx_len, int dw)
{
    dw = QSPI_GET_DATA_WIDTH(psNuQSPI->base) / 8;
    if (dw == 0) dw = 4;

    if (psNuQSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 0;
    }
    else
    {
        QSPI_SET_SS_LOW(psNuQSPI->base);
    }

    if (tx)
    {
        while (tx_len > 0)
        {
            tx += nu_qspi_write(psNuQSPI->base, tx, dw);
            tx_len -= dw;
        }
    }

    /* Clear QSPI RX FIFO */
    nu_qspi_drain_rxfifo(psNuQSPI->base);

    if (rx)
    {
        uint32_t dummy = 0;
        uint8_t *curr = rx;
        uint32_t remain = rx_len;
        while (remain > 0)
        {
            remain -= nu_qspi_write(psNuQSPI->base, (const uint8_t *)&dummy, dw);
            curr += nu_qspi_read(psNuQSPI->base, curr, dw);
        }
        while (QSPI_IS_BUSY(psNuQSPI->base));
        while ((rx + rx_len) != curr)
        {
            curr += nu_qspi_read(psNuQSPI->base, curr, dw);
        }
    }

    if (psNuQSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 1;
    }
    else
    {
        QSPI_SET_SS_HIGH(psNuQSPI->base);
    }

    return 0;
}

#if defined(CONFIG_QSPI_USE_PDMA)

static void nu_pdma_qspi_rx_cb_event(void *pvUserData, uint32_t u32EventFilter)
{
    struct nu_qspi *psNuQSPI = (struct nu_qspi *)pvUserData;

    QSPI_ASSERT(psNuQSPI);

    while (QSPI_IS_BUSY(psNuQSPI->base));

    if (psNuQSPI->pfnTransferDoneCb)
    {
        void (*cb)(void *) = psNuQSPI->pfnTransferDoneCb;
        void *pvData = psNuQSPI->pvUserData;
        psNuQSPI->pfnTransferDoneCb = NULL;
        psNuQSPI->bAsyncBusy = 0;

        if (psNuQSPI->ss_pin > 0)
        {
            GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 1;
        }
        else
        {
            QSPI_SET_SS_HIGH(psNuQSPI->base);
        }

        cb(pvData);
    }
    else
    {
#if defined(__FREERTOS__)
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && psNuQSPI->m_psSemBus != NULL)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(psNuQSPI->m_psSemBus, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            psNuQSPI->m_u32Done = 1;
        }
#else
        psNuQSPI->m_u32Done = 1;
#endif
    }
}

static void nu_pdma_qspi_tx_cb_trigger(void *pvUserData, uint32_t u32UserData)
{
    QSPI_T *base = (QSPI_T *)pvUserData;
    QSPI_TRIGGER_TX_RX_PDMA(base);
}

static void nu_pdma_qspi_rx_cb_disable(void *pvUserData, uint32_t u32UserData)
{
    QSPI_T *base = (QSPI_T *)pvUserData;
    QSPI_DISABLE_TX_RX_PDMA(base);
}

static int nu_pdma_qspi_rx_config(struct nu_qspi *psNuQSPI, uint8_t *pu8Buf, int32_t i32RcvLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;
    int result;
    uint8_t *dst_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    QSPI_T *base = psNuQSPI->base;
    uint8_t qspi_pdma_rx_chid = psNuQSPI->pdma_chanid_rx;

    nu_pdma_filtering_set(qspi_pdma_rx_chid, NU_PDMA_EVENT_TRANSFER_DONE);

    /* Register ISR callback function */
    sChnCB.m_eCBType = eCBType_Event;
    sChnCB.m_pfnCBHandler = nu_pdma_qspi_rx_cb_event;
    sChnCB.m_pvUserData = (void *)psNuQSPI;
    result = nu_pdma_callback_register(qspi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_qspi_rx_config;
    }

    /* Register Disable callback function */
    sChnCB.m_eCBType = eCBType_Disable;
    sChnCB.m_pfnCBHandler = nu_pdma_qspi_rx_cb_disable;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(qspi_pdma_rx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_qspi_rx_config;
    }

    if (pu8Buf == NULL)
    {
        memctrl  = eMemCtl_SrcFix_DstFix;
        dst_addr = (uint8_t *)&psNuQSPI->dummy;
    }
    else
    {
        memctrl  = eMemCtl_SrcFix_DstInc;
        dst_addr = pu8Buf;
    }

    result = nu_pdma_channel_memctrl_set(qspi_pdma_rx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_qspi_rx_config;
    }

    psNuQSPI->m_u32Done = 0;

    result = nu_pdma_transfer(qspi_pdma_rx_chid,
                              bytes_per_word * 8,
                              (uint32_t)&base->RX,
                              (uint32_t)dst_addr,
                              i32RcvLen / bytes_per_word,
                              0);
exit_nu_pdma_qspi_rx_config:

    return result;
}

static int nu_pdma_qspi_tx_config(struct nu_qspi *psNuQSPI, const uint8_t *pu8Buf, int32_t i32SndLen, uint8_t bytes_per_word)
{
    struct nu_pdma_chn_cb sChnCB;
    int result;
    uint8_t *src_addr = NULL;
    nu_pdma_memctrl_t memctrl = eMemCtl_Undefined;

    QSPI_T *base = psNuQSPI->base;
    uint8_t qspi_pdma_tx_chid = psNuQSPI->pdma_chanid_tx;

    if (pu8Buf == NULL)
    {
        psNuQSPI->dummy = 0;
        memctrl = eMemCtl_SrcFix_DstFix;
        src_addr = (uint8_t *)&psNuQSPI->dummy;
    }
    else
    {
        memctrl = eMemCtl_SrcInc_DstFix;
        src_addr = (uint8_t *)pu8Buf;
    }

    /* Register Trigger callback function */
    sChnCB.m_eCBType = eCBType_Trigger;
    sChnCB.m_pfnCBHandler = nu_pdma_qspi_tx_cb_trigger;
    sChnCB.m_pvUserData = (void *)base;
    result = nu_pdma_callback_register(qspi_pdma_tx_chid, &sChnCB);
    if (result != 0)
    {
        goto exit_nu_pdma_qspi_tx_config;
    }

    result = nu_pdma_channel_memctrl_set(qspi_pdma_tx_chid, memctrl);
    if (result != 0)
    {
        goto exit_nu_pdma_qspi_tx_config;
    }

    result = nu_pdma_transfer(qspi_pdma_tx_chid,
                              bytes_per_word * 8,
                              (uint32_t)src_addr,
                              (uint32_t)&base->TX,
                              i32SndLen / bytes_per_word,
                              0);
exit_nu_pdma_qspi_tx_config:

    return result;
}

int nu_qspi_transmit_pdma(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length, int dw)
{
    int result = 0;

    /* Drain any pending binary semaphore token */
#if defined(__FREERTOS__)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && psNuQSPI->m_psSemBus != NULL)
    {
        xSemaphoreTake(psNuQSPI->m_psSemBus, 0);
    }
#endif

    result = nu_pdma_qspi_rx_config(psNuQSPI, (uint8_t *)rx, length, (uint8_t)dw);
    QSPI_ASSERT(result == 0);

    result = nu_pdma_qspi_tx_config(psNuQSPI, (const uint8_t *)tx, length, (uint8_t)dw);
    QSPI_ASSERT(result == 0);

    /* Wait RX-PDMA transfer done */
#if defined(__FREERTOS__)
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && psNuQSPI->m_psSemBus != NULL)
    {
        xSemaphoreTake(psNuQSPI->m_psSemBus, portMAX_DELAY);
    }
    else
    {
        while (psNuQSPI->m_u32Done == 0);
    }
#else
    while (psNuQSPI->m_u32Done == 0);
#endif

    return length;
}

#endif /* CONFIG_QSPI_USE_PDMA */

int nu_qspi_init_pdma(struct nu_qspi *psNuQSPI)
{
#if defined(CONFIG_QSPI_USE_PDMA)
    if ((psNuQSPI->pdma_perp_tx > 0) && (psNuQSPI->pdma_chanid_tx < 0))
        psNuQSPI->pdma_chanid_tx = nu_pdma_channel_allocate(psNuQSPI->pdma_perp_tx);

    if ((psNuQSPI->pdma_perp_rx > 0) && (psNuQSPI->pdma_chanid_rx < 0))
        psNuQSPI->pdma_chanid_rx = nu_pdma_channel_allocate(psNuQSPI->pdma_perp_rx);

#if defined(__FREERTOS__)
    if (psNuQSPI->m_psSemBus == NULL)
        psNuQSPI->m_psSemBus = xSemaphoreCreateBinary();
#endif

    if ((psNuQSPI->pdma_chanid_tx < 0) || (psNuQSPI->pdma_chanid_rx < 0))
        return -1;
#endif
    return 0;
}

void nu_qspi_transfer_wait(struct nu_qspi *psNuQSPI)
{
#if defined(CONFIG_QSPI_USE_PDMA)
    while (psNuQSPI->bAsyncBusy)
    {
#if defined(__FREERTOS__)
        vTaskDelay(pdMS_TO_TICKS(1));
#endif
    }
#endif
}

int nu_qspi_transfer(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length)
{
    int ret, dw;

#if defined(CONFIG_QSPI_USE_PDMA)
    if (psNuQSPI->bAsyncBusy)
    {
        nu_qspi_transfer_wait(psNuQSPI);
    }
#endif

    nu_qspi_init_pdma(psNuQSPI);

    dw = QSPI_GET_DATA_WIDTH(psNuQSPI->base) / 8;
    if (dw == 0) dw = 4;

    if (psNuQSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 0;
    }
    else
    {
        QSPI_SET_SS_LOW(psNuQSPI->base);
    }

#if defined(CONFIG_QSPI_USE_PDMA)
    if ((psNuQSPI->pdma_chanid_tx != -1) &&
        (psNuQSPI->pdma_chanid_rx != -1) &&
        !((uint32_t)tx % dw) &&
        !((uint32_t)rx % dw) &&
        (dw != 3) &&
        (length >= CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD))
    {
        ret = nu_qspi_transmit_pdma(psNuQSPI, tx, rx, length, dw);
    }
    else
#endif
    {
        ret = nu_qspi_transmit_poll(psNuQSPI, (const uint8_t *)tx, (uint8_t *)rx, length, dw);
    }

    if (psNuQSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 1;
    }
    else
    {
        QSPI_SET_SS_HIGH(psNuQSPI->base);
    }

    return ret;
}

int nu_qspi_transfer_async(struct nu_qspi *psNuQSPI, const void *tx, void *rx, int length, void (*cb)(void *), void *pvUserData)
{
    int ret, dw;

#if defined(CONFIG_QSPI_USE_PDMA)
    if (psNuQSPI->bAsyncBusy)
    {
        nu_qspi_transfer_wait(psNuQSPI);
    }

    nu_qspi_init_pdma(psNuQSPI);
#endif

    dw = QSPI_GET_DATA_WIDTH(psNuQSPI->base) / 8;
    if (dw == 0) dw = 4;

    if (psNuQSPI->ss_pin > 0)
    {
        GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 0;
    }
    else
    {
        QSPI_SET_SS_LOW(psNuQSPI->base);
    }

#if defined(CONFIG_QSPI_USE_PDMA)
    if ((psNuQSPI->pdma_chanid_tx != -1) &&
        (psNuQSPI->pdma_chanid_rx != -1) &&
        !((uint32_t)tx % dw) &&
        !((uint32_t)rx % dw) &&
        (dw != 3) &&
        (length >= CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD))
    {
        psNuQSPI->pfnTransferDoneCb = cb;
        psNuQSPI->pvUserData = pvUserData;
        psNuQSPI->bAsyncBusy = 1;

        ret = nu_pdma_qspi_rx_config(psNuQSPI, (uint8_t *)rx, length, (uint8_t)dw);
        QSPI_ASSERT(ret == 0);

        ret = nu_pdma_qspi_tx_config(psNuQSPI, (const uint8_t *)tx, length, (uint8_t)dw);
        QSPI_ASSERT(ret == 0);

        return length;
    }
    else
#endif
    {
        ret = nu_qspi_transmit_poll(psNuQSPI, (const uint8_t *)tx, (uint8_t *)rx, length, dw);

        if (psNuQSPI->ss_pin > 0)
        {
            GPIO_PIN_DATA(NU_GET_PORT(psNuQSPI->ss_pin), NU_GET_PIN(psNuQSPI->ss_pin)) = 1;
        }
        else
        {
            QSPI_SET_SS_HIGH(psNuQSPI->base);
        }

        if (cb)
        {
            cb(pvUserData);
        }

        return ret;
    }
}

int nu_qspi_read_pdma(struct nu_qspi *psNuQSPI, void *rx, int length)
{
    int dw = QSPI_GET_DATA_WIDTH(psNuQSPI->base) / 8;
    if (dw == 0) dw = 4;

    nu_qspi_init_pdma(psNuQSPI);

#if defined(CONFIG_QSPI_USE_PDMA)
    if ((psNuQSPI->pdma_chanid_tx != -1) &&
        (psNuQSPI->pdma_chanid_rx != -1) &&
        (length >= CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD))
    {
        return nu_qspi_transmit_pdma(psNuQSPI, NULL, rx, length, dw);
    }
#endif
    return nu_qspi_transmit_poll(psNuQSPI, NULL, (uint8_t *)rx, length, dw);
}

int nu_qspi_write_pdma(struct nu_qspi *psNuQSPI, const void *tx, int length)
{
    int dw = QSPI_GET_DATA_WIDTH(psNuQSPI->base) / 8;
    if (dw == 0) dw = 4;

    nu_qspi_init_pdma(psNuQSPI);

#if defined(CONFIG_QSPI_USE_PDMA)
    if ((psNuQSPI->pdma_chanid_tx != -1) &&
        (psNuQSPI->pdma_chanid_rx != -1) &&
        (length >= CONFIG_QSPI_USE_PDMA_MIN_THRESHOLD))
    {
        return nu_qspi_transmit_pdma(psNuQSPI, tx, NULL, length, dw);
    }
#endif
    return nu_qspi_transmit_poll(psNuQSPI, (const uint8_t *)tx, NULL, length, dw);
}
