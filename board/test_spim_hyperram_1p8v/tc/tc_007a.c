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

static int tc007a_exec(void)
{
    int i32BS;

    int i32RunCount = 0;
    int i32ErrCount = 0;

    //for (i32BS = CONFIG_BATCH_SIZE_START; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += CONFIG_BATCH_SIZE_STEP)
    for (i32BS = 0x400; i32BS <= CONFIG_BATCH_SIZE_STOP; i32BS += 0x400)
    {
        tc_prepare((uint8_t *)CONFIG_DST_BUFFER_ADDRESS, (uint8_t *)CONFIG_SRC_BUFFER_ADDRESS, i32BS);

        nu_pdma_memcpy((void *)(CONFIG_DST_BUFFER_ADDRESS), (void *)(CONFIG_SRC_BUFFER_ADDRESS), i32BS);

        if (tc_compare((uint8_t *)CONFIG_DST_BUFFER_ADDRESS, (uint8_t *)CONFIG_SRC_BUFFER_ADDRESS, i32BS) < 0)
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

TC_EXPORT(tc007a_exec, "PDMA_COPY_SRAM_TO_HRAM(RB Write)", NULL, NULL);

