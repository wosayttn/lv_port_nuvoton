/**************************************************************************//**
 * @file     main.c
 * @brief    bring up lvgl.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
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

#if defined(__FreeRTOS__)

#else
    sysSetTimerReferenceClock(CONFIG_SYS_TIMER, CONFIG_EXTERN_FREQUENCY);
    sysStartTimer(CONFIG_SYS_TIMER, CONFIG_TICK_PER_SECOND, PERIODIC_MODE);
#endif

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

#if LV_USE_LOG
static void lv_nuvoton_log(const char *buf)
{
    sysprintf("%s", buf);
}
#endif /* LV_USE_LOG */

#if defined(__FreeRTOS__)
void lv_nuvoton_task(void *pdata)
{
    while (1)
    {
        lv_task_handler();
        vTaskDelay((const TickType_t) LV_DISP_DEF_REFR_PERIOD / portTICK_PERIOD_MS);
    }
}
#endif


int main(void)
{
    sys_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_nuvoton_log);
#endif /* LV_USE_LOG */

    lv_init();

    extern void lv_port_disp_init(void);
    lv_port_disp_init();

    extern void lv_port_indev_init(void);
    lv_port_indev_init();

    extern void ui_init(void);
    ui_init();

#if defined(__FreeRTOS__)

#define CONFIG_LV_TASK_STACKSIZE     8192
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

    xTaskCreate(lv_nuvoton_task, "lv", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);

    // Construct other tasks here __start.

    // Construct other tasks here __end.

    vTaskStartScheduler();

    for (;;);

#else

    while (1)
    {
        lv_task_handler();
#if 1
        sysDelay(LV_DISP_DEF_REFR_PERIOD);
#else
        // Put your code here  __start.
        // Put your code here  __eend.
#endif
    }

#endif

    return 0;
}
