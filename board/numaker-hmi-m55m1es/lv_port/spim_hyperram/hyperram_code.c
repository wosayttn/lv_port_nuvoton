/**************************************************************************//**
 * @file     hyperram_code.c
 * @version  V1.00
 * @brief    Collect of sub-routines running on SPIM flash.
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMicro.h"
#include "hyperram_code.h"

//------------------------------------------------------------------------------
#define DMM_MODE_TRIM
#define DLL_TRIM_SIZE               32

#define SPIM_HYPER_DIV              1

#define HYPERRAM_CSM_TIME           4000 /* ns */
#define HYPERRAM_RD_LTCY            7
#define HYPERRAM_WR_LTCY            7
#define HYPERRAM_CSHI_CYCLE         2
#define HYPER_RAM_RST_CNT           0xFF

//------------------------------------------------------------------------------
/**
  * @brief      SPIM Default Config HyperBus Access Module Parameters.
  * @param      spim
  * @param      u32CSM    Refer to the Hyper Device Specific Chip Select Maximum (tCSM) timing parameters.
  *                       The reference Winbond HyperRAM is 4000ns.
  * @param      u32AcctRD Initial Read Access Time 1 ~ 0x1F, Default Set 0x04
  * @param      u32AcctWR Initial Write Access Time 1 ~ 0x1F, Default Set 0x04
  * @return     None.
  */
void SPIM_Hyper_DefaultConfig(SPIM_T *spim, uint32_t u32CSM, uint32_t u32AcctRD, uint32_t u32AcctWR)
{
    uint32_t u32CoreFreq = (CLK_GetSCLKFreq() / 1000000);
    float fFreq = (float)((float)1000 / (float)u32CoreFreq);
    uint32_t u32DIV = SPIM_HYPER_GET_CLKDIV(spim);
    uint32_t u32CipherEn = SPIM_HYPER_GET_CIPHER(spim);
    uint32_t u32CSMAXLT = (uint32_t)((u32CSM / fFreq) - (2 * 8 * u32DIV) - ((u32CipherEn == SPIM_HYPER_OP_ENABLE) ? 21 : 54));

    /* Chip Select Setup Time 3.5 HCLK */
    SPIM_HYPER_SET_CSST(spim, SPIM_HYPER_CSST_3_5_HCLK);

    /* Chip Select Hold Time 3.5 HCLK */
    SPIM_HYPER_SET_CSH(spim, SPIM_HYPER_CSH_3_5_HCLK);

    /* Chip Select High between Transaction as 2 HCLK cycles */
    SPIM_HYPER_SET_CSHI(spim, HYPERRAM_CSHI_CYCLE);

    /* Chip Select Masximum low time HCLK */
    SPIM_HYPER_SET_CSMAXLT(spim, u32CSMAXLT);

    /* Initial Device RESETN Low Time 255 */
    SPIM_HYPER_SET_RSTNLT(spim, HYPER_RAM_RST_CNT);

    /* Initial Read Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTRD(spim, u32AcctRD);

    /* Initial Write Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTWR(spim, u32AcctWR);
}

/**
 * @brief Erase and check HyperRAM
 *
 * @param spim
 * @param u32StartAddr  erase start address
 * @param u32EraseSize  erase size
 */
void HyperRAM_Erase(SPIM_T *spim, uint32_t u32StartAddr, uint32_t u32EraseSize)
{
    uint16_t u16Data;
    uint32_t u32i = 0;
    uint32_t u32RemainSize = (u32EraseSize % 2);

    for (u32i = 0; u32i <= (u32EraseSize - u32RemainSize); u32i += 2)
    {
        SPIM_HYPER_Write2Byte(spim, (u32StartAddr + u32i), 0x0000);
        u16Data = SPIM_HYPER_Read1Word(spim, (u32StartAddr + u32i));

        if (u16Data != 0x0000)
        {
            printf("Erase Hyper RAM fail!! Read address:0x%08x, data::0x%08x  expect: 0\n",
                   u32StartAddr + u32i,
                   u16Data);

            while (1);
        }
    }

    if (u32RemainSize != 0)
    {
        SPIM_HYPER_Write1Byte(spim, (u32StartAddr + (u32EraseSize - 1)), 0x00);
        u16Data = SPIM_HYPER_Read1Word(spim, (u32StartAddr + u32EraseSize));

        if ((u16Data & 0xFF) != 0)
        {
            printf("Erase Remain Hyper RAM fail!! Read address:0x%08x, data::0x%08x  expect: 0\n",
                   (u32StartAddr + (u32EraseSize - 1)),
                   u16Data);

            while (1);
        }
    }
}

/**
 * @brief Trim DLL component delay stop number
 *
 * @param spim
 */
void HyperRAM_TrimDLLDelayNumber(SPIM_T *spim)
{
    uint8_t u8RdDelay = 0;
    uint8_t u8RdDelayIdx = 0;
    uint8_t u8RdDelayRes[SPIM_MAX_DLL_LATENCY] = {0};
    uint32_t u32i = 0;
    uint32_t u32SrcAddr = 0;
    uint8_t au8TrimPatten[128] =
    {
        0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0xCC, 0xC3, 0xCC, 0xC3, 0x3C, 0xCC, 0xFF, 0xFE, 0xFF, 0xFE, 0xEF,
        0xFF, 0xDF, 0xFF, 0xDD, 0xFF, 0xFB, 0xFF, 0xFB, 0xBF, 0xFF, 0x7F, 0xFF, 0x77, 0xF7, 0xBD, 0xEF,
        0xFF, 0xF0, 0xFF, 0xF0, 0x0F, 0xFC, 0xCC, 0x3C, 0xCC, 0x33, 0xCC, 0xCF, 0xFE, 0xFF, 0xFF, 0xEE,
        0xFF, 0xFD, 0xFF, 0xFD, 0xDF, 0xFF, 0xBF, 0xFF, 0xBB, 0xFF, 0xF7, 0xFF, 0xF7, 0x7F, 0x7B, 0xDE,
        0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0xCC, 0xC3, 0xCC, 0xC3, 0x3C, 0xCC, 0xFF, 0xFE, 0xFF, 0xFE, 0xEF,
        0xFF, 0xDF, 0xFF, 0xDD, 0xFF, 0xFB, 0xFF, 0xFB, 0xBF, 0xFF, 0x7F, 0xFF, 0x77, 0xF7, 0xBD, 0xEF,
        0xFF, 0xF0, 0xFF, 0xF0, 0x0F, 0xFC, 0xCC, 0x3C, 0xCC, 0x33, 0xCC, 0xCF, 0xFE, 0xFF, 0xFF, 0xEE,
        0xFF, 0xFD, 0xFF, 0xFD, 0xDF, 0xFF, 0xBF, 0xFF, 0xBB, 0xFF, 0xF7, 0xFF, 0xF7, 0x7F, 0x7B, 0xDE,
    };
    uint8_t au8DestArray[DLL_TRIM_SIZE] = {0};
#ifdef DMM_MODE_TRIM
    uint32_t u32DMMAddr = SPIM_HYPER_GetDMMAddress(spim);
    uint32_t *pu32RdBuf = NULL;
    uint32_t u32RdDataCnt = 0;
#endif

    SPIM_HYPER_EnableDLL(spim);

    /* Write Data to HyperRAM */
    for (u32i = u32SrcAddr; u32i < DLL_TRIM_SIZE; u32i++)
    {
        SPIM_HYPER_Write1Byte(spim, u32i, au8TrimPatten[u32i]);
    }

#ifdef DMM_MODE_TRIM
    SPIM_HYPER_EnterDirectMapMode(spim);
#endif

    for (u8RdDelay = 0; u8RdDelay < SPIM_HYPER_MAX_LATENCY; u8RdDelay++)
    {
        /* Set DLL calibration to select the valid delay step number */
        if (SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay) != SPIM_HYPER_OK)
        {
            printf("DLL Config Failed\r\n");
        }

        memset(au8DestArray, 0, DLL_TRIM_SIZE);

        pu32RdBuf = (uint32_t *)&au8DestArray[0];
        u32RdDataCnt = 0;

#ifndef DMM_MODE_TRIM
        /* Read Data from HyperRAM */
        SPIM_HYPER_DMARead(spim, u32SrcAddr, au8DestArray, u32TestSize);
#else

#ifdef NVT_DCACHE_ON
        SCB_InvalidateDCache_by_Addr((volatile void *)u32DMMAddr, (int32_t)DLL_TRIM_SIZE);
#endif

        for (u32i = u32SrcAddr; u32i < (u32SrcAddr + DLL_TRIM_SIZE); u32i += 4)
        {
            pu32RdBuf[u32RdDataCnt++] = inpw(u32DMMAddr + u32i);
        }

#endif

        /* Verify the data and save the number of successful delay steps */
        if (memcmp(au8TrimPatten, au8DestArray, DLL_TRIM_SIZE))
        {
            //printf("!!!\tData compare failed at block 0x%x\n", u32SrcAddr);
            printf("Delay Step Num : %d = Fail\r\n", u8RdDelay);
        }
        else
        {
            printf("Delay Step Num : %d = Pass\r\n", u8RdDelay);
            u8RdDelayRes[u8RdDelayIdx++] = u8RdDelay;
        }

        if (u8RdDelayIdx >= 7)
        {
            break;
        }
    }

    if (u8RdDelayIdx <= 1)
    {
        u8RdDelayIdx = 0;
    }
    else
    {
        if (u8RdDelayIdx >= 2)
        {
            u8RdDelayIdx = (u8RdDelayIdx / 2) - 1;
        }
        else
        {
            u8RdDelayIdx = 1;
        }
    }

    /* Set the number of intermediate delay steps */
    SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelayRes[u8RdDelayIdx]);
    printf("Set DLL Delay Num : %d\r\n", u8RdDelayRes[u8RdDelayIdx]);
}

void HyperRAM_Init(SPIM_T *spim)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable SPIM0/1 Module Clock */
    CLK_EnableModuleClock(SPIM0_MODULE);
    CLK_EnableModuleClock(SPIM1_MODULE);

    /* Lock protected registers */
    SYS_LockReg();

    /* Enable SPIM Hyper Bus Mode */
    SPIM_HYPER_Init(spim, 1);

    /* SPIM Def. Enable Cipher, First Disable the test. */
    SPIM_HYPER_DISABLE_CIPHER(spim);

    /* Set R/W Latency Number */
    SPIM_Hyper_DefaultConfig(spim, HYPERRAM_CSM_TIME, HYPERRAM_RD_LTCY, HYPERRAM_WR_LTCY);

    /* Reset HyperRAM */
    SPIM_HYPER_Reset(spim);

    /* Trim DLL component delay stop number */
    HyperRAM_TrimDLLDelayNumber(spim);

#if (SPIM_REG_CACHE == 1) //TESTCHIP_ONLY not support
    /* Enable SPIM Cache */
    SPIM_HYPER_ENABLE_CACHE(spim);
#endif
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
