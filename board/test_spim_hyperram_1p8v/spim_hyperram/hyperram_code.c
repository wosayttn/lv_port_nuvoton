/**************************************************************************//**
 * @file     hyperram_code.c
 * @version  V1.03
 * @brief    Collect of sub-routines running on SPIM flash.
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tc.h"
#include "NuMicro.h"
#include "hyperram_code.h"

//------------------------------------------------------------------------------
#define TRIM_PAT_SIZE               32

#define SPIM_HYPER_DIV              1

#define HYPERRAM_CSM_TIME           4000 /* ns */
#define HYPERRAM_RD_LTCY            7
#define HYPERRAM_WR_LTCY            7
#define HYPERRAM_CSHI_CYCLE         SPIM_HYPER_CSHI_5_HCLK
#define HYPER_RAM_RST_CNT           0xFF
#define VERIFY_ERASE_PATTERN        0x0000
#define CSMAXLT_CIPHER_OFF          21
#define CSMAXLT_CIPHER_ON           54

#define CONFIG_SPIM_OPTIM           1
#define HYPERRAM_RD_LTCY_OPTIM      5
#define HYPERRAM_WR_LTCY_OPTIM      5

//------------------------------------------------------------------------------
#define SPIM_HYPER_GET_CSST(spim)                                                \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSS_Msk) >> SPIM_HYPER_CONFIG1_CSS_Pos)

#define SPIM_HYPER_GET_CSH(spim)                                                 \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSH_Msk) >> SPIM_HYPER_CONFIG1_CSH_Pos)

#define SPIM_HYPER_GET_CSHI(spim)                                                 \
    ((spim->HYPER_CONFIG1 & SPIM_HYPER_CONFIG1_CSHI_Msk) >> SPIM_HYPER_CONFIG1_CSHI_Pos)

static void SPIM_Hyper_DumpConfig(SPIM_T *spim)
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
    uint32_t u32CSMAXLT = (uint32_t)((u32CSM / fFreq) -
                                     (2 * 8 * u32DIV) -
                                     (((!u32CipherEn) == SPIM_HYPER_OP_ENABLE) ? CSMAXLT_CIPHER_ON : CSMAXLT_CIPHER_OFF));

    /* Chip Select Setup Time 3.5 HCLK */
    SPIM_HYPER_SET_CSST(spim, SPIM_HYPER_CSST_4_5_HCLK);

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

    SPIM_Hyper_DumpConfig(spim);
}

/**
 * @brief    Erase HyperRAM
 *
 * @param    spim         SPIM device pointer
 * @param    u32StartAddr Erase start address
 * @param    u32EraseSize Erase size
 *
 * @return   None
 *
 * @note     This function is used to erase HyperRAM block
 */
void HyperRAM_Erase(SPIM_T *spim, uint32_t u32StartAddr, uint32_t u32EraseSize)
{
    uint32_t u32i;

    /* Erase Hyper RAM */
    for (u32i = 0; u32i < u32EraseSize; u32i += 2)
    {
        /* Erase Hyper RAM block */
        SPIM_HYPER_Write2Byte(spim, (u32StartAddr + u32i), VERIFY_ERASE_PATTERN);

        /* Read back check and erase fail */
        if (SPIM_HYPER_Read1Word(spim, u32StartAddr + u32i) != VERIFY_ERASE_PATTERN)
        {
            TC_PRINTF("Erase Hyper RAM fail!!\n");

            while (1);
        }
    }

    /* Check remain 1 byte */
    if (u32EraseSize % 2)
    {
        /* Erase remain 1 byte */
        SPIM_HYPER_Write1Byte(spim, (u32StartAddr + u32EraseSize - 1), 0x00);

        /* Read back check and erase fail */
        if ((SPIM_HYPER_Read1Word(spim, (u32StartAddr + u32EraseSize - 1)) >> 8) & 0xFF)
        {
            TC_PRINTF("Erase Remain HyperRAM fail, Read Data = %x !!\n",
                      (SPIM_HYPER_Read1Word(spim, (u32StartAddr + u32EraseSize - 1)) >> 8));

            while (1);
        }
    }
}

/**
 * @brief Check if the given array of values is consecutive.
 *
 * @param psDlyNumRange Pointer to the structure to store the range of consecutive values.
 * @param au8Src Array of values to be checked.
 * @param size Size of the array.
 */
uint8_t isConsecutive(uint8_t au8Src[], uint32_t size)
{
    uint8_t u8Find = 0, u8StartIdx = 0, u8MaxRang = 0;
    uint32_t u32i = 0, u32j = 1;

    // Check if the sequence is increasing or decreasing
    bool increasing = au8Src[1] > au8Src[0];

    // Iterate over the array
    for (u32i = 1; u32i < size; ++u32i)
    {
        // Check if the current element is consecutive to the previous one
        if ((increasing && au8Src[u32i] != au8Src[u32i - 1] + 1) ||
                (!increasing && au8Src[u32i] != au8Src[u32i - 1] - 1))
        {
            // Update the start and end indices of the consecutive range
            u8Find = u32i;
            u32j = 0;
        }

        // Increment the number of consecutive elements
        u32j++;

        // Update the range if the current range is longer than the previous one
        if (u32j >= u8MaxRang)
        {
            u8StartIdx = u8Find;
            u8MaxRang = u32j;
        }
    }

    return (u8MaxRang > 2) ?
           au8Src[((u8StartIdx + u8MaxRang / 2) + (((u8MaxRang % 2) != 0) ? 1 : 0)) - 1] :
           au8Src[u8StartIdx];
}

void HyperRAM_TrimDLLDelayNumber(SPIM_T *spim)
{
    if (spim == NULL)
    {
        return;
    }

    uint8_t u8RdDelay = 0;
    uint8_t u8RdDelayRes[SPIM_HYPER_MAX_LATENCY] = {0};
    uint32_t u32PatternSize = TRIM_PAT_SIZE;
    uint32_t u32LoopAddr = 0;
    uint32_t u32Val = 0;
    uint32_t u32i = 0;
    uint32_t u32j = 0;
    uint32_t u32k = 0;
    uint32_t u32SrcAddr = 0;
    uint32_t u32ReTrimCnt = 0;
    uint32_t u32ReTrimMaxCnt = 6;
    uint64_t au64TrimPattern[(TRIM_PAT_SIZE * 2) / 8] = {0};
    uint64_t au64VerifyBuf[TRIM_PAT_SIZE / 8] = {0};
    uint8_t *pu8TrimPattern = (uint8_t *)au64TrimPattern;
    uint8_t *pu8VerfiyBuf = (uint8_t *)au64VerifyBuf;
    uint32_t u32DMMAddr = SPIM_HYPER_GET_DMMADDR(spim);

    /* Create Trim Pattern */
    for (u32k = 0; u32k < sizeof(au64TrimPattern); u32k++)
    {
        u32Val = (u32k & 0x0F) ^ (u32k >> 4) ^ (u32k >> 3);

        if (u32k & 0x01)
        {
            u32Val = ~u32Val;
        }

        pu8TrimPattern[u32k] = ~(uint8_t)(u32Val ^ (u32k << 3) ^ (u32k >> 2));
    }

    SPIM_HYPER_DMAWrite(spim, u32SrcAddr, pu8TrimPattern, sizeof(au64TrimPattern));

    for (u32ReTrimCnt = 0; u32ReTrimCnt < u32ReTrimMaxCnt; u32ReTrimCnt++)
    {
        for (u8RdDelay = 0; u8RdDelay < SPIM_HYPER_MAX_LATENCY; u8RdDelay++)
        {
            /* Set DLL calibration to select the valid delay step number */
            SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay);

            memset(pu8VerfiyBuf, 0, sizeof(au64VerifyBuf));

#if (NVT_DCACHE_ON == 1)
            SCB_InvalidateDCache_by_Addr((volatile uint32_t *)((u32ReTrimCnt == 1) ? u32SrcAddr : (u32DMMAddr + u32SrcAddr)), (int32_t)TRIM_PAT_SIZE * 2);
#endif

            /* Calculate the pattern size based on the trim count */
            u32PatternSize =
                (((u32ReTrimCnt == 2) || (u32ReTrimCnt >= 3)) && (u8RdDelay == 0)) ?
                (TRIM_PAT_SIZE - 0x08) :
                TRIM_PAT_SIZE;

            /* Read data from the HyperRAM */
            u32LoopAddr = 0;

            for (u32k = 0; u32k < u32PatternSize; u32k += 0x08)
            {
                if (u32ReTrimCnt == 1)
                {
                    SPIM_HYPER_DMARead(spim, u32SrcAddr + u32LoopAddr, &pu8VerfiyBuf[u32k], 8);
                }
                else
                {
                    SPIM_HYPER_EnterDirectMapMode(spim);

                    /* Read 8 bytes of data from the HyperRAM */
                    *(volatile uint64_t *)&pu8VerfiyBuf[u32k] = *(volatile uint64_t *)(u32DMMAddr + u32SrcAddr + u32LoopAddr);
                }

                if ((u32i = memcmp(&pu8TrimPattern[u32LoopAddr], &pu8VerfiyBuf[u32k], 0x08)) != 0)
                {
                    break;
                }

                u32LoopAddr += (u32ReTrimCnt >= 3) ? 0x10 : 0x08;
            }

            u8RdDelayRes[u8RdDelay] += ((u32i == 0) ? 1 : 0);
        }
    }

    u32j = 0;

    for (u32i = 0; u32i < SPIM_HYPER_MAX_LATENCY; u32i++)
    {
        TC_PRINTF("[%d] RXDlyRes = %d\r\n", u32i, u8RdDelayRes[u32i]);

        if (u8RdDelayRes[u32i] == u32ReTrimMaxCnt)
        {
            u8RdDelayRes[u32j++] = u32i;
        }
    }

    u8RdDelay = (u32j < 2) ? u8RdDelayRes[0] : isConsecutive(u8RdDelayRes, u32j);

    TC_PRINTF("[%s]\n", __func__);
    TC_PRINTF("\tSet DLL Delay Num : %d\r\n", u8RdDelay);
    /* Set the number of intermediate delay steps */
    SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay);
}

typedef struct
{
    union
    {
        uint32_t u32REG;
        struct
        {
            uint32_t u4Manufacturer         : 4;
            uint32_t u4ColumnAddressBitCount: 4;
            uint32_t u4RowAddressBitCount   : 4;
            uint32_t u3Reserved             : 3;
            uint32_t : 17;
        };
    } ID0;

    union
    {
        uint32_t u32REG;
        struct
        {
            uint32_t u4DeviceType           : 4;
            uint32_t u3Reserved0            : 3;
            uint32_t u4DieStatus            : 1;
            uint32_t u8Reserved1            : 8;
            uint32_t : 16;
        };
    } ID1;

    union
    {
        uint32_t u32REG;
        struct
        {
            uint32_t u2BurstLength          : 2;
            uint32_t u1WrappedBrust         : 1;
            uint32_t u1FixedLatencyEanble   : 1;
            uint32_t u4InitialLatency       : 4;
            uint32_t u1BrustLength          : 1;
            uint32_t u3Reserved             : 3;
            uint32_t u3DriveStrength        : 3;
            uint32_t u1DeepPowerDownEnable  : 1;
            uint32_t : 16;
        };
    } CONFIG0;

    union
    {
        uint32_t u32REG;
        struct
        {
            uint32_t u2DistributedRefreshInerval     : 2;
            uint32_t u3PartialArrayRefresh           : 3;
            uint32_t u1HybirdSleep                   : 1;
            uint32_t u1MasterClockType               : 1;
            uint32_t u1SetRefreshRate                : 1;
            uint32_t u4Reserved                      : 4;
            uint32_t u4SoftwareReset                 : 4;
            uint32_t : 16;
        };
    } CONFIG1;

} S_W956D8NBRA_REG;


void HyperRAM_DumpConfig(SPIM_T *spim)
{
    S_W956D8NBRA_REG sHRAMReg;

    sHRAMReg.ID0.u32REG = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_ID_REG0);

    if (sHRAMReg.ID0.u4Manufacturer == 0x6) //Winbond, W956D8NBRA
    {
        TC_PRINTF("[%s] ID0.u4Manufacturer: %08X\n", __func__,                      sHRAMReg.ID0.u4Manufacturer);
        TC_PRINTF("[%s] ID0.u4ColumnAddressBitCount: %08X\n", __func__,             sHRAMReg.ID0.u4ColumnAddressBitCount);
        TC_PRINTF("[%s] ID0.u4RowAddressBitCount: %08X\n", __func__,                sHRAMReg.ID0.u4RowAddressBitCount);
        TC_PRINTF("[%s] ID0.u3Reserved: %08X\n", __func__,                          sHRAMReg.ID0.u3Reserved);

        sHRAMReg.ID1.u32REG = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_ID_REG1);
        TC_PRINTF("[%s] ID1.u4DeviceType: %08X\n", __func__,                        sHRAMReg.ID1.u4DeviceType);
        TC_PRINTF("[%s] ID1.u3Reserved0: %08X\n", __func__,                         sHRAMReg.ID1.u3Reserved0);
        TC_PRINTF("[%s] ID1.u4DieStatus: %08X\n", __func__,                         sHRAMReg.ID1.u4DieStatus);
        TC_PRINTF("[%s] ID1.u3Reserved1: %08X\n", __func__,                         sHRAMReg.ID1.u8Reserved1);

        sHRAMReg.CONFIG0.u32REG = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0);
        TC_PRINTF("[%s] CONFIG0.u2BurstLength: %08X\n", __func__,                   sHRAMReg.CONFIG0.u2BurstLength);
        TC_PRINTF("[%s] CONFIG0.u1WrappedBrust: %08X\n", __func__,                  sHRAMReg.CONFIG0.u1WrappedBrust);
        TC_PRINTF("[%s] CONFIG0.u1FixedLatencyEanble: %08X\n", __func__,            sHRAMReg.CONFIG0.u1FixedLatencyEanble);
        TC_PRINTF("[%s] CONFIG0.u4InitialLatency: %08X\n", __func__,                sHRAMReg.CONFIG0.u4InitialLatency);
        TC_PRINTF("[%s] CONFIG0.u1BrustLength: %08X\n", __func__,                   sHRAMReg.CONFIG0.u1BrustLength);
        TC_PRINTF("[%s] CONFIG0.u3Reserved: %08X\n", __func__,                      sHRAMReg.CONFIG0.u3Reserved);
        TC_PRINTF("[%s] CONFIG0.u3DriveStrength: %08X\n", __func__,                 sHRAMReg.CONFIG0.u3DriveStrength);
        TC_PRINTF("[%s] CONFIG0.u1DeepPowerDownEnable: %08X\n", __func__,           sHRAMReg.CONFIG0.u1DeepPowerDownEnable);
        //sHRAMReg.CONFIG0.u3DriveStrength = 0; // 25ohm (default)
        sHRAMReg.CONFIG0.u3DriveStrength = 1; // 50ohm
        //sHRAMReg.CONFIG0.u3DriveStrength = 2; //100ohm
        //sHRAMReg.CONFIG0.u3DriveStrength = 3; //200ohm
        //sHRAMReg.CONFIG0.u2BurstLength   = 4; //300ohm

        TC_PRINTF("[%s] sHRAMReg.CONFIG0.u32REG: %08X\n", __func__,                 sHRAMReg.CONFIG0.u32REG);
        SPIM_HYPER_WriteHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0, sHRAMReg.CONFIG0.u32REG);
        sHRAMReg.CONFIG0.u32REG = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0);
        TC_PRINTF("[%s] sHRAMReg.CONFIG0.u32REG: %08X\n", __func__,                 sHRAMReg.CONFIG0.u32REG);

        sHRAMReg.CONFIG1.u32REG = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG1);
        TC_PRINTF("[%s] CONFIG1.u2DistributedRefreshInerval: %08X\n", __func__,     sHRAMReg.CONFIG1.u2DistributedRefreshInerval);
        TC_PRINTF("[%s] CONFIG1.u3PartialArrayRefresh: %08X\n", __func__,           sHRAMReg.CONFIG1.u3PartialArrayRefresh);
        TC_PRINTF("[%s] CONFIG1.u1HybirdSleep: %08X\n", __func__,                   sHRAMReg.CONFIG1.u1HybirdSleep);
        TC_PRINTF("[%s] CONFIG1.u1SetRefreshRate: %08X\n", __func__,                sHRAMReg.CONFIG1.u1SetRefreshRate);
        TC_PRINTF("[%s] CONFIG1.u4Reserved: %08X\n", __func__,                      sHRAMReg.CONFIG1.u4Reserved);
        TC_PRINTF("[%s] CONFIG1.u4SoftwareReset: %08X\n", __func__,                 sHRAMReg.CONFIG1.u4SoftwareReset);
    }
}

void HyperRAM_Init(SPIM_T *spim)
{
    /* Enable SPIM Hyper Bus Mode */
    //SPIM_HYPER_Init(spim, SPIM_HYPERRAM_MODE, SPIM_HYPER_DIV);
    SPIM_HYPER_Init(spim, SPIM_HYPERFLASH_MODE, SPIM_HYPER_DIV);

    /* SPIM Def. Enable Cipher, First Disable the test. */
    SPIM_HYPER_DISABLE_CIPHER(spim);

    /* Set R/W Latency Number */
    SPIM_Hyper_DefaultConfig(spim, HYPERRAM_CSM_TIME, HYPERRAM_RD_LTCY, HYPERRAM_WR_LTCY);

    /* Reset HyperRAM */
    SPIM_HYPER_Reset(spim);

    /* Trim DLL component delay stop number */
    HyperRAM_TrimDLLDelayNumber(spim);

    HyperRAM_DumpConfig(spim);

    if (0)
    {
        uint32_t u32HRAMReg0 = SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0);

        TC_PRINTF("Before u32HRAMReg0:%08x\n", u32HRAMReg0);

        u32HRAMReg0 &= ~0x00F0;     // 5 Clock Latency @ 133MHz Max Freq.
        //u32HRAMReg0 |= 0xF0;     // 4 Clock Latency @ 104MHz Max Freq.

        TC_PRINTF("Updated u32HRAMReg0:%08x\n", u32HRAMReg0);

        SPIM_HYPER_WriteHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0, u32HRAMReg0);

        TC_PRINTF("After u32HRAMReg0:%08x\n", SPIM_HYPER_ReadHyperRAMReg(spim, SPIM_HYPER_HRAM_CONFIG_REG0));

        SPIM_Hyper_DefaultConfig(spim, HYPERRAM_CSM_TIME, HYPERRAM_RD_LTCY_OPTIM, HYPERRAM_WR_LTCY_OPTIM);
        TC_PRINTF("Applied CSM_Time:%d  RdLatency:%d, WrLatency:%d\n", HYPERRAM_CSM_TIME, HYPERRAM_RD_LTCY_OPTIM, HYPERRAM_WR_LTCY_OPTIM);

        /* Trim DLL component delay stop number */
        HyperRAM_TrimDLLDelayNumber(spim);

    }
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
