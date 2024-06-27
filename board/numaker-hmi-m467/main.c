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

    /* Enable PDMA0/PDMA1 module clock */
    CLK_EnableModuleClock(PDMA0_MODULE);
    CLK_EnableModuleClock(PDMA1_MODULE);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    /* Update System Core Clock */
    SystemCoreClockUpdate();

#if defined(USE_ILI9341_SPI)

    /* SPI2 */
    CLK_EnableModuleClock(SPI2_MODULE);

    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA11MFP_Msk | SYS_GPA_MFP2_PA10MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk | SYS_GPA_MFP2_PA8MFP_Msk);
    SYS->GPA_MFP2 |= (SYS_GPA_MFP2_PA11MFP_SPI2_SS | SYS_GPA_MFP2_PA10MFP_SPI2_CLK | SYS_GPA_MFP2_PA9MFP_SPI2_MISO | SYS_GPA_MFP2_PA8MFP_SPI2_MOSI);

    /* EADC Analog Pin */
    CLK_EnableModuleClock(EADC0_MODULE);

    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP1_PB7MFP_Msk | SYS_GPB_MFP1_PB6MFP_Msk);
    SYS->GPB_MFP1 |= (SYS_GPB_MFP1_PB7MFP_EADC0_CH7 | SYS_GPB_MFP1_PB6MFP_EADC0_CH6);
    SYS->GPB_MFP2 &= ~(SYS_GPB_MFP2_PB9MFP_Msk | SYS_GPB_MFP2_PB8MFP_Msk);
    SYS->GPB_MFP2 |= (SYS_GPB_MFP2_PB9MFP_EADC0_CH9 | SYS_GPB_MFP2_PB8MFP_EADC0_CH8);

    /* Disable digital path on these EADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT6 | BIT7 | BIT8 | BIT9);

#else
    /* Enable EBI module clock */
    CLK_EnableModuleClock(EBI_MODULE);

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
    SYS->GPH_MFP1 &= ~(SYS_GPH_MFP1_PH7MFP_Msk);
    SYS->GPH_MFP1 |= (SYS_GPH_MFP1_PH7MFP_EBI_ADR0);
    SYS->GPH_MFP2 &= ~(SYS_GPH_MFP2_PH11MFP_Msk | SYS_GPH_MFP2_PH10MFP_Msk | SYS_GPH_MFP2_PH9MFP_Msk | SYS_GPH_MFP2_PH8MFP_Msk);
    SYS->GPH_MFP2 |= (SYS_GPH_MFP2_PH11MFP_EBI_AD15 | SYS_GPH_MFP2_PH10MFP_EBI_AD14 | SYS_GPH_MFP2_PH9MFP_EBI_AD13 | SYS_GPH_MFP2_PH8MFP_EBI_AD12);
    SYS->GPJ_MFP2 &= ~(SYS_GPJ_MFP2_PJ9MFP_Msk | SYS_GPJ_MFP2_PJ8MFP_Msk);
    SYS->GPJ_MFP2 |= (SYS_GPJ_MFP2_PJ9MFP_EBI_nWR | SYS_GPJ_MFP2_PJ8MFP_EBI_nRD);

    GPIO_SetSlewCtl(PC, (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PD, (BIT8 | BIT9), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PE, (BIT14 | BIT15), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PE, (BIT0 | BIT1), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PH, (BIT7 | BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PJ, (BIT8 | BIT9), GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PD, BIT14, GPIO_SLEWCTL_FAST);

    /* Enable I2C1 module clock */
    CLK_EnableModuleClock(I2C1_MODULE);

    SYS->GPB_MFP2 &= ~(SYS_GPB_MFP2_PB11MFP_Msk | SYS_GPB_MFP2_PB10MFP_Msk);
    SYS->GPB_MFP2 |= (SYS_GPB_MFP2_PB11MFP_I2C1_SCL | SYS_GPB_MFP2_PB10MFP_I2C1_SDA);

    GPIO_SetPullCtl(PB, BIT11 | BIT10, GPIO_PUSEL_PULL_UP);
#endif

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP3 &= ~(SYS_GPB_MFP3_PB13MFP_Msk | SYS_GPB_MFP3_PB12MFP_Msk);
    SYS->GPB_MFP3 |= (SYS_GPB_MFP3_PB13MFP_UART0_TXD | SYS_GPB_MFP3_PB12MFP_UART0_RXD);

    UART_Open(UART0, 115200);

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
