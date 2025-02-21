/**************************************************************************//**
 * @file     main.c
 * @brief    M55M1 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "NuMicro.h"
#include "tc.h"

void SetDebugUartCLK(void)
{
    /* Select UART0 clock source from HXT */
    CLK_SetModuleClock(DEBUG_PORT_MODULE, CLK_UARTSEL0_UART0SEL_HIRC, CLK_UARTDIV0_UART0DIV(1));

    /* Enable UART clock */
    CLK_EnableModuleClock(DEBUG_PORT_MODULE);

    /* Reset UART module */
    SYS_ResetModule(DEBUG_PORT_RST);
}

static void sys_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

#if (_DEBUG==1)
    /* Enable PLL0 clock from HXT and switch SCLK clock source to PLL0 */
    CLK_SetBusClock(CLK_SCLKSEL_SCLKSEL_APLL0, CLK_APLLCTL_APLLSRC_HIRC, FREQ_220MHZ);
#else
    /* Enable Internal RC 12MHz clock */
    CLK_EnableXtalRC(CLK_SRCCTL_HIRCEN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Enable PLL0 clock */
    CLK_EnableAPLL(CLK_APLLCTL_APLLSRC_HIRC, FREQ_200MHZ, CLK_APLL0_SELECT);

    /* Switch SCLK clock source to PLL0 and divide 1 */
    CLK_SetSCLK(CLK_SCLKSEL_SCLKSEL_APLL0);
#endif

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    CLK_EnableModuleClock(GPIOD_MODULE);
    CLK_EnableModuleClock(GPIOG_MODULE);
    CLK_EnableModuleClock(GPIOH_MODULE);
    CLK_EnableModuleClock(GPIOJ_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

#if (_DEBUG==1)
    /* Enable UART0 module clock */
    SetDebugUartCLK();

    SetDebugUartMFP();

    InitDebugUart();
#endif

    /* Enable SPIM module clock */
    CLK_EnableModuleClock(SPIM0_MODULE);

    /* Enable SPIM module clock */
    CLK_EnableModuleClock(OTFC0_MODULE);

    /* Init SPIM multi-function pins */
    //SET_SPIM0_CLKN_PH12();
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
                  GPIO_SMTEN_SMTEN14_Msk 
                  /*| GPIO_SMTEN_SMTEN15_Msk*/);
    PH->SMTEN |= (/*GPIO_SMTEN_SMTEN12_Msk |
                  GPIO_SMTEN_SMTEN13_Msk |*/
                  GPIO_SMTEN_SMTEN14_Msk |
                  GPIO_SMTEN_SMTEN15_Msk);
    PJ->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk |
                  GPIO_SMTEN_SMTEN3_Msk |
                  GPIO_SMTEN_SMTEN4_Msk |
                  GPIO_SMTEN_SMTEN5_Msk |
                  GPIO_SMTEN_SMTEN6_Msk |
                  GPIO_SMTEN_SMTEN7_Msk);

    /* Set SPIM I/O pins as slew rate. */
    //#define CONFIG_SLEW_RATE      GPIO_SLEWCTL_FAST0
    #define CONFIG_SLEW_RATE      GPIO_SLEWCTL_FAST1

    GPIO_SetSlewCtl(PG, (BIT13 | BIT14 | BIT15), CONFIG_SLEW_RATE);
    GPIO_SetSlewCtl(PH, (/*BIT12 |*/ BIT13 | BIT14 | BIT15), CONFIG_SLEW_RATE);
    GPIO_SetSlewCtl(PJ, (BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7), CONFIG_SLEW_RATE);

    SET_GPIO_PH4();
    GPIO_SetMode(PH, BIT4, GPIO_MODE_OUTPUT);
    PH4 = 1;

    SET_GPIO_PD6();
    GPIO_SetMode(PD, BIT6, GPIO_MODE_OUTPUT);
    PD6 = 1;
}

__WEAK void HardFault_Handler(void)
{
    uint32_t u32IRQ = 0;
    struct StackFrame *psStackFrame = NULL;

    (void)u32IRQ;
    __ASM volatile("mrs %0, ipsr            \n" // Read IPSR (Exception number)
                   "sub %0, #16             \n" // Get it into IRQn_Type range
                   "tst lr, #4              \n" // Select the stack which was in use
                   "ite eq                  \n"
                   "mrseq %1, msp           \n"
                   "mrsne %1, psp           \n"
                   : "=r"(u32IRQ), "=r"(psStackFrame));

		extern void ProcessHardFault(uint32_t *pu32StackFrame);
    // Get the instruction caused the hardfault
    ProcessHardFault((uint32_t *)psStackFrame);
    TC_PRINTF("\n\nSPIM_DMM_TIMEOUT_FLAG_STS:%08x\n\n", SPIM0->DMM_TIMEOUT_FLAG_STS);
	
    // Halt here
    while (1);

}

int main(void)
{
    sys_init();

    tc_list();

    while(1)
    {
        tc_run();
        tc_report();
    }

    for (;;);

    return 0;
}
