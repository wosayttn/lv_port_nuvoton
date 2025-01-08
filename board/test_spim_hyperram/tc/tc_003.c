#include "tc.h"
#include "tc_gdma.h"
#include "string.h"

static int volatile s_i32ErrCount = 0;

static void tc003_prepare(uint32_t u32BaseAddr, int i32BatchSize)
{
    int i = 0;
    volatile uint8_t *ptr = (volatile uint8_t *)u32BaseAddr;

    for (i = 0; i < i32BatchSize; i++)
    {
        ptr[i + i32BatchSize] = ptr[i] = i % 256;
    }
    __ISB();
    __DSB();		
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

static void tc003_exec(void)
{
    int i32BS;
    int i32RunCount;
    uint8_t u8RdDelay;

    s_i32ErrCount = 0;
    i32RunCount = 0;

    for (i32BS = CONFIG_BATCH_SIZE_START; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    {
        tc003_prepare(CONFIG_BASE_ADDRESS, i32BS);

        if (tc003_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
        {
            s_i32ErrCount++;

#if (_DEBUG==0)
            while (bFail);
#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish memcmp!! (%d/%d, Error percentage: %f%%)\n", s_i32ErrCount, i32RunCount, (float)s_i32ErrCount * 100 / i32RunCount);
}

static int tc003_init(void)
{
    return 0;
}

static int tc003_cleanup(void)
{
    return 0;
}

TC_EXPORT(tc003_exec, "MEMCMP only", tc003_init, tc003_cleanup);
