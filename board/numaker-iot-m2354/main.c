/**************************************************************************//**
 * @file     main.c
 * @brief    main routine.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include "ff.h"
#include "diskio.h"

static void sys_init(void)
{
    SYS_UnlockReg();

    /* LXT source from external LXT */
    CLK_EnableModuleClock(RTC_MODULE);
    RTC->LXTCTL &= ~(RTC_LXTCTL_LIRC32KEN_Msk | RTC_LXTCTL_C32KSEL_Msk);
    CLK_DisableModuleClock(RTC_MODULE);

    /* Enable clock source */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LXTEN_Msk | CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_HIRC48EN_Msk | CLK_PWRCTL_MIRCEN_Msk);

    /* Waiting for clock source ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_LXTSTB_Msk | CLK_STATUS_HXTSTB_Msk | CLK_STATUS_HIRC48STB_Msk | CLK_STATUS_MIRCSTB_Msk);

    /* Disable PLL first to avoid unstable when setting PLL */
    CLK_DisablePLL();

    /* Set PLL frequency */
    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_96MHZ);

    /* Waiting for PLL ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* If the defines do not exist in your project, please refer to the related clk.h in the Header folder appended to the tool package. */
    /* Set HCLK clock */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Enable UART0 module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL2_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB8MFP_UART0_RXD | SYS_GPB_MFPH_PB9MFP_UART0_TXD);

    /* Enable EADC0 module clock/PIN */
    CLK_EnableModuleClock(EADC_MODULE);
    CLK_SetModuleClock(EADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_EADC(8));
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB7MFP_Msk | SYS_GPB_MFPL_PB6MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT4 | BIT5 | BIT6 | BIT7 );

    /* Enable QSPI0 module clock */
    CLK_EnableModuleClock(QSPI0_MODULE);
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PCLK0, MODULE_NoMsk);
    SYS_ResetModule(SPI0_RST);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA0MFP_QSPI0_MOSI0 | SYS_GPA_MFPL_PA1MFP_QSPI0_MISO0 | SYS_GPA_MFPL_PA2MFP_QSPI0_CLK | SYS_GPA_MFPL_PA3MFP_QSPI0_SS | SYS_GPA_MFPL_PA4MFP_QSPI0_MOSI1 | SYS_GPA_MFPL_PA5MFP_QSPI0_MISO1 );
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_HIGH);

    /* Enable USPI0 module clock */
    CLK_EnableModuleClock(USCI0_MODULE);
    SYS_ResetModule(USCI0_RST);
		SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk);
		SYS->GPA_MFPH |=  (SYS_GPA_MFPH_PA9MFP_USCI0_DAT1 | SYS_GPA_MFPH_PA10MFP_USCI0_DAT0 | SYS_GPA_MFPH_PA11MFP_USCI0_CLK);
		SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC13MFP_Msk);
		SYS->GPC_MFPH |=  (SYS_GPC_MFPH_PC13MFP_USCI0_CTL0);

    /* Enable SysTick module clock */
    CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HXT_DIV2, 0);

    /* Enable PDMA0/PDMA1 module clock */
    CLK_EnableModuleClock(PDMA0_MODULE);
    CLK_EnableModuleClock(PDMA1_MODULE);

    /* Select USB clock source as PLL and USB clock divider as 2 */
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_PLL, CLK_CLKDIV0_USB(2));

    /* Select USB Device role */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;

    /* Enable USBD module clock */
    CLK_EnableModuleClock(USBD_MODULE);

    /* USBD multi-function pins for VBUS, D+, D-, and ID pins */
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);

    UART_Open(UART0, 115200);

    /* Vref connect to internal */
    SYS_SetVRef(SYS_VREFCTL_VREF_3_0V);
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
