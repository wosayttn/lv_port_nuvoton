#include "tc.h"
#include "tc_gdma.h"
#include "tc_pdma.h"
#include "string.h"

static int tc004_exec(void)
{
    int i32BS;
    int i32RunCount = 0;
    int i32ErrCount = 0;

    for (i32BS = 1; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    {
        //TC_PRINTF("PDMA MEMCPY %d\n", i32BS);

        tc_prepare(CONFIG_BASE_ADDRESS, i32BS);

        nu_pdma_memcpy((void *)(CONFIG_BASE_ADDRESS + i32BS), (void *)(CONFIG_BASE_ADDRESS), i32BS);

        if (tc_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
        {
            i32ErrCount++;

#if (_DEBUG==0)

            while (1);

#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish XferSize: 1B!! (%04d/%04d, Error percentage: %f%%)\n", i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);

    return (i32ErrCount > 0) ? -1 : 0;
}

static int tc004_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    extern void HyperRAM_Init_WithoutTrim(SPIM_T * spim, uint8_t u8RxDlyNum);
    extern void HyperRAM_Init(SPIM_T * spim);
    //HyperRAM_Init(SPIM0);
    HyperRAM_Init_WithoutTrim(SPIM0, 7);

#if CONFIG_SPIM_CACHE_ON
    SPIM_HYPER_ENABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_ENABLE_CACHE ON!!\n");
#else
    SPIM_HYPER_DISABLE_CACHE(SPIM0);
    TC_PRINTF("\tSPIM_HYPER_DISABLE_CACHE!!\n");
#endif

    /* Chip Select High between Transaction as 2 HCLK cycles */
    TC_PRINTF("Modified SPIM_HYPER_SET_CSH and SPIM_HYPER_SET_CSHI!!\n");
    SPIM_HYPER_SET_CSHI(SPIM0, 2);
    SPIM_HYPER_SET_CSH(SPIM0, 2);

    SPIM_HYPER_EnterDirectMapMode(SPIM0);

    return 0;
}

static int tc004_cleanup(void)
{
    SPIM_HYPER_ExitDirectMapMode(SPIM0);

    return 0;
}

TC_EXPORT(tc004_exec, "MEMORY COPY using PDMA", tc004_init, tc004_cleanup);

