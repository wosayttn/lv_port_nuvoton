/**************************************************************************//**
 * @file     main.c
 * @brief    M460 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

static void sys_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk);

    /* Set core clock to 192MHz */
    CLK_SetCoreClock(FREQ_192MHZ);

    /* Set PCLK-related clock */
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
    CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Enable EBI module clock */
    CLK_EnableModuleClock(EBI_MODULE);

    /* Enable I2C1 module clock */
    CLK_EnableModuleClock(I2C1_MODULE);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP3 &= ~(SYS_GPB_MFP3_PB13MFP_Msk | SYS_GPB_MFP3_PB12MFP_Msk);
    SYS->GPB_MFP3 |= (SYS_GPB_MFP3_PB13MFP_UART0_TXD | SYS_GPB_MFP3_PB12MFP_UART0_RXD);

    /* EBI */
    SYS->GPC_MFP0 &= ~(SYS_GPC_MFP0_PC3MFP_Msk | SYS_GPC_MFP0_PC2MFP_Msk | SYS_GPC_MFP0_PC1MFP_Msk | SYS_GPC_MFP0_PC0MFP_Msk);
    SYS->GPC_MFP0 |= (SYS_GPC_MFP0_PC3MFP_EBI_AD3 | SYS_GPC_MFP0_PC2MFP_EBI_AD2 | SYS_GPC_MFP0_PC1MFP_EBI_AD1 | SYS_GPC_MFP0_PC0MFP_EBI_AD0);
    SYS->GPC_MFP1 &= ~(SYS_GPC_MFP1_PC5MFP_Msk | SYS_GPC_MFP1_PC4MFP_Msk);
    SYS->GPC_MFP1 |= (SYS_GPC_MFP1_PC5MFP_EBI_AD5 | SYS_GPC_MFP1_PC4MFP_EBI_AD4);
    SYS->GPD_MFP2 &= ~(SYS_GPD_MFP2_PD9MFP_Msk | SYS_GPD_MFP2_PD8MFP_Msk);
    SYS->GPD_MFP2 |= (SYS_GPD_MFP2_PD9MFP_EBI_AD7 | SYS_GPD_MFP2_PD8MFP_EBI_AD6);
    SYS->GPD_MFP3 &= ~(SYS_GPD_MFP3_PD14MFP_Msk);
    SYS->GPD_MFP3 |= (SYS_GPD_MFP3_PD14MFP_EBI_nCS0);
    SYS->GPE_MFP0 &= ~(SYS_GPE_MFP0_PE1MFP_Msk | SYS_GPE_MFP0_PE0MFP_Msk);
    SYS->GPE_MFP0 |= (SYS_GPE_MFP0_PE1MFP_EBI_AD10 | SYS_GPE_MFP0_PE0MFP_EBI_AD11);
    SYS->GPE_MFP3 &= ~(SYS_GPE_MFP3_PE15MFP_Msk | SYS_GPE_MFP3_PE14MFP_Msk);
    SYS->GPE_MFP3 |= (SYS_GPE_MFP3_PE15MFP_EBI_AD9 | SYS_GPE_MFP3_PE14MFP_EBI_AD8);
    SYS->GPH_MFP2 &= ~(SYS_GPH_MFP2_PH11MFP_Msk | SYS_GPH_MFP2_PH10MFP_Msk | SYS_GPH_MFP2_PH9MFP_Msk | SYS_GPH_MFP2_PH8MFP_Msk);
    SYS->GPH_MFP2 |= (SYS_GPH_MFP2_PH11MFP_EBI_AD15 | SYS_GPH_MFP2_PH10MFP_EBI_AD14 | SYS_GPH_MFP2_PH9MFP_EBI_AD13 | SYS_GPH_MFP2_PH8MFP_EBI_AD12);
    SYS->GPJ_MFP2 &= ~(SYS_GPJ_MFP2_PJ9MFP_Msk | SYS_GPJ_MFP2_PJ8MFP_Msk);
    SYS->GPJ_MFP2 |= (SYS_GPJ_MFP2_PJ9MFP_EBI_nWR | SYS_GPJ_MFP2_PJ8MFP_EBI_nRD);

    GPIO_SetSlewCtl(PC, (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PD, (BIT8 | BIT9), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PE, (BIT14 | BIT15), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PE, (BIT0 | BIT1), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PH, (BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PJ, (BIT8 | BIT9), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PD, BIT14, GPIO_SLEWCTL_FAST);

    /* I2C1 */
    SYS->GPB_MFP2 &= ~(SYS_GPB_MFP2_PB11MFP_Msk | SYS_GPB_MFP2_PB10MFP_Msk);
    SYS->GPB_MFP2 |= (SYS_GPB_MFP2_PB11MFP_I2C1_SCL | SYS_GPB_MFP2_PB10MFP_I2C1_SDA);

    GPIO_SetPullCtl(PB, BIT11 | BIT10, GPIO_PUSEL_PULL_UP);

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
