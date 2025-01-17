#include "NuMicro.h"
#include "tc.h"
#include "stdlib.h"
#include "string.h"

#define CONFIG_MAX_TC_NUM     64

static tc_export_t tc_table = NULL;
static int tc_num = 0;
static int ai32tc_result[CONFIG_MAX_TC_NUM] = {0};
static int tc_times = 0;

static int tc_init(void)
{
    extern const int TcTab$$Base;
    extern const int TcTab$$Limit;
    tc_table = (tc_export_t)&TcTab$$Base;
    tc_num = (tc_export_t)&TcTab$$Limit - tc_table;

    memset((void *)&ai32tc_result[0], 0, tc_num * sizeof(int));

    TC_PRINTF("\n\ntotal testcase num: (%d)\n", tc_num);

    return tc_num;
}

void tc_list(void)
{
    int i = 0;

    tc_init();

    for (i = 0; i < tc_num; i++)
    {
        TC_PRINTF("[#%d]:%s \n", i + 1, tc_table[i].name);
    }
    TC_PRINTF("\n");
}

void tc_report(void)
{
    int i = 0;

    TC_PRINTF("\n\n\n");
    TC_PRINTF("#################################################################\n");
    TC_PRINTF("[%08d]#######################################################\n", tc_times);
    for (i = 0; i < tc_num; i++)
    {
        TC_PRINTF("[%02d]%44s  [%08d-%4s]\n", i + 1, tc_table[i].name, ai32tc_result[i], (ai32tc_result[i] == 0) ? "PASS" : "FAIL");
    }
    TC_PRINTF("#################################################################\n");
    TC_PRINTF("#################################################################\n");
}

int tc_run(void)
{
    int i = 0;

    for (i = 0; i < tc_num; i++)
    {
        if (tc_table[i].tc_exec)
        {
            TC_PRINTF("\n**** [%d] %s Start ****\n", i + 1, tc_table[i].name);
            if (tc_table[i].tc_init)
            {
                //TC_PRINTF("initial %s\n", tc_table[i].name);
                tc_table[i].tc_init();
            }

            //TC_PRINTF("execute %s\n", tc_table[i].name);
            if (tc_table[i].tc_exec() < 0)
                ai32tc_result[i]++;

            if (tc_table[i].tc_cleanup)
            {
                //TC_PRINTF("cleanup %s\n", tc_table[i].name);
                tc_table[i].tc_cleanup();
            }
            TC_PRINTF("**** [%d] %s Stop ****\n", i + 1, tc_table[i].name);
        }
    }

    tc_times++;

    return 0;
}

void tc_prepare(uint32_t u32BaseAddr, int i32BatchSize)
{
    int i = 0;
    volatile uint8_t *ptr = (volatile uint8_t *)u32BaseAddr;
    volatile uint8_t *pu8SrcBuf = &ptr[0];
    volatile uint8_t *pu8DstBuf = &ptr[i32BatchSize];

    memset((void *)pu8DstBuf, 0xa5, i32BatchSize);
    //memset((void *)pu8SrcBuf, 0xa5, 2*i32BatchSize);

    for (i = 0; i < i32BatchSize; i++)
    {
        pu8SrcBuf[i] = i % 256;
    }

    __ISB();
    __DSB();
}



int tc_compare(uint32_t u32BaseAddr, int i32BatchSize)
{
    uint8_t *ptr = (uint8_t *)u32BaseAddr;
    uint8_t *pu8V0 = &ptr[0];
    uint8_t *pu8V1 = &ptr[i32BatchSize];

    int j;
    int bFail = 0;

    /* Start comparison. */
    PD6 = 0;

    for (j = 0; j < i32BatchSize; j++)
    {
        if (pu8V0[j] != pu8V1[j])
        {
            bFail = 1;
            PH4 = 0;
            goto exit_tc_compare;
        }
    }

    /* Stop comparison. */
    PD6 = 1;

    return 0;

exit_tc_compare:

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

        TC_PRINTF("\tFirst: BS=%d, 0x%02X@0x%08X != 0x%02X@0x%08X\n", i32BatchSize, pu8V0[j], (uint32_t)&pu8V0[j], pu8V1[j], (uint32_t)&pu8V1[j]);
    }

    return -1;
}
