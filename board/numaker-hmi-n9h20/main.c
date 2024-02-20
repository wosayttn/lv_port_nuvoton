/**************************************************************************//**
 * @file     main.c
 * @brief    bring up lvgl.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#if defined(__FreeRTOS__)
void lv_nuvoton_task(void *pdata)
{
    while (1)
    {
        lv_task_handler();
        vTaskDelay((const TickType_t) 1 / portTICK_PERIOD_MS);
    }
}

void lv_tick_task(void *pdata)
{
    while (1)
    {
        lv_tick_inc(1);
        vTaskDelay((const TickType_t) 1 / portTICK_PERIOD_MS);
    }
}

#else
#define CONFIG_SYS_TIMER    TIMER0
static void systick_inc(void)
{
    lv_tick_inc(1);
}

static uint32_t get_systick(void)
{
    return sysGetTicks(CONFIG_SYS_TIMER);
}

#endif

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
    sysSetTimerEvent(CONFIG_SYS_TIMER, 1, systick_inc);
#endif

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

#if LV_USE_LOG
static void lv_nuvoton_log(lv_log_level_t level, const char *buf)
{
    sysprintf("[LVGL] %s", buf);
}
#endif /* LV_USE_LOG */

#if defined(__FreeRTOS__)
#endif


int main(void)
{
    sys_init();

    lv_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_nuvoton_log);
#endif /* LV_USE_LOG */

#if defined(__FreeRTOS__)
    lv_tick_set_cb(xTaskGetTickCount);    /*Expression evaluating to current system time in ms*/
    lv_delay_set_cb(vTaskDelay);
#else
    lv_tick_set_cb(get_systick);          /*Expression evaluating to current system time in ms*/
    lv_delay_set_cb(sysDelay);
#endif

    extern void lv_port_disp_init(void);
    lv_port_disp_init();

    extern void lv_port_indev_init(void);
    lv_port_indev_init();

    extern void ui_init(void);
    ui_init();

#if defined(__FreeRTOS__)

#define CONFIG_LV_TASK_STACKSIZE     8192
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

    xTaskCreate(lv_nuvoton_task, "lv_handler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    xTaskCreate(lv_tick_task, "lv_tick", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);

    // Construct other tasks here __start.

    // Construct other tasks here __end.

    vTaskStartScheduler();

    for (;;);

#else

    while (1)
    {
        lv_task_handler();
        // Put your code here  __start.
        // Put your code here  __eend.
    }

#endif

    return 0;
}
