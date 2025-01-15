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
    CLK_SetBusClock(CLK_SCLKSEL_SCLKSEL_APLL0, CLK_APLLCTL_APLLSRC_HXT, FREQ_200MHZ);
#else
    /* Enable Internal RC 12MHz clock */
    CLK_EnableXtalRC(CLK_SRCCTL_HIRCEN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Enable PLL0 clock */
    CLK_EnableAPLL(CLK_APLLCTL_APLLSRC_HIRC, FREQ_220MHZ, CLK_APLL0_SELECT);

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
    SET_SPIM0_CLKN_PH12();
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

    /* Set SPIM I/O pins as slew rate. */
    #define CONFIG_SLEW_RATE      GPIO_SLEWCTL_FAST0
    //#define CONFIG_SLEW_RATE      GPIO_SLEWCTL_FAST1

    GPIO_SetSlewCtl(PG, (BIT13 | BIT14 | BIT15), CONFIG_SLEW_RATE);
    GPIO_SetSlewCtl(PH, (BIT12 | BIT13 | BIT14 | BIT15), CONFIG_SLEW_RATE);
    GPIO_SetSlewCtl(PH, (BIT14 | BIT15), CONFIG_SLEW_RATE);
    GPIO_SetSlewCtl(PJ, (BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7), CONFIG_SLEW_RATE);

    SET_GPIO_PH4();
    GPIO_SetMode(PH, BIT4, GPIO_MODE_OUTPUT);
    PH4 = 1;

    SET_GPIO_PD6();
    GPIO_SetMode(PD, BIT6, GPIO_MODE_OUTPUT);
    PD6 = 1;
}

typedef struct
{
    uint32_t u32Div;
    uint32_t u32CipherOn;
    uint32_t u32CacheOn;
    uint32_t u32ReadLatency;
    uint32_t u32WriteLatency;
    uint32_t u32CSMLT;
    uint32_t u32CSHIT;
    uint32_t u32CSHT;
    uint32_t u32CSST;
    uint32_t u32RSTLT;
} S_SPIM_HYPER;

//------------------------------------------------------------------------------
#define SPIM_HYPER_GET_CSST(spim)                                                \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSS_Msk) >> SPIM_HYPER_CONFIG1_CSS_Pos)

#define SPIM_HYPER_GET_CSH(spim)                                                 \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSH_Msk) >> SPIM_HYPER_CONFIG1_CSH_Pos)

#define SPIM_HYPER_GET_CSHI(spim)                                                 \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSHI_Msk) >> SPIM_HYPER_CONFIG1_CSHI_Pos)

static void SPIM_Hyper_CondifDump(SPIM_T *spim)
{
    uint32_t u32CoreFreq = (CLK_GetSCLKFreq() / 1000000);
    float fFreq = (float)1000.0f / u32CoreFreq;
    uint32_t u32DIV = SPIM_HYPER_GET_CLKDIV(spim);

    TC_PRINTF("[%s]\n", __func__);
    TC_PRINTF("\tHCLKFreq:%d MHz, fFreq:%f(ns), u32DIV:%d\n", u32CoreFreq, fFreq, u32DIV);

    TC_PRINTF("\tSPIM_HYPER_SET_CSST:     %f\n",   SPIM_HYPER_GET_CSST(spim) ? 4.5 : 3.5);
    TC_PRINTF("\tSPIM_HYPER_SET_CSH:      %lu\n",  SPIM_HYPER_GET_CSH(spim));
    TC_PRINTF("\tSPIM_HYPER_SET_CSHI:     %lu\n",  SPIM_HYPER_GET_CSHI(spim));
    TC_PRINTF("\tSPIM_HYPER_SET_CSMAXLT:  %lu\n",  SPIM_HYPER_GET_CSMAXLT(spim));
    TC_PRINTF("\tSPIM_HYPER_SET_RSTNLT:   %lu\n",  SPIM_HYPER_GET_RSTNLT(spim));
    TC_PRINTF("\tSPIM_HYPER_SET_ACCTRD:   %lu\n",  SPIM_HYPER_GET_ACCTRD(spim));
    TC_PRINTF("\tSPIM_HYPER_SET_ACCTWR:   %lu\n",  SPIM_HYPER_GET_ACCTWR(spim));
}

static void SPIM_HYPER_DMM_Config(SPIM_T *spim, S_SPIM_HYPER *psSPIMHyper)
{
#define CSMAXLT_CIPHER_OFF          21
#define CSMAXLT_CIPHER_ON           54

    uint32_t u32CoreFreq = (CLK_GetSCLKFreq() / 1000000);
    float fFreq = (float)1000.0f / u32CoreFreq;
    uint32_t u32CSMAXLT = psSPIMHyper->u32CSMLT / fFreq;

    SPIM_HYPER_Init(spim, SPIM_HYPERRAM_MODE, psSPIMHyper->u32Div);

    if (psSPIMHyper->u32CipherOn)
    {
        SPIM_HYPER_ENABLE_CIPHER(spim);
        u32CSMAXLT -=  CSMAXLT_CIPHER_ON;			
    }
    else
    {
        SPIM_HYPER_DISABLE_CIPHER(spim);
        u32CSMAXLT -=  CSMAXLT_CIPHER_OFF;
    }

    if (psSPIMHyper->u32CacheOn)
    {
        SPIM_HYPER_ENABLE_CACHE(spim);
    }
    else
    {
        SPIM_HYPER_DISABLE_CACHE(spim);
        u32CSMAXLT -= (2 * 8 * psSPIMHyper->u32Div);
    }

    /* Chip Select Setup Time 3.5 HCLK */
    SPIM_HYPER_SET_CSST(spim,  psSPIMHyper->u32CSST);

    /* Chip Select Hold Time HCLK */
    SPIM_HYPER_SET_CSH(spim, psSPIMHyper->u32CSHT);

    /* Chip Select High between Transaction */
    SPIM_HYPER_SET_CSHI(spim, psSPIMHyper->u32CSHIT);

    /* Chip Select Masximum low time HCLK */
    SPIM_HYPER_SET_CSMAXLT(spim, u32CSMAXLT);

    /* Initial Device RESETN Low Time 255 */
    SPIM_HYPER_SET_RSTNLT(spim, psSPIMHyper->u32RSTLT);

    /* Initial Read Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTRD(spim, psSPIMHyper->u32ReadLatency);

    /* Initial Write Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTWR(spim, psSPIMHyper->u32WriteLatency);

    SPIM_Hyper_CondifDump(spim);

    /* Reset HyperRAM */
    SPIM_HYPER_Reset(spim);

    SPIM_HYPER_SetDLLDelayNum(spim, 7);
		
    if ( 0 && (psSPIMHyper->u32ReadLatency==5) && (psSPIMHyper->u32WriteLatency==5))
    {
        uint32_t u32HRAMReg0 = SPIM_HYPER_ReadHyperRAMReg(SPIM0, SPIM_HYPER_HRAM_CONFIG_REG0);

        TC_PRINTF("Before u32HRAMReg0:%08x\n", u32HRAMReg0);

        //u32HRAMReg0 &= ~0xF0;     // 5 Clock Latency @ 133MHz Max Freq.
        u32HRAMReg0 |= 0xF0;     // 4 Clock Latency @ 104MHz Max Freq.

        SPIM_HYPER_WriteHyperRAMReg(SPIM0, SPIM_HYPER_HRAM_CONFIG_REG0, u32HRAMReg0);

        TC_PRINTF("Updated u32HRAMReg0:%08x\n", u32HRAMReg0);

        u32HRAMReg0 = SPIM_HYPER_ReadHyperRAMReg(SPIM0, SPIM_HYPER_HRAM_CONFIG_REG0);

        TC_PRINTF("After u32HRAMReg0:%08x\n", u32HRAMReg0);
    }
}


int main(void)
{
    sys_init();

    tc_list();

    extern void HyperRAM_Init(SPIM_T * spim);

#if 1
    HyperRAM_Init(SPIM0);

#if CONFIG_SPIM_CACHE_ON
    SPIM_HYPER_ENABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_ENABLE_CACHE ON!!\n");
#else
    SPIM_HYPER_DISABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_DISABLE_CACHE!!\n");
#endif
	
#else

    S_SPIM_HYPER sSPIMHyper = {0};

    sSPIMHyper.u32Div = 1;
    sSPIMHyper.u32CipherOn = 0;
    sSPIMHyper.u32CacheOn = 0;

    //sSPIMHyper.u32ReadLatency = 4;
    sSPIMHyper.u32ReadLatency = 7;
    //sSPIMHyper.u32WriteLatency = 4;
    sSPIMHyper.u32WriteLatency = 7;

    sSPIMHyper.u32CSMLT = 4000;

    //sSPIMHyper.u32CSHIT =  SPIM_HYPER_CSHI_2_HCLK;
    sSPIMHyper.u32CSHIT =  SPIM_HYPER_CSHI_3_HCLK;

    sSPIMHyper.u32CSST = SPIM_HYPER_CSST_3_5_HCLK;
    //sSPIMHyper.u32CSST = SPIM_HYPER_CSST_4_5_HCLK;

    //sSPIMHyper.u32CSHT = SPIM_HYPER_CSH_0_5_HCLK;
    //sSPIMHyper.u32CSHT = SPIM_HYPER_CSH_1_5_HCLK;
    //sSPIMHyper.u32CSHT = SPIM_HYPER_CSH_2_5_HCLK;
    sSPIMHyper.u32CSHT = SPIM_HYPER_CSST_3_5_HCLK;

    sSPIMHyper.u32RSTLT = 0xFF;

    SPIM_HYPER_DMM_Config(SPIM0, &sSPIMHyper);

#endif

    SPIM_HYPER_EnterDirectMapMode(SPIM0);

    tc_run();

    for (;;);

    return 0;
}
