/**************************************************************************//**
 * @file     main.c
 * @brief    M480 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

static void sys_init(void)
{
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk);

    /* Disable PLL first to avoid unstable when setting PLL */
    CLK_DisablePLL();

    /* Set PLL frequency */
    CLK->PLLCTL = (CLK->PLLCTL & ~(0x000FFFFFul)) | 0x0000421Eul;

    /* Waiting for PLL ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* If the defines do not exist in your project, please refer to the related clk.h in the Header folder appended to the tool package. */
    /* Set HCLK clock */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));

    /* Set PCLK-related clock */
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2);

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Enable EADC0 module clock */
    CLK_EnableModuleClock(EADC_MODULE);
    CLK_SetModuleClock(EADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_EADC(8));

    /* Enable EBI module clock */
    CLK_EnableModuleClock(EBI_MODULE);

    /* Enable PDMA module clock */
    CLK_EnableModuleClock(PDMA_MODULE);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* EBI */
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA8MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA8MFP_EBI_ALE);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD14MFP_Msk | SYS_GPD_MFPH_PD11MFP_Msk | SYS_GPD_MFPH_PD9MFP_Msk | SYS_GPD_MFPH_PD8MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD14MFP_EBI_nCS0 | SYS_GPD_MFPH_PD11MFP_EBI_nCS1 | SYS_GPD_MFPH_PD9MFP_EBI_AD7 | SYS_GPD_MFPH_PD8MFP_EBI_AD6);
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE15MFP_Msk | SYS_GPE_MFPH_PE14MFP_Msk);
    SYS->GPE_MFPH |= (SYS_GPE_MFPH_PE15MFP_EBI_AD9 | SYS_GPE_MFPH_PE14MFP_EBI_AD8);
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE5MFP_Msk | SYS_GPE_MFPL_PE4MFP_Msk | SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE5MFP_EBI_nRD | SYS_GPE_MFPL_PE4MFP_EBI_nWR | SYS_GPE_MFPL_PE1MFP_EBI_AD10 | SYS_GPE_MFPL_PE0MFP_EBI_AD11);
    SYS->GPG_MFPH &= ~(SYS_GPG_MFPH_PG14MFP_Msk | SYS_GPG_MFPH_PG13MFP_Msk | SYS_GPG_MFPH_PG12MFP_Msk | SYS_GPG_MFPH_PG11MFP_Msk | SYS_GPG_MFPH_PG10MFP_Msk | SYS_GPG_MFPH_PG9MFP_Msk | SYS_GPG_MFPH_PG8MFP_Msk);
    SYS->GPG_MFPH |= (SYS_GPG_MFPH_PG14MFP_EBI_AD5 | SYS_GPG_MFPH_PG13MFP_EBI_AD4 | SYS_GPG_MFPH_PG12MFP_EBI_AD3 | SYS_GPG_MFPH_PG11MFP_EBI_AD2 | SYS_GPG_MFPH_PG10MFP_EBI_AD1 | SYS_GPG_MFPH_PG9MFP_EBI_AD0 | SYS_GPG_MFPH_PG8MFP_EBI_nWRH);
    SYS->GPG_MFPL &= ~(SYS_GPG_MFPL_PG7MFP_Msk);
    SYS->GPG_MFPL |= (SYS_GPG_MFPL_PG7MFP_EBI_nWRL);
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH11MFP_Msk | SYS_GPH_MFPH_PH10MFP_Msk | SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH8MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH11MFP_EBI_AD15 | SYS_GPH_MFPH_PH10MFP_EBI_AD14 | SYS_GPH_MFPH_PH9MFP_EBI_AD13 | SYS_GPH_MFPH_PH8MFP_EBI_AD12);

    /*EADC*/
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB9MFP_EADC0_CH9 | SYS_GPB_MFPH_PB8MFP_EADC0_CH8);

    UART_Open(UART0, 115200);

    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_3_0V);

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
