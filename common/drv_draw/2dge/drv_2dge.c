#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv_glue.h"

#if (LV_USE_OS==LV_OS_FREERTOS)
    static SemaphoreHandle_t s_xGE2DSem = NULL;
#endif

void ge2dWaitForCompletion(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    while (xSemaphoreTake(s_xGE2DSem, portMAX_DELAY) != pdTRUE);
#else
    while ((inpw(REG_GE2D_INTSTS) & 0x01) == 0); // wait for command complete
#endif
}

#if (LV_USE_OS==LV_OS_FREERTOS)
void ge2dISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear interrupt status. */
    outpw(REG_GE2D_INTSTS, 1);

    xSemaphoreGiveFromISR(s_xGE2DSem, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif

void ge2dInterruptInit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    s_xGE2DSem = xSemaphoreCreateBinary();
    LV_ASSERT(s_xGE2DSem != NULL);

    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, GE2D_IRQn, (PVOID)ge2dISR);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(GE2D_IRQn);
#endif
}

void ge2dInterruptDeinit(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    sysDisableInterrupt(GE2D_IRQn);

    vSemaphoreDelete(s_xGE2DSem);
#endif
}
