#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv_glue.h"
#include "dma350_ch_drv.h"
#include "dma350_lib.h"

#if defined(__ICCARM__)
    #include "arm_cmse.h" // patch from EWARM 9.50.2 service pack
#else
    #include <arm_cmse.h>
#endif

#if (LV_USE_OS==LV_OS_FREERTOS)
    static SemaphoreHandle_t s_xGDMASem = NULL;
#endif

/* DMA350 driver structures */
static const struct dma350_dev_cfg_t GDMA_DEV_CFG_S =
{
    .dma_sec_cfg = (DMASECCFG_TypeDef *)(GDMA_S + 0x0UL),
    .dma_sec_ctrl = (DMASECCTRL_TypeDef *)(GDMA_S + 0x100UL),
    .dma_nsec_ctrl = (DMANSECCTRL_TypeDef *)(GDMA_S + 0x200UL),
    .dma_info = (DMAINFO_TypeDef *)(GDMA_S + 0xF00UL)
};

static struct dma350_dev_data_t GDMA_DEV_DATA_S =
{
    .state = 0
};

static struct dma350_dev_t GDMA_DEV_S =
{
    &(GDMA_DEV_CFG_S),
    &(GDMA_DEV_DATA_S)
};

static struct dma350_ch_dev_t GDMA_CH0_DEV_S =
{
    .cfg = {
        .ch_base = (DMACH_TypeDef *)(GDMA_S + 0x1000UL),
        .channel = 0
    },
    .data = {0}
};

struct dma350_ch_dev_t *const GDMA_CH_DEV_S[] =
{
    &GDMA_CH0_DEV_S
};

void gdmaWaitForCompletion(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    while (xSemaphoreTake(s_xGDMASem, portMAX_DELAY) != pdTRUE);
#endif
}

#if (LV_USE_OS==LV_OS_FREERTOS)
void GDMACH0_IRQHandler(void)
{
    /* Clear interrupt status. */
    union dma350_ch_status_t status = dma350_ch_get_status(GDMA_CH_DEV_S[0]);

    if (status.b.STAT_DONE)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        GDMA_CH_DEV_S[0]->cfg.ch_base->CH_STATUS = DMA350_CH_STAT_DONE;

        xSemaphoreGiveFromISR(s_xGDMASem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
#endif

void gdmaInterruptInit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    s_xGDMASem = xSemaphoreCreateBinary();
    LV_ASSERT(s_xGDMASem != NULL);

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable GDMA0 clock source */
    CLK_EnableModuleClock(GDMA0_MODULE);

    /* Reset GDMA module */
    SYS_ResetModule(SYS_GDMA0RST);

    dma350_init(&GDMA_DEV_S);

    /* Enable NVIC for GDMA CH0 */
    NVIC_EnableIRQ(GDMACH0_IRQn);
#endif
}

void gdmaInterruptDeinit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    NVIC_DisableIRQ(GDMACH0_IRQn);

    vSemaphoreDelete(s_xGDMASem);
#endif
}
