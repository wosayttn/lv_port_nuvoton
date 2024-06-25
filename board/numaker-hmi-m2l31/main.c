/**************************************************************************//**
 * @file     main.c
 * @brief    main routine.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

static void sys_init(void)
{
    SYS_UnlockReg();

    /* Enable HIRC/HXT/LXT/LIRC/HIRC48M clocks */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_LIRCEN_Msk | CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HIRC48MEN_Msk);

    /* Wait for HIRC/HXT/LXT/LIRC/HIRC48M clocks ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_LIRCSTB_Msk | CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_HIRC48MSTB_Msk);

    /* Set PLL to 144MHz */
    CLK_EnablePLL(CLK_PLLCTL_PLLSRC_HXT, FREQ_144MHZ);

    /* Set CPU clock to 72MHz */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));

    /* Set PCLK0 = PCLK1 = HCLK */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1);

    /* Set PCLK0 = PCLK1 = HCLK/2 */
    //CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    SystemCoreClockUpdate();

    CLK_EnableModuleClock(GPA_MODULE);
    CLK_EnableModuleClock(GPB_MODULE);
    CLK_EnableModuleClock(GPC_MODULE);
    CLK_EnableModuleClock(GPD_MODULE);
    CLK_EnableModuleClock(GPE_MODULE);
    CLK_EnableModuleClock(GPF_MODULE);
    CLK_EnableModuleClock(GPG_MODULE);
    CLK_EnableModuleClock(GPH_MODULE);

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL4_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    SYS->GPB_MFP3 &= ~(SYS_GPB_MFP3_PB12MFP_Msk | SYS_GPB_MFP3_PB13MFP_Msk);
    SYS->GPB_MFP3 |= (SYS_GPB_MFP3_PB13MFP_UART0_TXD | SYS_GPB_MFP3_PB12MFP_UART0_RXD);
    UART_Open(UART0, 115200);

    /* Enable EADC0 module clock/PIN */
    CLK_EnableModuleClock(EADC0_MODULE);
    CLK_SetModuleClock(EADC0_MODULE, CLK_CLKSEL0_EADC0SEL_HIRC, CLK_CLKDIV0_EADC0(2));
    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP1_PB4MFP_Msk | SYS_GPB_MFP1_PB5MFP_Msk | SYS_GPB_MFP1_PB6MFP_Msk | SYS_GPB_MFP1_PB7MFP_Msk);
    SYS->GPB_MFP1 |= (SYS_GPB_MFP1_PB4MFP_EADC0_CH4 | SYS_GPB_MFP1_PB5MFP_EADC0_CH5 | SYS_GPB_MFP1_PB6MFP_EADC0_CH6 | SYS_GPB_MFP1_PB7MFP_EADC0_CH7);

    /* Disable digital path on these ADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT4 | BIT5 | BIT6 | BIT7);

    /* Enable SPI0 module clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    SYS_ResetModule(SPI0_RST);
    SYS->GPD_MFP0 &= ~(SYS_GPD_MFP0_PD0MFP_Msk | SYS_GPD_MFP0_PD1MFP_Msk | SYS_GPD_MFP0_PD2MFP_Msk | SYS_GPD_MFP0_PD3MFP_Msk);
    SYS->GPD_MFP0 |= (SYS_GPD_MFP0_PD0MFP_SPI0_MOSI | SYS_GPD_MFP0_PD1MFP_SPI0_MISO | SYS_GPD_MFP0_PD2MFP_SPI0_CLK | SYS_GPD_MFP0_PD3MFP_SPI0_SS);

    /* ILI9341 LCD panel */
    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA8MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk | SYS_GPA_MFP2_PA11MFP_Msk);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HXT_DIV2, 0);

    /* Enable PDMA0 module clock */
    CLK_EnableModuleClock(PDMA0_MODULE);

    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_PIN);
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
