#include "tc.h"
#include "tc_gdma.h"
#include "string.h"

static int tc_boundary_crossing(void)
{
#if 0
#define DEF_ROW_BOUNDARY          0x400
#define DEF_SPIM_READ_FIFO_SIZE   16
#define DEF_MAGIC                 (0xA5)

    int i32ErrCount = 0;
    int i32RunCount = 0;

    volatile uint8_t u8Last = (uint8_t)DEF_MAGIC;
    volatile uint8_t *ptrW = (volatile uint8_t *)(CONFIG_BASE_ADDRESS + DEF_ROW_BOUNDARY - DEF_SPIM_READ_FIFO_SIZE);
    volatile uint8_t *ptrR = (volatile uint8_t *)(CONFIG_BASE_ADDRESS + DEF_ROW_BOUNDARY - DEF_SPIM_READ_FIFO_SIZE);

    while (i32RunCount++ < DEF_SPIM_READ_FIFO_SIZE)
    {
        u8Last = ~u8Last;
        *ptrW++ = u8Last;
        ptrW++;
    }

    i32RunCount = 0;

    while (i32RunCount < DEF_SPIM_READ_FIFO_SIZE)
    {
        u8Last = ptrR[i32RunCount];
        i32RunCount++;
    }

    i32RunCount--;

    TC_PRINTF("Finish boundary_crossing!! (%04d/%04d, Error percentage: %f%%)\n", i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);
    return (i32ErrCount > 0) ? -1 : 0;

#else
#define DEF_MAGIC_NUM   0x000000
#define DEF_OFFSET      0
#define DEF_STEP        0x100
#define DEF_STOP        0x1000

    int32_t i32DMA;
    uint32_t u32RowAddr;
    uint32_t MagicNum;
    int ret = 0;

    for (MagicNum = DEF_MAGIC_NUM; MagicNum < 256; MagicNum++)
    {
        for (u32RowAddr = DEF_OFFSET; u32RowAddr <= DEF_STOP + DEF_OFFSET; u32RowAddr += DEF_STEP)
        {
            SPIM_HYPER_Write1Byte(SPIM0, u32RowAddr,   MagicNum);
            SPIM_HYPER_Write1Byte(SPIM0, u32RowAddr + 1, 0x00);
            SPIM_HYPER_Write1Byte(SPIM0, u32RowAddr + 2, 0x00);
            SPIM_HYPER_Write1Byte(SPIM0, u32RowAddr + 3, 0x00);

            i32DMA = SPIM_HYPER_Read2Word(SPIM0, u32RowAddr);

            if (i32DMA != MagicNum)
            {
                TC_PRINTF("\t 0x%02X, Write 1B CMD Read: 0x%08X@0x%08x\n", MagicNum, i32DMA, u32RowAddr);
                ret = -1;
            }
        }

        for (u32RowAddr = DEF_OFFSET; u32RowAddr <= DEF_STOP + DEF_OFFSET; u32RowAddr += DEF_STEP)
        {
            SPIM_HYPER_Write2Byte(SPIM0, u32RowAddr,   MagicNum);
            SPIM_HYPER_Write2Byte(SPIM0, u32RowAddr + 2, 0x0000);

            i32DMA = SPIM_HYPER_Read2Word(SPIM0, u32RowAddr);

            if (i32DMA != MagicNum)
            {
                TC_PRINTF("\t 0x%02X, Write 2B CMD Read: 0x%08X@0x%08x\n", MagicNum, i32DMA, u32RowAddr);
                ret = -1;
            }
        }

        for (u32RowAddr = DEF_OFFSET; u32RowAddr <= DEF_STOP + DEF_OFFSET; u32RowAddr += DEF_STEP)
        {
            SPIM_HYPER_Write4Byte(SPIM0, u32RowAddr, MagicNum);
            i32DMA = SPIM_HYPER_Read2Word(SPIM0, u32RowAddr);

            if (i32DMA != MagicNum)
            {
                TC_PRINTF("\t 0x%02X, Write 4B CMD Read: 0x%08X@0x%08x\n", MagicNum, i32DMA, u32RowAddr);
                ret = -1;
            }
        }
    }

    return ret;

#endif
}
TC_EXPORT(tc_boundary_crossing, "RB CROSSING(READ)", NULL, NULL);
