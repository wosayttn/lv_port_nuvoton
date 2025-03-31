#include "tc.h"
#include "tc_pdma.h"
#include "string.h"
#include <math.h>

#define CONFIG_SRAM_ADDRESS           0x20110000
#define CONFIG_HYPERRAM_ADDRESS       0x82000000

#if 1
    #define CONFIG_SRC_BUFFER_ADDRESS     CONFIG_SRAM_ADDRESS
    #define CONFIG_DST_BUFFER_ADDRESS     CONFIG_HYPERRAM_ADDRESS
#else
    #define CONFIG_SRC_BUFFER_ADDRESS     CONFIG_HYPERRAM_ADDRESS
    #define CONFIG_DST_BUFFER_ADDRESS     CONFIG_SRAM_ADDRESS
#endif

static void tc007c_prepare(uint8_t *pu8DstBufAddr, uint8_t *pu8SrcBufAddr, int i32XferCount)
{
    int i = 0;

    for (i = 0; i < i32XferCount; i++)
    {
        *pu8SrcBufAddr = i % 256;
        pu8SrcBufAddr++;
    }

    memset((void *)(pu8DstBufAddr), 0xA5,    i32XferCount);

    __ISB();
    __DSB();
}

static int tc007c_compare(uint8_t *pu8DstBufAddr, uint8_t *pu8SrcBufAddr, int i32XferCount)
{
    int i;
    int bFail = 0;

    /* Start comparison. */
    PD6 = 0;

    for (i = 0; i < i32XferCount; i++)
    {
        if (pu8SrcBufAddr[i] != pu8DstBufAddr[i])
        {
            bFail = 1;
            PH4 = 0;
            goto exit_tc007_compare;
        }
    }

    /* Stop comparison. */
    PD6 = 1;

    return 0;

exit_tc007_compare:

    if (1 & bFail)
    {
        TC_PRINTF("[BS=%04dB] Compare [0x%08X ~ 0x%08X] and [0x%08X ~ 0x%08X] -> %s\n",
                  i32XferCount,
                  (uint32_t)pu8SrcBufAddr,
                  (uint32_t)pu8SrcBufAddr + (i32XferCount - 1),
                  (uint32_t)pu8DstBufAddr,
                  (uint32_t)pu8DstBufAddr + (i32XferCount - 1),
                  bFail ? "Fail" : "Okay");

        TC_PRINTF("\tFirst: XferCount=%d, 0x%02X@0x%08X != 0x%02X@0x%08X\n", i32XferCount, pu8SrcBufAddr[i], (uint32_t)&pu8SrcBufAddr[i], pu8DstBufAddr[i], (uint32_t)&pu8DstBufAddr[i]);
    }

    return -1;
}

static int tc007c_exec(void)
{
    int i32BS;
    int i32RunCount = 0;
    int i32ErrCount = 0;

    for (i32BS = 0x400; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += 0x400)
    {
        tc_prepare((uint8_t *)CONFIG_DST_BUFFER_ADDRESS + i32BS, (uint8_t *)(CONFIG_SRC_BUFFER_ADDRESS + i32BS), 0x400);

        nu_pdma_memcpy((void *)(CONFIG_DST_BUFFER_ADDRESS + i32BS), (void *)(CONFIG_SRC_BUFFER_ADDRESS + i32BS), 0x400);

        i32RunCount++;
    }

    if (tc_compare((uint8_t *)(CONFIG_DST_BUFFER_ADDRESS), (uint8_t *)(CONFIG_SRC_BUFFER_ADDRESS), CONFIG_BATCH_SIZE_STOP) < 0)
    {
        i32ErrCount++;

#if (_DEBUG==0)
        while (1);
#endif
    }
    TC_PRINTF("Finish XferSize: 1B (%04d/%04d, Error percentage: %f%%)\n", i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);

    return (i32ErrCount > 0) ? -1 : 0;
}

TC_EXPORT(tc007c_exec, "PDMA_COPY_SRAM_TO_HRAM(FIXED_BS)", NULL, NULL);

