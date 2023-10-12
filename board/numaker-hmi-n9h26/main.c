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
    uart.uiFreq = CONFIG_EXTERN_FREQUENCY;

    //sysUartPort(WB_UART_1);
    sysInitializeUART(&uart);

    /**********************************************************************************************
     * Clock Constraints:
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from
     *     same clock source
     *********************************************************************************************/
#if 0
    /**********************************************************************************************
     * Slower down system and memory clock procedures:
     * If current working clock fast than desired working clock, Please follow the procedure below
     * 1. Change System Clock first
     * 2. Then change Memory Clock
     *
     * Following example shows the Memory Clock = System Clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
    sysSetDramClock(eSYS_EXT, 12000000, 12000000);
#else
    /**********************************************************************************************
     * Speed up system and memory clock procedures:
     * If current working clock slower than desired working clock, Please follow the procedure below
     * 1. Change Memory Clock first
     * 2. Then change System Clock
     *
     * Following example shows to speed up clock case. User can specify different
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
                      240000000,            //UINT32 u32PllKHz,
                      240000000);           //UINT32 u32SysKHz,
    sysSetCPUClock(240000000);
#endif

    sysSetTimerReferenceClock(CONFIG_SYS_TIMER, CONFIG_EXTERN_FREQUENCY);
    sysStartTimer(CONFIG_SYS_TIMER, CONFIG_TICK_PER_SECOND, PERIODIC_MODE);

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
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
#if 1
        sysDelay(LV_DISP_DEF_REFR_PERIOD);
#else
        // Put your code here  __start.
        // Put your code here  __eend.
#endif
    }

    return 0;
}
