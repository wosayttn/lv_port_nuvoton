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

static void tc007_prepare(int i32DataWidth, int i32XferCount)
{
    int i = 0;
    volatile uint8_t *pu8SrcBufAddr = (volatile uint8_t *)CONFIG_SRC_BUFFER_ADDRESS;
    volatile uint8_t *pu8DstBufAddr = (volatile uint8_t *)CONFIG_DST_BUFFER_ADDRESS;

    for (i = 0; i < i32XferCount; i++)
    {
        memset((void *)(pu8SrcBufAddr + (i32DataWidth * i)), i % 256, i32DataWidth);
        memset((void *)(pu8DstBufAddr + (i32DataWidth * i)), 0xA5, i32DataWidth);
    }

    __ISB();
    __DSB();
}

static int tc007_compare(int i32DataWidth, int i32XferCount)
{
    int i;
    int bFail = 0;
    volatile uint8_t *pu8SrcBufAddr = (volatile uint8_t *)CONFIG_SRC_BUFFER_ADDRESS;
    volatile uint8_t *pu8DstBufAddr = (volatile uint8_t *)CONFIG_DST_BUFFER_ADDRESS;

    /* Start comparison. */
    PD6 = 0;

    for (i = 0; i < i32XferCount; i++)
    {
        if (memcmp((void *)(pu8SrcBufAddr + (i32DataWidth * i)), (void *)(pu8DstBufAddr + (i32DataWidth * i)), i32DataWidth) != 0)
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
        TC_PRINTF("[BS=%04dB] Compare [0x%08X] and [0x%08X] -> %s\n",
                  i32XferCount,
                  (uint32_t)pu8SrcBufAddr + (i32DataWidth * i),
                  (uint32_t)pu8DstBufAddr + (i32DataWidth * i),
                  bFail ? "Fail" : "Okay");

        TC_PRINTF("\tFirst: DataWidth=%d, XferCount=%d, @0x%08X != @0x%08X\n", i32DataWidth, i32XferCount, (uint32_t)&pu8SrcBufAddr[i], (uint32_t)&pu8DstBufAddr[i]);
    }

    return -1;
}

static int tc007_exec(void)
{
    int i32BS;

    int i32RunCount = 0;
    int i32ErrCount = 0;

    for (i32BS = CONFIG_BATCH_SIZE_START; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    {
        tc007_prepare(1, i32BS);

        nu_pdma_memcpy((void *)(CONFIG_SRC_BUFFER_ADDRESS), (void *)(CONFIG_DST_BUFFER_ADDRESS), i32BS);

        if (tc007_compare(1, i32BS) < 0)
        {
            i32ErrCount++;

#if (_DEBUG==0)
            while (1);
#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish XferSize: 1B (%04d/%04d, Error percentage: %f%%)\n", i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);

    return (i32ErrCount > 0) ? -1 : 0;
}

static int tc007_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    extern void HyperRAM_Init(SPIM_T * spim);
    //HyperRAM_Init(SPIM0);
    extern void HyperRAM_Init_WithoutTrim(SPIM_T * spim, uint8_t u8RxDlyNum);
    HyperRAM_Init_WithoutTrim(SPIM0, 7);

#if CONFIG_SPIM_CACHE_ON
    SPIM_HYPER_ENABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_ENABLE_CACHE ON!!\n");
#else
    SPIM_HYPER_DISABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_DISABLE_CACHE!!\n");
#endif

    /* Chip Select High between Transaction as 2 HCLK cycles */
    //TC_PRINTF("Modified SPIM_HYPER_SET_CSHI to 2!!\n");
    //SPIM_HYPER_SET_CSHI(SPIM0, 2);

    SPIM_HYPER_EnterDirectMapMode(SPIM0);

    return 0;
}

static int tc007_cleanup(void)
{
    SPIM_HYPER_ExitDirectMapMode(SPIM0);

    return 0;
}

TC_EXPORT(tc007_exec, "SPIM_HYPER_PDMA_SRAM_HYPERRAM_COPY", tc007_init, tc007_cleanup);

