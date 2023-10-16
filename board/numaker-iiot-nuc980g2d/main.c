/**************************************************************************//**
 * @file     main.c
 * @brief    NUC980 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

static void sys_init(void)
{
    SYS_UnlockReg();

    /* Cache on. */
    if (! sysGetCacheState())
    {
        sysInvalidCache();
        sysEnableCache(CACHE_WRITE_BACK);
        sysFlushCache(I_D_CACHE);
    }

    /* Enable clocks */
    nu_sys_ipclk_enable(CPUCKEN);
    nu_sys_ipclk_enable(HCLKCKEN);
    nu_sys_ipclk_enable(HCLK1CKEN);
    nu_sys_ipclk_enable(HCLK3CKEN);
    nu_sys_ipclk_enable(HCLK4CKEN);
    nu_sys_ipclk_enable(PCLK0CKEN);
    nu_sys_ipclk_enable(PCLK1CKEN);
    nu_sys_ipclk_enable(SRAMCKEN);
    nu_sys_ipclk_enable(SDICCKEN);
    nu_sys_ipclk_enable(PCLK2CKEN);
    nu_sys_ipclk_enable(PCLKEN0_Reserved_3);

    /* Enable UART0 clock */
    nu_sys_ipclk_enable(UART0CKEN);

    /* Enable GPIO clock */
    nu_sys_ipclk_enable(GPIOCKEN);

    /* Enable SPI0 clock */
    nu_sys_ipclk_enable(SPI0CKEN);

    /* Enable timer5 clock */
    nu_sys_ipclk_enable(TIMER5CKEN);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* ILI9341 PIN, BACKLIGHT, DC, RESET. PF9, PF10, PG11 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & ~0x00000FF0));
    outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL) & ~0xF0000000));

    /* SPI0 pins multi-function pins */
    outpw(REG_SYS_GPD_MFPH, (inpw(REG_SYS_GPD_MFPH) & ~0x0000FFFF) | 0x00001111);

    UART_Open(UART0, 115200);

    systick_init();

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

#if LV_USE_LOG
static void lv_nuvoton_log(const char *buf)
{
    printf("%s", buf);
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
