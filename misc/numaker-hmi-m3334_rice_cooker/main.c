/**************************************************************************//**
 * @file     main.c
 * @brief    M3331 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include <stdint.h>

static void sys_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    SYS_SetPowerLevel(SYS_PLCTL_PLSEL_PL0);

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk);

    /* Set core clock to __HSI */
    CLK_SetCoreClock(180000000);

    /* Set PCLK-related clock */
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV1 | CLK_PCLKDIV_PCLK1DIV1);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Enable PDMA0 module clock */
    CLK_EnableModuleClock(PDMA0_MODULE);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, 0);

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* EADC Analog Pin */
    CLK_EnableModuleClock(EADC0_MODULE);
    CLK_SetModuleClock(EADC0_MODULE, CLK_CLKSEL0_EADC0SEL_PLL_DIV2, CLK_CLKDIV0_EADC0(8));

#if defined(NUFUN) && (NUFUN==1)

    /* SPI1 */
    CLK_EnableModuleClock(SPI1_MODULE);

    SYS->GPE_MFP0 &= ~(SYS_GPE_MFP0_PE1MFP_Msk | SYS_GPE_MFP0_PE0MFP_Msk);
    SYS->GPE_MFP0 |= (SYS_GPE_MFP0_PE1MFP_SPI1_MISO | SYS_GPE_MFP0_PE0MFP_SPI1_MOSI);
    SYS->GPH_MFP2 &= ~(SYS_GPH_MFP2_PH9MFP_Msk | SYS_GPH_MFP2_PH8MFP_Msk);
    SYS->GPH_MFP2 |= (SYS_GPH_MFP2_PH9MFP_SPI1_SS | SYS_GPH_MFP2_PH8MFP_SPI1_CLK);

    SYS->GPB_MFP0 &= ~(SYS_GPB_MFP0_PB2MFP_Msk | SYS_GPB_MFP0_PB3MFP_Msk);
    SYS->GPB_MFP0 |= (SYS_GPB_MFP0_PB2MFP_EADC0_CH2 | SYS_GPB_MFP0_PB3MFP_EADC0_CH3);
    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP1_PB4MFP_Msk | SYS_GPB_MFP1_PB5MFP_Msk);
    SYS->GPB_MFP1 |= (SYS_GPB_MFP1_PB4MFP_EADC0_CH4 | SYS_GPB_MFP1_PB5MFP_EADC0_CH5);

    /* Disable digital path on these EADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT2 | BIT3 | BIT4 | BIT5);

#else

    /* SPI2 */
    CLK_EnableModuleClock(SPI2_MODULE);

    SYS->GPA_MFP2 &= ~(SYS_GPA_MFP2_PA11MFP_Msk | SYS_GPA_MFP2_PA10MFP_Msk | SYS_GPA_MFP2_PA9MFP_Msk | SYS_GPA_MFP2_PA8MFP_Msk);
    SYS->GPA_MFP2 |= (SYS_GPA_MFP2_PA11MFP_SPI2_SS | SYS_GPA_MFP2_PA10MFP_SPI2_CLK | SYS_GPA_MFP2_PA9MFP_SPI2_MISO | SYS_GPA_MFP2_PA8MFP_SPI2_MOSI);

    /* EADC Analog Pin: UNO_A0, UNO_A1, UNO_A2, UNO_A3 */
    SYS->GPB_MFP1 &= ~(SYS_GPB_MFP3_PB14MFP_Msk | SYS_GPB_MFP3_PB15MFP_Msk);
    SYS->GPB_MFP1 |= (SYS_GPB_MFP3_PB14MFP_EADC0_CH14 | SYS_GPB_MFP3_PB15MFP_EADC0_CH15);
    SYS->GPB_MFP2 &= ~(SYS_GPB_MFP2_PB9MFP_Msk | SYS_GPB_MFP2_PB8MFP_Msk);
    SYS->GPB_MFP2 |= (SYS_GPB_MFP2_PB9MFP_EADC0_CH9 | SYS_GPB_MFP2_PB8MFP_EADC0_CH8);

    /* Disable digital path on these EADC pins */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT8 | BIT9 | BIT14 | BIT15);

#endif

    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_PIN);

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
