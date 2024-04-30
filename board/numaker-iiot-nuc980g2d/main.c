/**************************************************************************//**
 * @file     main.c
 * @brief    bring up lvgl.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

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

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* ILI9341 PIN, BACKLIGHT, DC, RESET. PF9, PF10, PG11 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & ~0x00000FF0));
    outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL) & ~0xF0000000));

    /* SPI0 pins multi-function pins */
    outpw(REG_SYS_GPD_MFPH, (inpw(REG_SYS_GPD_MFPH) & ~0x0000FFFF) | 0x00001111);

    UART_Open(UART0, 115200);

    sysSetLocalInterrupt(ENABLE_IRQ);   // Enable CPSR I bit
}

void test_task1(void *pdata)
{
    while (1)
    {
        printf("%s\n", __func__);
        vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
    }
}
void test_task2(void *pdata)
{
    while (1)
    {
        printf("%s\n", __func__);
        vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
    }
}


int task_test_init(void)
{
    xTaskCreate(test_task1, "test_task1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(test_task2, "test_task2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    return 0;
}


int main(void)
{
    sys_init();

    int task_lv_init(void);
    task_lv_init();

    /* Start scheduling. */
    vTaskStartScheduler();

    for (;;);

    return 0;
}
