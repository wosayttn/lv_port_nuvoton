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
    sysInvalidCache();
    sysEnableCache(CACHE_WRITE_BACK);
    sysFlushCache(I_D_CACHE);

    WB_UART_T uart =
    {
        .uart_no = WB_UART_1,
        .uiBaudrate = 115200,
        .uiDataBits = WB_DATA_BITS_8,
        .uiStopBits = WB_STOP_BITS_1,
        .uiParity = WB_PARITY_NONE,
        .uiRxTriggerLevel = LEVEL_1_BYTE,
    };
    uart.uiFreq = CONFIG_EXTERN_FREQUENCY;

    sysUartPort(WB_UART_1);
    sysInitializeUART(&uart);

    sysSetTimerReferenceClock(CONFIG_SYS_TIMER, CONFIG_EXTERN_FREQUENCY);
    sysStartTimer(CONFIG_SYS_TIMER, CONFIG_TICK_PER_SECOND, PERIODIC_MODE);

    LV_LOG_INFO("APLL: %d kHz\n", sysGetClock(eSYS_APLL));
    LV_LOG_INFO("UPLL: %d kHz\n", sysGetClock(eSYS_UPLL));
    LV_LOG_INFO("SYSTEM: %d kHz\n", sysGetClock(eSYS_SYSTEM));
    LV_LOG_INFO("HCLK1: %d kHz\n", sysGetClock(eSYS_HCLK1));
    LV_LOG_INFO("HCLK234: %d kHz\n", sysGetClock(eSYS_HCLK234));
    LV_LOG_INFO("PCLK: %d kHz\n", sysGetClock(eSYS_PCLK));
}

#if LV_USE_LOG
static void lv_nuvoton_log(const char *buf)
{
    sysprintf("%s", buf);
}
#endif /* LV_USE_LOG */

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

    while (1)
    {
        lv_task_handler();
        sysDelay(LV_DISP_DEF_REFR_PERIOD);
    }

    return 0;
}
