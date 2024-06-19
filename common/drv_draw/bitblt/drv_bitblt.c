#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv_glue.h"

#if (LV_USE_OS==LV_OS_FREERTOS)
    static SemaphoreHandle_t s_xBITBLTSem = NULL;
#endif

void bitbltWaitForCompletion(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    while (xSemaphoreTake(s_xBITBLTSem, portMAX_DELAY) != pdTRUE);
#endif
}

#if (LV_USE_OS==LV_OS_FREERTOS)
void bitbltISR(void)
{
    UINT32 bltIntStat = inp32(REG_BLTINTCR);

    /* BLT complete */
    if (bltIntStat & BLT_INTS)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* clear interrupt (write 1 to clear) */
        outp32(REG_BLTINTCR, (bltIntStat & BLT_INTE) | BLT_INTS);

        xSemaphoreGiveFromISR(s_xBITBLTSem, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
#endif

void bitbltInterruptInit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    s_xBITBLTSem = xSemaphoreCreateBinary();
    LV_ASSERT(s_xBITBLTSem != NULL);

    outp32(REG_BLTINTCR, inp32(REG_BLTINTCR) | BLT_INTE);

    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, IRQ_BLT, (PVOID)bitbltISR);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_BLT);
#endif
}

void bitbltInterruptDeinit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    vSemaphoreDelete(s_xBITBLTSem);
#endif
}
