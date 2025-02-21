#include "tc.h"
#include "tc_gdma.h"
#include "string.h"

static void tc003_prepare(uint32_t u32BaseAddr, int i32BatchSize)
{
    int i = 0;
    volatile uint8_t *ptr = (volatile uint8_t *)u32BaseAddr;

#if 1
    for (i = 0; i < i32BatchSize; i++)
    {
        ptr[i + i32BatchSize] = ptr[i] = i % 256;
    }
    __ISB();
    __DSB();
#else
    SPIM_HYPER_DISABLE_CACHE(SPIM0);


#endif
}

static int tc003_compare(uint32_t u32BaseAddr, int i32BatchSize)
{
    volatile uint8_t *ptr = (volatile uint8_t *)u32BaseAddr;

    volatile uint8_t *pu8V0 = &ptr[0];
    volatile uint8_t *pu8V1 = &ptr[i32BatchSize];

    int bFail = 0;
    int j;

    /* Start comparison. */
    PD6 = 0;

    if (memcmp((const void *)pu8V0, (const void *)pu8V1, i32BatchSize) != 0)
    {
        for (j = 0; j < i32BatchSize; j++)
        {
            if (pu8V0[j] != pu8V1[j])
            {
                bFail = 1;
                PH4 = 0;
                goto exit_tc003_compare;
            }
        }
    }

    /* Stop comparison. */
    PD6 = 1;

    return 0;

exit_tc003_compare:

    if (0 & bFail)
    {
        TC_PRINTF("[BaseAddr=0x%08x, BS=%04dB] Compare [0x%08X ~ 0x%08X] and [0x%08X ~ 0x%08X] -> %s\n",
                  u32BaseAddr,
                  i32BatchSize,
                  (uint32_t)pu8V0,
                  (uint32_t)pu8V0 + (i32BatchSize - 1),
                  (uint32_t)pu8V1,
                  (uint32_t)pu8V1 + (i32BatchSize - 1),
                  bFail ? "Fail" : "Okay");
        TC_PRINTF("Found first!!, BS=%d, 0x%02X@0x%08X != 0x%02X@0x%08X\n", i32BatchSize, pu8V0[j], (uint32_t)&pu8V0[j], pu8V1[j], (uint32_t)&pu8V1[j]);
    }

    return -1;
}

static int tc003_exec(void)
{
    int i32BS;
    int i32RunCount;
    uint8_t u8RdDelay;
    int i32ErrCount = 0;

    i32RunCount = 0;

    for (i32BS = 2048; i32BS <= 4096; i32BS += 2048)
    {
        tc003_prepare(CONFIG_BASE_ADDRESS, i32BS);

        if (tc003_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
        {
            i32ErrCount++;

#if (_DEBUG==0)
            while (1);
#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish memcmp!! (%04d/%04d, Error percentage: %f%%)\n", i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);

    return (i32ErrCount > 0) ? -1 : 0;
}

static int tc003_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable GDMA0 clock source */
    CLK_EnableModuleClock(GDMA0_MODULE);

    /* Reset GDMA module */
    SYS_ResetModule(SYS_GDMA0RST);

    dma350_init(&GDMA_DEV_S);
    dma350_set_ch_privileged(&GDMA_DEV_S, 0);
    dma350_set_ch_privileged(&GDMA_DEV_S, 1);

    /* Enable NVIC for GDMA CH0 */
    NVIC_EnableIRQ(GDMACH0_IRQn);

    /* Enable NVIC for GDMA CH1 */
    NVIC_EnableIRQ(GDMACH1_IRQn);

    extern void HyperRAM_Init(SPIM_T * spim);
    HyperRAM_Init(SPIM0);

    SPIM_HYPER_EnterDirectMapMode(SPIM0);

    return 0;
}

static int tc003_cleanup(void)
{
    SPIM_HYPER_ExitDirectMapMode(SPIM0);

    return 0;
}

TC_EXPORT(tc003_exec, "MEMCMP only", tc003_init, tc003_cleanup);
