/**************************************************************************//**
 * @file     main.c
 * @brief    bring up lvgl.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lv_glue.h"


static void sys_init(void)
{
    // Cache on.
    if (! sysGetCacheState())
    {
        sysInvalidCache();
        sysEnableCache(CACHE_WRITE_BACK);
        sysFlushCache(I_D_CACHE);
    }

    WB_UART_T uart =
    {
        .uart_no = WB_UART_1,
        .uiBaudrate = 115200,
        .uiDataBits = WB_DATA_BITS_8,
        .uiStopBits = WB_STOP_BITS_1,
        .uiParity = WB_PARITY_NONE,
        .uiRxTriggerLevel = LEVEL_1_BYTE,
    };
    uart.uiFreq = (sysGetExternalClock() * 1000);

    sysUartPort(WB_UART_1);
    sysInitializeUART(&uart);

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

int main(void)
{
    sys_init();

    task_lv_init();

    /* Start scheduling. */
    vTaskStartScheduler();

    for (;;);

    return 0;
}
