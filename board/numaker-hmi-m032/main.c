/**************************************************************************//**
 * @file     main.c
 * @brief    M032 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

static void sys_init(void)
{
    SYS_UnlockReg();

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk | CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk | CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk);

    /* Disable PLL first to avoid unstable when setting PLL */
    CLK_DisablePLL();

    /* Set PLL frequency */
    CLK->PLLCTL = (CLK->PLLCTL & ~(0x000FFFFFul)) | 0x0008C02Eul;

    /* Waiting for PLL ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* If the defines do not exist in your project, please refer to the related clk.h in the Header folder appended to the tool package. */
    /* Set HCLK clock */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Set PCLK-related clock */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1);

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB12MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB13MFP_UART0_TXD | SYS_GPB_MFPH_PB12MFP_UART0_RXD);

    /* Enable EADC0 module clock/PIN */
    CLK_EnableModuleClock(ADC_MODULE);
    CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL2_ADCSEL_PCLK1, CLK_CLKDIV0_ADC(1));
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk |
                       SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB6MFP_Msk | SYS_GPB_MFPL_PB7MFP_Msk);

    /* Enable digital path on these ADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);

    /* Enable USPI1 module clock */
    CLK_EnableModuleClock(USCI0_MODULE);
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA8MFP_Msk | SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk);
    SYS->GPA_MFPH |= (/*SYS_GPA_MFPH_PA8MFP_USCI0_CTL1 |*/ SYS_GPA_MFPH_PA9MFP_USCI0_DAT1 | SYS_GPA_MFPH_PA10MFP_USCI0_DAT0 | SYS_GPA_MFPH_PA11MFP_USCI0_CLK);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    CLK_EnableModuleClock(PDMA_MODULE);

    UART_Open(UART0, 115200);

    systick_init();
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
