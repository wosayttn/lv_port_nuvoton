#include "tc.h"
#include "tc_gdma.h"
#include "string.h"

static int volatile s_i32ErrCount = 0;

static void tc002_exec(void)
{
    int i32BS;
    int i32RunCount = 0;
    s_i32ErrCount = 0;
    enum dma350_lib_error_t lib_err;

    for (i32BS = CONFIG_BATCH_SIZE_START; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    {
        tc_prepare(CONFIG_BASE_ADDRESS, i32BS);
        lib_err = dma350_memcpy(GDMA_CH_DEV_S[0],
                                (const void *)(CONFIG_BASE_ADDRESS), (void *)(CONFIG_BASE_ADDRESS + i32BS),
                                i32BS,
                                DMA350_LIB_EXEC_BLOCKING);
        __ISB();
        __DSB();

        if (tc_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
        {
            s_i32ErrCount++;

#if (_DEBUG==0)
            while (bFail);
#endif
        }

        i32RunCount++;
    }

    TC_PRINTF("Finish XferSize: 1B!! (%d/%d, Error percentage: %f%%)\n", s_i32ErrCount, i32RunCount, (float)s_i32ErrCount * 100 / i32RunCount);
}

static int tc002_init(void)
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

    return 0;
}

static int tc002_cleanup(void)
{
    return 0;
}

TC_EXPORT(tc002_exec, "MEMORY COPY using GDMA(CHANNEL)", tc002_init, tc002_cleanup);

