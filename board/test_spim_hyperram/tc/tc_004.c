#include "tc.h"
#include "tc_gdma.h"
#include "tc_pdma.h"
#include "string.h"

static int volatile s_i32ErrCount = 0;

static void tc004_exec(void)
{
    int i32BS;
    int i32RunCount = 0;
    s_i32ErrCount = 0;

    for (i32BS = CONFIG_BATCH_SIZE_START; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    {
        TC_PRINTF("PDMA MEMCPY %d\n", i32BS);

        tc_prepare(CONFIG_BASE_ADDRESS, i32BS);

        nu_pdma_memcpy((void *)(CONFIG_BASE_ADDRESS + i32BS), (void *)(CONFIG_BASE_ADDRESS), i32BS);

        if (tc_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
        {
            s_i32ErrCount++;

#if (_DEBUG==0)
            while (1);
#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish XferSize: 1B!! (%04d/%04d, Error percentage: %f%%)\n", s_i32ErrCount, i32RunCount, (float)s_i32ErrCount * 100 / i32RunCount);
}

static int tc004_init(void)
{
    return 0;
}

static int tc004_cleanup(void)
{
    return 0;
}

TC_EXPORT(tc004_exec, "MEMORY COPY using PDMA", tc004_init, tc004_cleanup);

