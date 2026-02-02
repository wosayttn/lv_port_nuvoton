/**************************************************************************//**
 * @file     norflash_code.c
 * @version  V1.03
 * @brief    Collect of sub-routines running on SPIM flash.
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMicro.h"
#include "norflash_code.h"

//------------------------------------------------------------------------------
#define SPIM_PORT                   SPIM0
#define SPIM_PORT_DIV               1

#define DMM_MODE_TRIM
#define TRIM_PAT_SIZE               512

//------------------------------------------------------------------------------
#define FLASH_BLOCK_SIZE            (8 * 1024)  /* Flash block size. Depend on the physical flash. */
#define TEST_BLOCK_ADDR             0x10000     /* Test block address on SPI flash. */
#define BUFFER_SIZE                 2048

//------------------------------------------------------------------------------
#if (NVT_DCACHE_ON == 1)
// DCache-line aligned buffer for improved performance when DCache is enabled
static uint8_t gau8buff[DCACHE_ALIGN_LINE_SIZE(BUFFER_SIZE)] __attribute__((aligned(DCACHE_LINE_SIZE))) = {0};
#else
// Standard buffer alignment when DCache is disabled
static uint8_t gau8buff[BUFFER_SIZE] __attribute__((aligned(32))) = {0};
#endif

//------------------------------------------------------------------------------
/* Program Command Phase */
extern SPIM_PHASE_T gsWb02hWrCMD;
extern SPIM_PHASE_T gsWb12hWrCMD;

/* Standard Read Command Phase */
extern SPIM_PHASE_T gsWb0BhRdCMD;

/* Dual Read Command Phase */
extern SPIM_PHASE_T gsWbBBhRdCMD;
extern SPIM_PHASE_T gsWbBChRdCMD;

/* Quad Read Command Phase */
extern SPIM_PHASE_T gsWbEBhRdCMD;
extern SPIM_PHASE_T gsWbEChRdCMD;

static uint8_t idBuf[3] = {0};

/**
 * @brief Check if the given array of values is consecutive.
 *
 * @param psDlyNumRange Pointer to the structure to store the range of consecutive values.
 * @param au8Src Array of values to be checked.
 * @param size Size of the array.
 */
static uint8_t isConsecutive(uint8_t au8Src[], uint32_t size)
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


uint32_t jedec_capacity_bytes(uint8_t cap_code)
{
    /* Common JEDEC SPI NOR capacity codes: 0x10 ~ 0x25 */
    if (cap_code < 0x10 || cap_code > 0x25)
        return 0;   // unknown / unsupported

    return 1UL << cap_code;
}


/**
 * @brief Trim DLL component delay number
 *
 * @details This function is used to trim the delay number of DLL component,
 *          it can improve the SPIM clock performance.
 *
 * @param[in] spim The pointer of the specified SPIM module
 *
 */
void SPIM_TrimRxClkDlyNum(SPIM_T *spim, SPIM_PHASE_T *psWbWrCMD, SPIM_PHASE_T *psWbRdCMD)
{
    uint8_t u8RdDelay = 0;
    uint8_t u8RdDelayRes[SPIM_MAX_DLL_LATENCY] = {0};
    uint32_t u32PatternSize = TRIM_PAT_SIZE;
    uint32_t u32ReTrimMaxCnt = 6;
    uint32_t u32LoopAddr = 0;
    uint32_t u32Val = 0;
    uint32_t u32i = 0;
    uint32_t u32j = 0;
    uint32_t u32k = 0;
    uint32_t u32ReTrimCnt = 0;
    uint32_t u32SrcAddr;
    uint32_t u32Div = SPIM_GET_CLOCK_DIVIDER(spim); // Divider value

#define SPIFLASH_BLOCK_SIZE   (64 * 1024)
    u32SrcAddr = jedec_capacity_bytes(idBuf[2]) - SPIFLASH_BLOCK_SIZE;
    printf("%dKB trim pool is at 0x%08x\n", (SPIFLASH_BLOCK_SIZE / 1024), u32SrcAddr);

    /*
       SPIM DMA requires memory buffers to be 8-byte aligned.
       TRIM_PAT_SIZE is in bytes and must be divisible by 8.
    */
    uint64_t au64TrimPattern[(TRIM_PAT_SIZE * 2) / 8] = {0};
    uint64_t au64VerifyBuf[(TRIM_PAT_SIZE / 8)] = {0};
    uint8_t *pu8TrimPattern = (uint8_t *)au64TrimPattern;
    uint8_t *pu8VerifyBuf = (uint8_t *)au64VerifyBuf;
    uint32_t u32DMMAddr = SPIM_GET_DMMADDR(spim);

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

    /* Set SPIM clock divider to 8 */
    SPIM_SET_CLOCK_DIVIDER(spim, 8);

    /* Erase 64KB block */
    SPIM_EraseBlock(spim,
                    u32SrcAddr,
                    psWbWrCMD->u32AddrWidth == PHASE_WIDTH_32 ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                    OPCODE_BE_64K,
                    SPIM_PhaseModeToNBit(psWbWrCMD->u32CMDPhase),
                    SPIM_OP_ENABLE);

    /* Write trim pattern */
    SPIM_DMA_Write(spim,
                   u32SrcAddr,
                   psWbWrCMD->u32AddrWidth == PHASE_WIDTH_32 ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                   sizeof(au64TrimPattern),
                   pu8TrimPattern,
                   psWbWrCMD->u32CMDCode);

    /* Restore clock divider */
    SPIM_SET_CLOCK_DIVIDER(spim, u32Div); // Restore clock divider

    SPIM_DMADMM_InitPhase(spim, psWbRdCMD, SPIM_CTL0_OPMODE_PAGEREAD);
    SPIM_DMADMM_InitPhase(spim, psWbRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

    for (u32ReTrimCnt = 0; u32ReTrimCnt < u32ReTrimMaxCnt; u32ReTrimCnt++)
    {
        for (u8RdDelay = 0; u8RdDelay < SPIM_MAX_RX_DLY_NUM; u8RdDelay++)
        {
            /* Set DLL calibration to select the valid delay step number */
            SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);

            memset(pu8VerifyBuf, 0, sizeof(au64VerifyBuf));

            /* Calculate the pattern size based on the trim count */
            u32PatternSize =
                (((u32ReTrimCnt == 2) || (u32ReTrimCnt >= 3)) && (u8RdDelay == 0)) ?
                (TRIM_PAT_SIZE - 0x08) :
                TRIM_PAT_SIZE;

            /* Read data from the HyperRAM */
            u32LoopAddr = 0;

            for (u32k = 0; u32k < u32PatternSize; u32k += 0x08)
            {
#if (NVT_DCACHE_ON == 1)
                // Invalidate the data cache for the trimmed data
                // Address is determined based on the re-trim count
                SCB_InvalidateDCache_by_Addr(
                    (volatile uint32_t *)((u32ReTrimCnt == 1) ? u32SrcAddr : (u32DMMAddr + u32SrcAddr)),
                    (int32_t)TRIM_PAT_SIZE * 2); // Size of the trimmed data is 256 bytes
#endif

                if (u32ReTrimCnt == 1)
                {
                    SPIM_DMA_Read(SPIM_PORT,
                                  (u32SrcAddr + u32LoopAddr),
                                  (psWbRdCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                                  0x08,
                                  &pu8VerifyBuf[u32k],
                                  psWbRdCMD->u32CMDCode,
                                  SPIM_OP_ENABLE);
                }
                else
                {
                    SPIM_EnterDirectMapMode(spim,
                                            (psWbRdCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                                            psWbRdCMD->u32CMDCode,
                                            1);

                    /* Read 8 bytes of data from the HyperRAM */
                    *(volatile uint64_t *)&pu8VerifyBuf[u32k] = *(volatile uint64_t *)(u32DMMAddr + u32SrcAddr + u32LoopAddr);
                }

                if ((u32i = memcmp(&pu8TrimPattern[u32LoopAddr], &pu8VerifyBuf[u32k], 0x08)) != 0)
                {
                    break;
                }
                else
                {
                    //printf("u32ReTrimCnt:%d u8RdDelay:%d\n", u32ReTrimCnt, u8RdDelay);
                }

                u32LoopAddr += (u32ReTrimCnt >= 3) ? 0x10 : 0x08;
            }

            u8RdDelayRes[u8RdDelay] += ((u32i == 0) ? 1 : 0);
        }
    }

    u32j = 0;

    for (u32i = 0; u32i < SPIM_MAX_RX_DLY_NUM; u32i++)
    {
        printf("[%d/%d]: RX Delay Num : %d\r\n", u32i, SPIM_MAX_RX_DLY_NUM, u8RdDelayRes[u32i]);
        if (u8RdDelayRes[u32i] == u32ReTrimMaxCnt)
        {
            u8RdDelayRes[u32j++] = u32i;
        }
    }

    if (u32j == 0)
    {
        printf("No valid RX Delay Num.\r\n");
        while (1);
    }
    else
    {
        u8RdDelay = (u32j < 2) ? u8RdDelayRes[0] : isConsecutive(u8RdDelayRes, u32j);
        printf("<< Set RX Delay Num to %d. >>\r\n", u8RdDelay);

        /* Set the number of intermediate delay steps */
        SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);
    }
}


void SPIM_NorFlash_Init(SPIM_T *spim)
{
    /* Set SPIM clock as HCLK divided by 1 */
    SPIM_SET_CLOCK_DIVIDER(SPIM_PORT, SPIM_PORT_DIV);

    if (SPIM_InitFlash(SPIM_PORT, SPIM_OP_ENABLE))      /* Initialized SPI flash */
    {
        printf("SPIM flash initialize failed!\n");
        while (1);
    }

    SPIM_ReadJedecId(SPIM_PORT, idBuf, sizeof(idBuf), SPIM_BITMODE_1);
    printf("SPIM get JEDEC ID=0x%02X, 0x%02X, 0x%02X\n", idBuf[0], idBuf[1], idBuf[2]);

    printf("SPI NOR Flash Capacity is %dMB.\n", (jedec_capacity_bytes(idBuf[2]) >> 20));
    printf("Configure Quad read...\r\n");

    SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWb12hWrCMD, SPIM_CTL0_OPMODE_PAGEWRITE);
    SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWbEChRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

    /* Trim RX clock delay cycle. Adjust the sampling clock of received data to latch the correct data. */
    SPIM_TrimRxClkDlyNum(SPIM_PORT, &gsWb12hWrCMD, &gsWbEChRdCMD);

    /* Enter DMM Mode */
    SPIM_EnterDirectMapMode(SPIM_PORT, (gsWbEChRdCMD.u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE, gsWbEChRdCMD.u32CMDCode, 1);

lexit:
    return;
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
