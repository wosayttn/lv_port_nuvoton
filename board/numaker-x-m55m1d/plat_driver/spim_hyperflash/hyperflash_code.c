/**************************************************************************//**
 * @file     hyperflash_code.c
 * @version  V1.00
 * @brief    Collect of sub-routines running on SPIM flash.
 *
 *
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMicro.h"
#include "hyperflash_code.h"

#define HF_WORKAROUND

#ifndef HF_DEBUG
    #define HF_DEBUG              1
#endif

#if HF_DEBUG
    #define HF_DBGMSG   printf
#else
    #define HF_DBGMSG(...)   do { } while (0) /* disable debug */
#endif

#define SPIM_HYPER_DIV              1       // Divider for SPIM HyperBus clock

#define VERIFY_ERASE_PATTERN        0x0000  // Pattern used to verify erase operation
#define CSMAXLT_CIPHER_OFF          21      // Maximum low time for Chip Select when cipher is off
#define CSMAXLT_CIPHER_ON           54      // Maximum low time for Chip Select when cipher is on
#define SPIM_FLASH_DEFAULT_LENGTH   (1024 * 1024)

#define DMM_VERIFY_MAX_RETRY        3       // Maximum number of retries for DMM verification

//------------------------------------------------------------------------------
static const uint32_t hf_crc32_tab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t hf_crc32(const uint8_t *ptr, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;

    for (uint32_t i = 0; i < len; i++)
    {
        crc = hf_crc32_tab[(crc ^ ptr[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFFUL;
}

//------------------------------------------------------------------------------
static void HyperFlash_WriteOPCMD(SPIM_T *spim, uint32_t u32CMD, uint32_t u32Addr);
static int32_t HyperFlash_WaitBusBusy(SPIM_T *spim);

static void HyperFlash_ClearECCError(SPIM_T *spim)
{
    HyperFlash_WriteOPCMD(spim, HF_CMD_NOOP_CODE, HF_CMD_NOOP_CODE);
    //HyperFlash_WriteOPCMD(spim, HF_CMD_NOOP_CODE, HF_CMD_NOOP_CODE);
    //HyperFlash_WriteOPCMD(spim, HF_CMD_NOOP_CODE, HF_CMD_NOOP_CODE);
}

/**
 * @brief Send Hyper Flash Operation Command
 *
 * @param spim
 * @param u32CMD
 * @param u32CMDData
 */
static void HyperFlash_WriteOPCMD(SPIM_T *spim, uint32_t u32CMD, uint32_t u32Addr)
{
    SPIM_HYPER_Write2Byte(spim, (u32CMD * 2), u32Addr);
}

static uint16_t HyperFlash_ReadOPCMD(SPIM_T *spim, uint32_t u32Addr)
{
    return SPIM_HYPER_Read1Word(spim, (u32Addr * 2));
}

static uint16_t HyperFlash_ReadIDMode(SPIM_T *spim, uint32_t u32Addr)
{
    volatile uint16_t u32RdData = 0;

    /* ID Entry */
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_COMMON_AA);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_2AA, HF_CMD_COMMON_55);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_90);

    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_98);

    u32RdData = HyperFlash_ReadOPCMD(spim, u32Addr);

    HyperFlash_WriteOPCMD(spim, HF_CMD_NOOP_CODE, HF_CMD_F0);

    return u32RdData;
}

static void HyperFlash_WriteConfigRegister(SPIM_T *spim, uint32_t u32Reg, uint16_t u16WrData)
{
#ifdef HF_WORKAROUND
    HyperFlash_ClearECCError(spim);
#endif

    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_COMMON_AA);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_2AA, HF_CMD_COMMON_55);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, u32Reg);

    SPIM_HYPER_Write2Byte(spim, HF_CMD_NOOP_CODE, u16WrData);

    HyperFlash_WaitBusBusy(spim);
}

/**
 * @brief Read HyperFlash Non-volatile config register
 *
 * @param spim
 * @return uint16_t Register value
 */
static uint16_t HyperFlash_ReadConfigRegister(SPIM_T *spim, uint32_t u32Reg)
{
    volatile uint16_t u16RdData = 0;

#ifdef HF_WORKAROUND
    HyperFlash_ClearECCError(spim);
#endif
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_COMMON_AA);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_2AA, HF_CMD_COMMON_55);
    HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, u32Reg);

    u16RdData = SPIM_HYPER_Read1Word(spim, HF_CMD_NOOP_CODE);

    return u16RdData;
}

static int32_t HyperFlash_WaitBusBusy(SPIM_T *spim)
{
    volatile int32_t i32Timeout = SPIM_HYPER_TIMEOUT;
    volatile uint32_t u32Status = 0;
    uint32_t u32Div = SPIM_HYPER_GET_CLKDIV(spim);

    SPIM_HYPER_SET_CLKDIV(spim, 2);

    do
    {
#ifdef HF_WORKAROUND
        HyperFlash_ClearECCError(spim);
#endif
        HyperFlash_WriteOPCMD(spim, HF_CMD_COMMON_555, HF_CMD_70);
        u32Status = (SPIM_HYPER_Read1Word(spim, HF_CMD_NOOP_CODE) & 0x80);
    } while ((u32Status != 0x80) && (--i32Timeout >= 0));

    SPIM_HYPER_SET_CLKDIV(spim, u32Div);

    if (i32Timeout < 0)
    {
        printf("Error: HyperFlash_WaitBusBusy timeout!\r\n");
        return SPIM_HYPER_ERR_TIMEOUT;
    }

    return SPIM_HYPER_OK;
}

/**
 * @brief Invalidate DCache for HyperFlash address range.
 *
 * @param u32Addr Start address of the range to invalidate.
 * @param u32Size Size of the range to invalidate.
 */
static void HyperFlash_InvalidateDCacheByAddr(uint32_t u32Addr, uint32_t u32Size)
{
#if (NVT_DCACHE_ON == 1)
    uint32_t u32AlignedAddr = u32Addr & ~31UL;                                // align to 32 bytes
    uint32_t u32Offset = u32Addr - u32AlignedAddr;
    uint32_t u32AlignedSize = (u32Size + u32Offset + 31) & ~31UL;                // round up to multiple of 32

    SCB_InvalidateDCache_by_Addr((uint32_t *)u32AlignedAddr, (int32_t)u32AlignedSize);
#else
    (void)u32Addr;
    (void)u32Size;
#endif
}

/**
 * @brief Trims the DLL delay number for the specified SPIM instance.
 *
 * @details Non-destructive approach: reads existing flash data at the slowest
 *          (most reliable) DLL setting and computes a CRC32 reference checksum.
 *          Then iterates through all possible DLL delay numbers at the target
 *          speed, computing CRC32 for each, and selects the center of the
 *          longest consecutive matching range as the optimal setting.
 *          This preserves original HyperFlash content (no erase/write required).
 *
 * @param spim  Pointer to the SPIM peripheral.
 */
static void HyperFlash_TrimDLLDelayNumber(SPIM_T *spim)
{
    uint8_t u8RdDelay = 0;
    uint32_t u32ReadAddr, u32ReadSize;
    uint32_t u32CRC32[SPIM_HYPER_MAX_LATENCY] = {0};
    uint32_t u32RefCRC32;
    uint32_t u32i, u32j;
    uint32_t u32Div = SPIM_HYPER_GET_CLKDIV(spim);

#if defined(__ICCARM__)    /* IAR Compiler */
    #pragma section = "SPIM"
    u32ReadAddr = (uint32_t)__section_begin("SPIM");
    u32ReadSize = (uint32_t)__section_size("SPIM");

    if ((u32ReadAddr == 0) || (u32ReadSize == 0))
    {
        u32ReadAddr = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        u32ReadSize = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }

#elif defined(__GNUC__) && !defined(__ARMCC_VERSION)    /* GCC */
    extern uint32_t __spim_start__ __attribute__((weak));
    extern uint32_t __spim_end__ __attribute__((weak));

    if (((uint32_t)&__spim_start__ == 0) || ((uint32_t)&__spim_end__ == 0))
    {
        u32ReadAddr = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        u32ReadSize = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }
    else
    {
        u32ReadAddr = (uint32_t)&__spim_start__;
        u32ReadSize = (uint32_t)&__spim_end__ - (uint32_t)&__spim_start__;
    }

#else    /* ARM Compiler 5/6 (ARMCC / ARMCLANG) */
    __WEAK extern uint32_t Image$$SPIM$$Base, Image$$SPIM$$Length;

    if (((uint32_t)&Image$$SPIM$$Base == 0) || ((uint32_t)&Image$$SPIM$$Length == 0))
    {
        u32ReadAddr = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        u32ReadSize = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }
    else
    {
        u32ReadAddr = (uint32_t)&Image$$SPIM$$Base;
        u32ReadSize = (uint32_t)&Image$$SPIM$$Length;
    }

#endif

    SPIM_HYPER_EnterDirectMapMode(spim);

    /*
     * Phase 1: Sweep all DLL values at DIV=2 (slow, reliable speed).
     *          Compute CRC32 for each, then pick the most frequently
     *          occurring CRC32 as the reference checksum.
     */
    HF_DBGMSG("Phase 1: Sweep all DLL values at DIV=2 (slow, reliable speed).\r\n");
    HF_DBGMSG("         Compute CRC32 for each, then pick the most frequently\r\n");
    HF_DBGMSG("         occurring CRC32 as the reference checksum.\r\n");

    SPIM_HYPER_SET_CLKDIV(spim, 2);

    for (u8RdDelay = 0; u8RdDelay < SPIM_HYPER_MAX_LATENCY; u8RdDelay++)
    {
        if (SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay) != SPIM_HYPER_OK)
        {
            u32CRC32[u8RdDelay] = 0;
            continue;
        }

        HyperFlash_InvalidateDCacheByAddr(u32ReadAddr, u32ReadSize);
        u32CRC32[u8RdDelay] = hf_crc32((const uint8_t *)u32ReadAddr, u32ReadSize);
    }

    /* Find the CRC32 value that appears most frequently */
    u32RefCRC32 = u32CRC32[0];
    {
        uint32_t u32BestCount = 0;

        for (u32i = 0; u32i < SPIM_HYPER_MAX_LATENCY; u32i++)
        {
            uint32_t u32Count = 0;
            HF_DBGMSG("HyperFlash(DIV=2)[%02d] Calculate CRC32=0x%08X\r\n", u32i, u32CRC32[u32i]);

            for (u32j = 0; u32j < SPIM_HYPER_MAX_LATENCY; u32j++)
            {
                if (u32CRC32[u32j] == u32CRC32[u32i])
                    u32Count++;
            }

            if (u32Count > u32BestCount)
            {
                u32BestCount = u32Count;
                u32RefCRC32 = u32CRC32[u32i];
            }
        }
    }

    HF_DBGMSG("HyperFlash(DIV=2) DLL Trim: Final reference CRC32=0x%08X\r\n", u32RefCRC32);

    /*
     * Phase 2: Sweep all DLL values at DIV=1 (target speed).
     *          Compare each CRC32 against the reference from Phase 1.
     */
    HF_DBGMSG("Phase 2: Sweep all DLL values at DIV=1 (target speed).\r\n");
    HF_DBGMSG("         Compare each CRC32 against the reference from Phase 1.\r\n");

    SPIM_HYPER_SET_CLKDIV(spim, 1);

    uint8_t u8ValidDly[SPIM_HYPER_MAX_LATENCY] = {0};

    for (u8RdDelay = 0; u8RdDelay < SPIM_HYPER_MAX_LATENCY; u8RdDelay++)
    {
        if (SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay) != SPIM_HYPER_OK)
        {
            u8ValidDly[u8RdDelay] = 0;
            continue;
        }

        HyperFlash_InvalidateDCacheByAddr(u32ReadAddr, u32ReadSize);

        if (hf_crc32((const uint8_t *)u32ReadAddr, u32ReadSize) == u32RefCRC32)
            u8ValidDly[u8RdDelay] = 1;
    }

    /*
     * Phase 3: Find the longest consecutive valid range and pick its center.
     */
    HF_DBGMSG("Phase 3: Find the longest consecutive valid range and pick its center.\r\n");

    uint8_t u8BestStart = 0, u8BestLen = 0;
    uint8_t u8CurStart = 0, u8CurLen = 0;

    for (u32i = 0; u32i < SPIM_HYPER_MAX_LATENCY; u32i++)
    {
        if (u8ValidDly[u32i])
        {
            if (u8CurLen == 0) u8CurStart = (uint8_t)u32i;

            u8CurLen++;
        }
        else
        {
            if (u8CurLen > u8BestLen)
            {
                u8BestLen = u8CurLen;
                u8BestStart = u8CurStart;
            }

            u8CurLen = 0;
        }
    }

    if (u8CurLen > u8BestLen)
    {
        u8BestLen = u8CurLen;
        u8BestStart = u8CurStart;
    }

    HF_DBGMSG("DLL Valid Range (DIV=1): start=%d, len=%d\r\n", u8BestStart, u8BestLen);

    for (u32i = 0; u32i < SPIM_HYPER_MAX_LATENCY; u32i++)
    {
        HF_DBGMSG("HyperFlash(DIV=1)[%02d] TrimRXDlyNum=%d\r\n", u32i, u8ValidDly[u32i]);
    }

    if (u8BestLen > 0)
    {
        u8RdDelay = u8BestStart + (u8BestLen / 2);
        HF_DBGMSG("DLL Delay set to %d (center of valid range)\r\n", u8RdDelay);
    }
    else
    {
        u8RdDelay = 0;
        HF_DBGMSG("WARNING: No valid DLL delay found at DIV=1. Fallback to %d\r\n", u8RdDelay);
    }

    SPIM_HYPER_SET_CLKDIV(spim, u32Div);
    SPIM_HYPER_SetDLLDelayNum(spim, u8RdDelay);
    SPIM_HYPER_ExitDirectMapMode(spim);
}

/**
  * @brief      SPIM Default Config HyperBus Access Module Parameters.
  * @param      spim
  * @param      u32CSMaxLT Chip Select Maximum Low Time 0 ~ 0xFFFF, Default Set 0x02ED
  * @param      u32AcctRD Initial Read Access Time 1 ~ 0x1F, Default Set 0x04
  * @param      u32AcctWR Initial Write Access Time 1 ~ 0x1F, Default Set 0x04
  * @return     None.
  */
static void SPIM_HyperFlash_DefaultConfig(SPIM_T *spim, uint32_t u32CSMaxLow,
        uint32_t u32AcctRD, uint32_t u32AcctWR)
{
    /* Chip Select Setup Time 2.5 */
    SPIM_HYPER_SET_CSST(spim, SPIM_HYPER_CSST_3_5_HCLK);

    /* Chip Select Hold Time 3.5 HCLK */
    SPIM_HYPER_SET_CSH(spim, SPIM_HYPER_CSH_3_5_HCLK);

    /* Chip Select High between Transaction as 2 HCLK cycles */
    SPIM_HYPER_SET_CSHI(spim, 2);

    /* Chip Select Masximum low time HCLK */
    SPIM_HYPER_SET_CSMAXLT(spim, u32CSMaxLow);

    /* Initial Device RESETN Low Time 255 */
    SPIM_HYPER_SET_RSTNLT(spim, 0xFF);

    /* Initial Read Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTRD(spim, u32AcctRD);

    /* Initial Write Access Time Clock cycle*/
    SPIM_HYPER_SET_ACCTWR(spim, u32AcctWR);
}

static uint32_t HyperFlash_GetLatencyNum(uint32_t u32Latency)
{
    if (u32Latency < 5)
    {
        u32Latency = 5;
    }
    else if (u32Latency > 16)
    {
        u32Latency = 16;
    }

    return (u32Latency - 5);
}

static void HyperFlash_SetReadLatency(SPIM_T *spim, uint32_t u32Latency)
{
    uint32_t u32RegValue = 0;
    uint32_t u32VCRValue = 0x8E0B;

    /* HyperFlash default read latency is 16 and write is always 1 */
    SPIM_HyperFlash_DefaultConfig(spim, HFLH_MAX_CS_LOW, 16, HFLH_WR_ACCTIME);

    SPIM_HYPER_SetDLLDelayNum(spim, 8);

    SPIM_HYPER_Reset(spim);

    //u32RegValue = HyperFlash_ReadConfigRegister(spim, READ_VCR_REG);
    //log_printf("1 VCReg = %x\r\n", u32RegValue);

    u32VCRValue |= (HyperFlash_GetLatencyNum(u32Latency) << 4);

    HyperFlash_WriteConfigRegister(spim, LOAD_VCR_REG, u32VCRValue);

    SPIM_HyperFlash_DefaultConfig(spim, HFLH_MAX_CS_LOW, u32Latency, HFLH_WR_ACCTIME);

    //u32RegValue = HyperFlash_ReadConfigRegister(spim, READ_VCR_REG);
    //log_printf("2 VCReg = %x\r\n", u32RegValue);

    HyperFlash_TrimDLLDelayNumber(spim);

    u32RegValue = HyperFlash_ReadConfigRegister(spim, READ_VCR_REG);
    //printf("2 VCReg = %x\r\n", u32RegValue);
}

void SPIM_HyperFlash_Init(SPIM_T *spim)
{
    /* Enable SPIM Hyper Bus Mode */
    SPIM_HYPER_Init(spim, SPIM_HYPERFLASH_MODE, SPIM_HYPER_DIV);

    /* SPIM Def. Enable Cipher, First Disable the test. */
    SPIM_HYPER_DISABLE_CIPHER(spim);

    HyperFlash_SetReadLatency(spim, HFLH_RD_ACCTIME);

    SPIM_HYPER_EnterDirectMapMode(spim);
}

/*** (C) COPYRIGHT 2021 Nuvoton Technology Corp. ***/
