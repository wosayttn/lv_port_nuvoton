/**************************************************************************//**
 * @file     main.c
 * @brief    N9H31 Driver Sample Code
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

    // Configure multi-function pin for LCD interface
    //GPG6 (CLK), GPG7 (HSYNC)
    outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL) & ~0xFF000000) | 0x22000000);
    //GPG8 (VSYNC), GPG9 (DEN)
    outpw(REG_SYS_GPG_MFPH, (inpw(REG_SYS_GPG_MFPH) & ~0xFF) | 0x22);

    //DATA pin
    /* GPA0 ~ GPA7 (DATA0~7) */
    outpw(REG_SYS_GPA_MFPL, 0x22222222);
    /* GPA8 ~ GPA15 (DATA8~15) */
    outpw(REG_SYS_GPA_MFPH, 0x22222222);
    /* GPD8 ~ GPD15 (DATA16~23) */
    outpw(REG_SYS_GPD_MFPH, 0x22222222);

#if (LV_COLOR_DEPTH==32)
    /* LCD clock is selected from UPLL and divide to 20MHz */
    outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0xE18);

    /* LCD clock is selected from UPLL and divide to 30MHz */
    //outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0x918);

    /* LCD clock is selected from UPLL and divide to 33.3MHz */
    //outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0x818);
#elif (LV_COLOR_DEPTH==16)
    /* LCD clock is selected from UPLL and divide to 10MHz */
    outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0xE18);
#else
#error "Wrong LV_COLOR_DEPTH definition. Please correct".
#endif

    /* Close WDT first, to avoid WDT timer is enabled IBR timeout reset. */
    outpw(REG_WDT_CTL, 0);

    /* LCD_PWM */
    outpw(REG_SYS_GPH_MFPL, (inpw(REG_SYS_GPH_MFPL) & ~0xF00) | 0x0);
    GPIO_OpenBit(GPIOH, BIT2, DIR_OUTPUT, NO_PULL_UP);
    GPIO_ClrBit(GPIOH, BIT2);

    sysInitializeUART();
    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

int main(void)
{
    int task_lv_init(void);

    sys_init();

    task_lv_init();

    /* Start scheduling. */
    vTaskStartScheduler();

    for (;;);

    return 0;
}
