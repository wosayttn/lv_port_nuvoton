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

#include "norflash_code.h"

//------------------------------------------------------------------------------
#define SPIM_PORT                   SPIM0
#define SPIM_PORT_MAX_DIV           8
#define SPIFLASH_BLOCK_SIZE        (64 * 1024)

/*
   SPIM DMA requires memory buffers to be 8-byte aligned.
   TRIM_PAT_SIZE is in bytes and must be divisible by 8.
*/
#define TRIM_PAT_SIZE               256
//------------------------------------------------------------------------------

static uint8_t idBuf[3] = {0};
static uint64_t au64TrimPattern[TRIM_PAT_SIZE / sizeof(uint64_t)] = {0};
static uint64_t au64VerifyBuf[TRIM_PAT_SIZE / sizeof(uint64_t)] = {0};

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
static void SPIM_TrimRxClkDlyNum(SPIM_T *spim, SPIM_PHASE_T *psWbWrCMD, SPIM_PHASE_T *psWbRdCMD)
{
    uint8_t u8RdDelay = 0;
    uint8_t u8RdDelayRes[SPIM_MAX_DLL_LATENCY] = {0};
    uint32_t u32Val = 0;
    uint32_t u32i = 0;
    uint32_t u32k = 0;
    uint32_t u32TrimPoolPA; // Physical Address
    uint32_t u32TrimPoolMA; // Mapped Address
    uint32_t u32Div;  // Divider value
    u32TrimPoolPA = jedec_capacity_bytes(idBuf[2]) - SPIFLASH_BLOCK_SIZE;
    u32TrimPoolMA = SPIM_GET_DMMADDR(spim) + u32TrimPoolPA;
    printf("%dKB trim pool is at 0x%08x\n", (SPIFLASH_BLOCK_SIZE / 1024), u32TrimPoolPA);

    uint8_t *pu8TrimPattern = (uint8_t *)au64TrimPattern;

    /* Create Trim Pattern */
    for (u32k = 0; u32k < TRIM_PAT_SIZE; u32k++)
    {
        u32Val = (u32k & 0x0F) ^ (u32k >> 4) ^ (u32k >> 3);

        if (u32k & 0x01)
        {
            u32Val = ~u32Val;
        }

        pu8TrimPattern[u32k] = ~(uint8_t)(u32Val ^ (u32k << 3) ^ (u32k >> 2));
    }

    /* Initialize SPIM DMA/DMM read phases for page read operation */
    SPIM_DMADMM_InitPhase(spim, psWbRdCMD, SPIM_CTL0_OPMODE_PAGEREAD);

    /* Switch SPIM operation mode to Direct Map after page read phase is set */
    SPIM_DMADMM_InitPhase(spim, psWbRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

    /* Temporarily reduce SPIM clock speed to improve read stability */
    SPIM_SET_CLOCK_DIVIDER(spim, SPIM_PORT_MAX_DIV);


    /*
     * Perform DMA read from SPIM flash to verify trim pattern.
     * This verification step is used to avoid unnecessary erase/write
     * operations on every boot, thereby extending flash endurance.
     */
    SPIM_DMA_Read(SPIM_PORT,
                  u32TrimPoolPA,
                  (psWbRdCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                  TRIM_PAT_SIZE,
                  (uint8_t *)&au64VerifyBuf[0],
                  psWbRdCMD->u32CMDCode,
                  SPIM_OP_ENABLE);

#if (NVT_DCACHE_ON == 1)
    // Invalidate the data cache for the trimmed data
    SCB_InvalidateDCache_by_Addr((void *)&au64VerifyBuf[0], TRIM_PAT_SIZE);
#endif

    /* Compare read-back data with expected trim pattern */
    if (memcmp(pu8TrimPattern, (void *)&au64VerifyBuf[0], TRIM_PAT_SIZE) != 0)
    {
        /*
         * Trim pattern mismatch detected:
         * Only erase and reprogram the flash when the stored pattern
         * is invalid or missing. This prevents redundant erase/write
         * cycles on every power-up and helps prolong flash lifetime.
         */

        printf("Trim pattern not exist!! \n");
        printf("Will ERASE-PROGRAM Trim pattern at latest block address(0x%08X).\n", u32TrimPoolPA);

        /* Erase 64KB block at source address */
        SPIM_EraseBlock(spim,
                        u32TrimPoolPA,
                        (psWbWrCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                        OPCODE_BE_64K,
                        SPIM_PhaseModeToNBit(psWbWrCMD->u32CMDPhase),
                        SPIM_OP_ENABLE);

#if (NVT_DCACHE_ON == 1)
        // Invalidate the data cache for the trimmed data
        // Address is determined based on the re-trim count
        SCB_CleanDCache_by_Addr((void *)pu8TrimPattern, TRIM_PAT_SIZE);
#endif

        /* Write trim pattern back to flash using DMA */
        SPIM_DMA_Write(spim,
                       u32TrimPoolPA,
                       (psWbWrCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                       TRIM_PAT_SIZE,
                       pu8TrimPattern,
                       psWbWrCMD->u32CMDCode);
    }

    SPIM_EnterDirectMapMode(spim,
                            (psWbRdCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                            psWbRdCMD->u32CMDCode,
                            1);

    u32Div = 1; // Divider value
    do
    {
        uint32_t u32RXDlySettings = 0;

        printf("Set SPIM Clock frequency to %d Hz\n", CLK_GetHCLK0Freq() / (u32Div * 2));
        SPIM_SET_CLOCK_DIVIDER(spim, u32Div);

        memset(u8RdDelayRes, 0, SPIM_MAX_DLL_LATENCY);

        for (u8RdDelay = 0; u8RdDelay < SPIM_MAX_RX_DLY_NUM; u8RdDelay++)
        {
            /* Set DLL calibration to select the valid delay step number */
            SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);

            memset((void *)&au64VerifyBuf[0], 0, sizeof(au64VerifyBuf));

#if (NVT_DCACHE_ON == 1)
            // Invalidate the data cache for the trimmed data
            // Address is determined based on the re-trim count
            SCB_InvalidateDCache_by_Addr((void *)(u32TrimPoolMA), TRIM_PAT_SIZE);
#endif

            if (memcmp(&pu8TrimPattern[0], (void *)(u32TrimPoolMA), TRIM_PAT_SIZE) == 0)
            {
                u8RdDelayRes[u8RdDelay]++;
            }
            else if ((u8RdDelay > 0) && (u8RdDelayRes[u8RdDelay - 1] != 0))
            {
                // Not a continuous RxDlyNum;
                break;
            }

        }

        for (u32i = 0; u32i < u8RdDelay; u32i++)
        {
            printf("[%d/%d]: RXDelayPool Num: %d\r\n", u32i, u8RdDelay, u8RdDelayRes[u32i]);
            if (u8RdDelayRes[u32i] > 0)
            {
                u8RdDelayRes[u32RXDlySettings++] = u32i;
            }
        }

        if (u32RXDlySettings == 0)
        {
            printf("No valid RX Delay Num.\r\n");
        }
        else
        {
            u8RdDelay = (u32RXDlySettings == 2) ? u8RdDelayRes[1] : isConsecutive(u8RdDelayRes, u32RXDlySettings);

            printf("<< Set RX Delay Num to %d. >>\r\n", u8RdDelay);

            /* Set the number of intermediate delay steps */
            SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);

            break;
        }
    }
    while (u32Div++ < SPIM_PORT_MAX_DIV);
}


void SPIM_NorFlash_Init(SPIM_T *spim)
{
    SPIM_SET_CLOCK_DIVIDER(SPIM_PORT, SPIM_PORT_MAX_DIV);

    if (SPIM_InitFlash(SPIM_PORT, SPIM_OP_ENABLE))      /* Initialized SPI flash */
    {
        printf("SPIM flash initialize failed!\n");
        while (1);
    }

    SPIM_ReadJedecId(SPIM_PORT, idBuf, sizeof(idBuf), SPIM_BITMODE_1);
    printf("SPIM get JEDEC ID=0x%02X, 0x%02X, 0x%02X\n", idBuf[0], idBuf[1], idBuf[2]);

    printf("SPI NOR Flash Capacity is %dMB.\n", (jedec_capacity_bytes(idBuf[2]) >> 20));

    if (jedec_capacity_bytes(idBuf[2]) > (16 * 1024 * 1024)) //32MB
    {
        printf("Configure 4B-Addressing Quad read...\r\n");

        SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWb12hWrCMD, SPIM_CTL0_OPMODE_PAGEWRITE);
        SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWbEChRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

        /* Trim RX clock delay cycle. Adjust the sampling clock of received data to latch the correct data. */
        SPIM_TrimRxClkDlyNum(SPIM_PORT, &gsWb12hWrCMD, &gsWbEChRdCMD);

        /* Enter DMM Mode */
        SPIM_EnterDirectMapMode(SPIM_PORT, (gsWbEChRdCMD.u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE, gsWbEChRdCMD.u32CMDCode, 1);
    }
    else  //<=16MB
    {
        printf("Configure 3B-Addressing Quad read...\r\n");

        SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWb02hWrCMD, SPIM_CTL0_OPMODE_PAGEWRITE);
        SPIM_DMADMM_InitPhase(SPIM_PORT, &gsWbEBhRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

        /* Trim RX clock delay cycle. Adjust the sampling clock of received data to latch the correct data. */
        SPIM_TrimRxClkDlyNum(SPIM_PORT, &gsWb02hWrCMD, &gsWbEBhRdCMD);

        /* Enter DMM Mode */
        SPIM_EnterDirectMapMode(SPIM_PORT, (gsWbEBhRdCMD.u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE, gsWbEBhRdCMD.u32CMDCode, 1);
    }

lexit:
    return;
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
