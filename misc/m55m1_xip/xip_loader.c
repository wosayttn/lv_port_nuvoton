/**************************************************************************//**
 * @file     main.c
 * @brief    M55M1 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "NuMicro.h"

/* HyperFlash need the CLK pin. */
#define HYPERRAM_DIFF_CLK    DEF_USE_SPIM_HYPERFLASH

void SetDebugUartCLK(void)
{
#if (!defined(DEBUG_ENABLE_SEMIHOST) || (DEBUG_ENABLE_SEMIHOST == 1)) && !defined(OS_USE_SEMIHOSTING)

    /* Select UARTn clock source from HIRC */
    CLK_SetModuleClock(DEBUG_PORT_MODULE, DEBUG_PORT_CLKSEL, DEBUG_PORT_CLKDIV);

    /* Enable UART clock */
    CLK_EnableModuleClock(DEBUG_PORT_MODULE);

    /* Reset UART module */
    SYS_ResetModule(DEBUG_PORT_RST);

#endif
}

static void sys_init(void)
{
    /*
        Set I/O slew rate to FAST1 (100 MHz).
        Use FAST1 if targeting 1.8V devices for better timing margin.
        Adjust if signal issues or EMI are observed.
    */
    uint32_t u32SlewRate = GPIO_SLEWCTL_FAST1;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable PLL0 200MHz clock */
    CLK_EnableAPLL(CLK_APLLCTL_APLLSRC_HIRC, FREQ_200MHZ, CLK_APLL0_SELECT);

    /* Switch SCLK clock source to PLL0 and divide 1 */
    CLK_SetSCLK(CLK_SCLKSEL_SCLKSEL_APLL0);

    /* Set HCLK2 divide 2 */
    CLK_SET_HCLK2DIV(2);

    /* Set PCLKx divide 2 */
    CLK_SET_PCLK0DIV(2);
    CLK_SET_PCLK1DIV(2);
    CLK_SET_PCLK2DIV(2);
    CLK_SET_PCLK3DIV(2);
    CLK_SET_PCLK4DIV(2);

    /* Enable SPIM module clock */
    CLK_EnableModuleClock(SPIM0_MODULE);
    CLK_EnableModuleClock(OTFC0_MODULE);

    /* Enable GPIO Module clock */
    CLK_EnableModuleClock(GPIOA_MODULE);
    CLK_EnableModuleClock(GPIOB_MODULE);
    CLK_EnableModuleClock(GPIOC_MODULE);
    CLK_EnableModuleClock(GPIOD_MODULE);
    CLK_EnableModuleClock(GPIOE_MODULE);
    CLK_EnableModuleClock(GPIOF_MODULE);
    CLK_EnableModuleClock(GPIOG_MODULE);
    CLK_EnableModuleClock(GPIOH_MODULE);
    CLK_EnableModuleClock(GPIOI_MODULE);
    CLK_EnableModuleClock(GPIOJ_MODULE);

    /* Init SPIM multi-function pins */
    /*
        HyperRAM CLK# usage depends on clock mode:
        Differential: CK and CK# both used (enable CLKN pin).
        Single-ended: CK# unused (do not set CLKN).
        Refer to HyperRAM datasheet before enabling CLKN.
    */
#if HYPERRAM_DIFF_CLK
    /* Differential mode: CK# required enable CLKN pin */
    /* See note above and ensure CK# is connected or tied to VCCQ/VSSQ (not floating). */
    SET_SPIM0_CLKN_PH12();
#else
    /* Single-ended mode: CK# not used by device (CR1[6]=1 default).
       If CK# not routed, tie to VCCQ or VSSQ to avoid floating on the PCB. */
    /* No CLKN mux needed */
#endif

    SET_SPIM0_CLK_PH13();
    SET_SPIM0_D2_PJ5();
    SET_SPIM0_D3_PJ6();
    SET_SPIM0_D4_PH14();
    SET_SPIM0_D5_PH15();
    SET_SPIM0_D6_PG13();
    SET_SPIM0_D7_PG14();
    SET_SPIM0_MISO_PJ4();
    SET_SPIM0_MOSI_PJ3();
    SET_SPIM0_RESETN_PJ2();
    SET_SPIM0_RWDS_PG15();
    SET_SPIM0_SS_PJ7();

    PG->SMTEN |= (GPIO_SMTEN_SMTEN13_Msk |
                  GPIO_SMTEN_SMTEN14_Msk |
                  GPIO_SMTEN_SMTEN15_Msk);
    PH->SMTEN |= (GPIO_SMTEN_SMTEN12_Msk |
                  GPIO_SMTEN_SMTEN13_Msk |
                  GPIO_SMTEN_SMTEN14_Msk |
                  GPIO_SMTEN_SMTEN15_Msk);
    PJ->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk |
                  GPIO_SMTEN_SMTEN3_Msk |
                  GPIO_SMTEN_SMTEN4_Msk |
                  GPIO_SMTEN_SMTEN5_Msk |
                  GPIO_SMTEN_SMTEN6_Msk |
                  GPIO_SMTEN_SMTEN7_Msk);

    /* Set SPIM I/O pins as high slew rate up to 80 MHz. */
    GPIO_SetSlewCtl(PG, BIT13, u32SlewRate);
    GPIO_SetSlewCtl(PG, BIT14, u32SlewRate);
    GPIO_SetSlewCtl(PG, BIT15, u32SlewRate);

    GPIO_SetSlewCtl(PH, BIT12, u32SlewRate);
    GPIO_SetSlewCtl(PH, BIT13, u32SlewRate);
    GPIO_SetSlewCtl(PH, BIT14, u32SlewRate);
    GPIO_SetSlewCtl(PH, BIT15, u32SlewRate);

    GPIO_SetSlewCtl(PJ, BIT2, u32SlewRate);
    GPIO_SetSlewCtl(PJ, BIT3, u32SlewRate);
    GPIO_SetSlewCtl(PJ, BIT4, u32SlewRate);
    GPIO_SetSlewCtl(PJ, BIT5, u32SlewRate);
    GPIO_SetSlewCtl(PJ, BIT6, u32SlewRate);
    GPIO_SetSlewCtl(PJ, BIT7, u32SlewRate);	

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Enable UART0 module clock */
    SetDebugUartCLK();

    SetDebugUartMFP();

    InitDebugUart();

#if (DEF_USE_SPIM_NORFLASH==1)
    extern void SPIM_NorFlash_Init(SPIM_T * spim);
    SPIM_NorFlash_Init(SPIM0);
#elif (DEF_USE_SPIM_HYPERFLASH==1)
    void SPIM_HyperFlash_Init(SPIM_T *spim);
    SPIM_HyperFlash_Init(SPIM0);
#endif
}

int main(void)
{
    sys_init();

    printf("SPIM DMM initialized.\n");

    /* Jump to Reset_Handler in SPIM0 XIP region (dual image) */
    {
        volatile uint32_t *SPIM_VECTBL = (volatile uint32_t *)SPIM_DMM0_SADDR;
        uint32_t u32SP  = SPIM_VECTBL[0];  /* Initial SP */
        uint32_t u32PC  = SPIM_VECTBL[1];  /* Reset_Handler address */

        printf("Jump to SPIM code: SP=0x%08x, PC=0x%08x\n", u32SP, u32PC);

        /* Relocate vector table to SPIM image */
        SCB->VTOR = (uint32_t)SPIM_DMM0_SADDR;
        __DSB();
        __ISB();

        ((void (*)(void))u32PC)();
    }

    /* Should never reach here */
    while (1);
}
