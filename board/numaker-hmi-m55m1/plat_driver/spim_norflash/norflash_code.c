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
#define SPIM_FLASH_DEFAULT_LENGTH   (1024 * 1024)

#define ENABLE_DEBUG    1
#if ENABLE_DEBUG
    #define SPIM_DBGMSG   printf
#else
    #define SPIM_DBGMSG(...)   do { } while (0)      /* disable debug */
#endif

#if 1
static const uint32_t crc32_tab[] =
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

static uint32_t crc32(uint8_t *ptr, uint32_t len)
{
#define CRC_SEED    0xFFFFFFFF
    uint32_t crc = CRC_SEED;
    uint32_t i;
    uint8_t data8;

    for (i = 0; i < len; i++)
    {
        data8 = ptr[i];
        crc = crc32_tab[(crc ^ data8) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}

#else

static uint32_t crc32(uint8_t *ptr, uint32_t len)
{
    volatile uint32_t reg;
    uint32_t u32DMAChecksum = 0xFFFFFFFF;

    CLK_EnableModuleClock(CRC0_MODULE);

    CRC_Open(CRC_32, (CRC_WDATA_RVS | CRC_CHECKSUM_RVS | CRC_CHECKSUM_COM), 0xFFFFFFFF, CRC_CPU_WDATA_32);

    /*Set input data address for CRC DMA Master*/
    CRC_SET_DMA_SADDR(CRC, (uint32_t)ptr);

    CRC_SET_DMACNT_WORD(CRC, len / 4);

    CRC_ENABLE_DMA_INT(CRC);

    CRC_DMA_START(CRC);

    while (CRC->DMACTL & CRC_DMACTL_START_Msk) {};

    reg = CRC->DMASTS;
    if ((reg & CRC_DMASTS_FINISH_Msk) == CRC_DMASTS_FINISH_Msk) /* transfer done */
    {
        CRC->DMASTS |= CRC_DMASTS_FINISH_Msk;
        u32DMAChecksum =  CRC->CHECKSUM;
    }
    else
    {
        if ((reg & CRC_DMASTS_ABORTED_Msk) == CRC_DMASTS_ABORTED_Msk)   /* target abort */
        {
            SPIM_DBGMSG("abort flag 0x%x\n", reg);
            CRC->DMASTS |= CRC_DMASTS_ABORTED_Msk;
        }

        if ((reg & CRC_DMASTS_CFGERR_Msk) == CRC_DMASTS_CFGERR_Msk) /* config error */
        {
            SPIM_DBGMSG("config error 0x%x\n", reg);
            CRC->DMASTS |= CRC_DMASTS_CFGERR_Msk;
        }

        if ((reg & CRC_DMASTS_ACCERR_Msk) == CRC_DMASTS_ACCERR_Msk) /* access error */
        {
            SPIM_DBGMSG("access error 0x%x\n", reg);
            CRC->DMASTS |= CRC_DMASTS_ACCERR_Msk;
        }
    }

    return u32DMAChecksum;
}

#endif

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


static uint32_t jedec_capacity_bytes(uint8_t cap_code)
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
static void SPIM_TrimRxClkDlyNum(SPIM_T *spim, SPIM_PHASE_T *psWbRdCMD)
{
    __WEAK extern uint32_t Image$$SPIM$$Base, Image$$SPIM$$Length;

    uint32_t spim_start, spim_used_size;

#if defined(__ICCARM__)    /* IAR Compiler */
    #pragma section = "SPIM"
    spim_start = (uint32_t)__section_begin("SPIM");
    spim_used_size = (uint32_t)__section_size("SPIM");

    if ((spim_start == 0) || (spim_used_size == 0))
    {
        spim_start = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        spim_used_size = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }

#elif defined(__GNUC__) && !defined(__ARMCC_VERSION)    /* GCC */
    extern uint32_t __spim_start__ __attribute__((weak));
    extern uint32_t __spim_end__ __attribute__((weak));

    if (((uint32_t)&__spim_start__ == 0) || ((uint32_t)&__spim_end__ == 0))
    {
        spim_start = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        spim_used_size = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }
    else
    {
        spim_start = (uint32_t)&__spim_start__;
        spim_used_size = (uint32_t)&__spim_end__ - (uint32_t)&__spim_start__;
    }

#else    /* ARM Compiler 5/6 (ARMCC / ARMCLANG) */
    __WEAK extern uint32_t Image$$SPIM$$Base, Image$$SPIM$$Length;

    if (((uint32_t)&Image$$SPIM$$Base == 0) || ((uint32_t)&Image$$SPIM$$Length == 0))
    {
        spim_start = (uint32_t)SPIM_HYPER_DMM0_SADDR;
        spim_used_size = (uint32_t)SPIM_FLASH_DEFAULT_LENGTH;
    }
    else
    {
        spim_start = (uint32_t)&Image$$SPIM$$Base;
        spim_used_size = (uint32_t)&Image$$SPIM$$Length;
    }
#endif

    uint8_t u8RdDelay = 0;
    uint8_t u8RdDelayRes[SPIM_MAX_DLL_LATENCY] = {0};
    uint32_t u32i = 0;
    uint32_t u32Div;  // Divider value
    uint32_t u32ExcpectedCRC32Chksum;

    SPIM_DBGMSG("start address of spim_data is at 0x%08X\n", spim_start);
    SPIM_DBGMSG("Used size of spim_data is %d\n", spim_used_size);

    /* Switch SPIM operation mode to Direct Map after page read phase is set */
    SPIM_DMADMM_InitPhase(spim, psWbRdCMD, SPIM_CTL0_OPMODE_DIRECTMAP);

    /* Temporarily reduce SPIM clock speed to improve read stability */
    SPIM_SET_CLOCK_DIVIDER(spim, SPIM_PORT_MAX_DIV);

    /* Enter DMM mode. */
    SPIM_EnterDirectMapMode(spim,
                            (psWbRdCMD->u32AddrWidth == PHASE_WIDTH_32) ? SPIM_OP_ENABLE : SPIM_OP_DISABLE,
                            psWbRdCMD->u32CMDCode,
                            1);

    /* Calculate CRC32 checksum in lower speed rate. */
    u32ExcpectedCRC32Chksum = crc32((uint8_t *)spim_start, spim_used_size);
    SPIM_DBGMSG("Excpected CRC32 Chksum is 0x%08X\n", u32ExcpectedCRC32Chksum);

    u32Div = 1; // Divider value
    do
    {
        uint32_t u32RXDlySettings = 0;

        SPIM_DBGMSG("Set SPIM Clock frequency to %d Hz\n", CLK_GetHCLK0Freq() / (u32Div * 2));
        SPIM_SET_CLOCK_DIVIDER(spim, u32Div);

        memset(u8RdDelayRes, 0, SPIM_MAX_DLL_LATENCY);

        for (u8RdDelay = 0; u8RdDelay < SPIM_MAX_RX_DLY_NUM; u8RdDelay++)
        {
            /* Set DLL calibration to select the valid delay step number */
            SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);

#if (NVT_DCACHE_ON == 1)
            // Invalidate the data cache for the trimmed data
            // Address is determined based on the re-trim count
            SCB_InvalidateDCache_by_Addr((void *)spim_start, spim_used_size);
#endif
            if (crc32((uint8_t *)spim_start, spim_used_size) == u32ExcpectedCRC32Chksum)
            {
                u8RdDelayRes[u8RdDelay]++;
            }
            else if ((u8RdDelay > 0) && (u8RdDelayRes[u8RdDelay - 1] != 0))
            {
                // Not a continuous RxDlyNum.
                break;
            }
        }

        for (u32i = 0; u32i < u8RdDelay; u32i++)
        {
            SPIM_DBGMSG("[%d/%d]: RXDelayPool Num: %d\r\n", u32i, u8RdDelay, u8RdDelayRes[u32i]);
            if (u8RdDelayRes[u32i] > 0)
            {
                u8RdDelayRes[u32RXDlySettings++] = u32i;
            }
        }

        if (u32RXDlySettings > 0)
        {
            u8RdDelay = isConsecutive(u8RdDelayRes, u32RXDlySettings);

            SPIM_DBGMSG("<< Set RX Delay Num to %d. >>\r\n", u8RdDelay);

            /* Set the number of intermediate delay steps */
            SPIM_SET_RXCLKDLY_RDDLYSEL(spim, u8RdDelay);

            break;
        }
    }
    while (u32Div++ < SPIM_PORT_MAX_DIV);

    if (u32Div == SPIM_PORT_MAX_DIV)
    {
        SPIM_DBGMSG("!!!ASSERT: No valid RX Delay Num.\r\n");
        while (1);
    }
}

void SPIM_NorFlash_Init(SPIM_T *spim)
{
    uint8_t idBuf[3] = {0};

    SPIM_SET_CLOCK_DIVIDER(SPIM_PORT, SPIM_PORT_MAX_DIV);

    if (SPIM_InitFlash(SPIM_PORT, SPIM_OP_ENABLE))      /* Initialized SPI flash */
    {
        SPIM_DBGMSG("SPIM flash initialize failed!\n");
        while (1);
    }

    SPIM_ReadJedecId(SPIM_PORT, idBuf, sizeof(idBuf), SPIM_BITMODE_1);
    SPIM_DBGMSG("SPIM get JEDEC ID=0x%02X, 0x%02X, 0x%02X\n", idBuf[0], idBuf[1], idBuf[2]);

    SPIM_DBGMSG("SPI NOR Flash Capacity is %dMB.\n", (jedec_capacity_bytes(idBuf[2]) >> 20));

    if (jedec_capacity_bytes(idBuf[2]) > (16 * 1024 * 1024)) //32MB
    {
        SPIM_DBGMSG("Configure 4B-Addressing Quad read...\r\n");

        /* Trim RX clock delay cycle. Adjust the sampling clock of received data to latch the correct data. */
        SPIM_TrimRxClkDlyNum(SPIM_PORT, &gsWbEChRdCMD);
    }
    else  //<=16MB
    {
        SPIM_DBGMSG("Configure 3B-Addressing Quad read...\r\n");

        /* Trim RX clock delay cycle. Adjust the sampling clock of received data to latch the correct data. */
        SPIM_TrimRxClkDlyNum(SPIM_PORT, &gsWbEBhRdCMD);
    }
}

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
