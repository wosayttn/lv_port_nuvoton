/**************************************************************************//**
 * @file     main.c
 * @brief    M2354 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
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
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA7MFP_UART0_TXD | SYS_GPA_MFPL_PA6MFP_UART0_RXD);

    /* Enable EADC0 module clock/PIN */
    CLK_EnableModuleClock(EADC_MODULE);
    CLK_SetModuleClock(EADC_MODULE, MODULE_NoMsk, CLK_CLKDIV0_EADC(8));
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB11MFP_Msk | SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB7MFP_Msk | SYS_GPB_MFPL_PB6MFP_Msk);

    /* Enable digital path on these EADC pins */
    GPIO_ENABLE_DIGITAL_PATH(PB, BIT6 | BIT7 | BIT8 | BIT9 | BIT10 | BIT11);

    /* Enable SPI0 module clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
    SYS_ResetModule(SPI0_RST);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA0MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA3MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | SYS_GPA_MFPL_PA1MFP_SPI0_MISO | SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA3MFP_SPI0_SS);
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT3, GPIO_SLEWCTL_HIGH);

    /* Pull-up MISO1 & MOSI1 pins */
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC9MFP_Msk | SYS_GPC_MFPH_PC10MFP_Msk);
    GPIO_SetMode(PC, BIT9 | BIT10, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT9 | BIT10, GPIO_PUSEL_PULL_UP);

    /* RESET ESP12 to avoid power issue. */
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC13MFP_Msk);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    PC13 = 0;

    /* RESET BUTTON to avoid power issue. */
    SYS->GPF_MFPH &= ~(SYS_GPF_MFPH_PF11MFP_Msk);
    GPIO_SetMode(PF, BIT11, GPIO_MODE_INPUT);

    /* Enable SPI1 module clock */
    CLK_EnableModuleClock(SPI1_MODULE);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);
    SYS_ResetModule(SPI1_RST);

    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE1MFP_SPI1_MISO | SYS_GPE_MFPL_PE0MFP_SPI1_MOSI);
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH8MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH9MFP_SPI1_SS | SYS_GPH_MFPH_PH8MFP_SPI1_CLK);

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
    sys_init();

    /* Initial fatfs and access SPI-NOR flash device on NuTFT board. */
    /* Notice: Due to PA0, PA1, PA2, PA3 pins(SPI0) is in 1.8v power-domain. */
    /* You should switch 3.3v domain to access the W25X16 SPI NOR flash of Nu-TFT board. */
    task_fatfs_init();

    /* If button pressed, to enter USB MSC mode. */
    /* If button release, to enter LVGL demo mode. */
    if (PF11 == 0)
        task_msc_sfd_init();
    else
        task_lv_init();

    /* Start scheduling. */
    vTaskStartScheduler();

    for (;;);

    return 0;
}
