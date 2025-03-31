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

void tc_prepare(uint8_t *pu8DstBuf, uint8_t *pu8SrcBuf, int i32BatchSize)
{
    int i = 0;

    for (i = 0; i < i32BatchSize; i++)
    {
        pu8DstBuf[i] = 0xA5;
		    __DSB();
        pu8SrcBuf[i] = i % 256;
		    __DSB();
    }
}

int tc_compare(uint8_t *pu8DstBuf, uint8_t *pu8SrcBuf, int i32BatchSize)
{
    int j;
    int bFail = 0;

    /* Start comparison. */
    PD6 = 0;
    __DSB();

    for (j = 0; j < i32BatchSize; j++)
    {
        if (pu8SrcBuf[j] != pu8DstBuf[j])
        {
            PD5 = 1;
            __DSB();
            bFail = 1;
            goto exit_tc_compare;
        }
    }

exit_tc_compare:

    if (1 & bFail)
    {
        TC_PRINTF("[BS=%04dB] Compare [0x%08X ~ 0x%08X] and [0x%08X ~ 0x%08X] -> %s\n",
                  i32BatchSize,
                  (uint32_t)pu8DstBuf,
                  (uint32_t)pu8DstBuf + (i32BatchSize - 1),
                  (uint32_t)pu8SrcBuf,
                  (uint32_t)pu8SrcBuf + (i32BatchSize - 1),
                  bFail ? "Fail" : "Okay");

        TC_PRINTF("\tFirst: BS=%d, 0x%02X@0x%08X != 0x%02X@0x%08X\n", i32BatchSize, pu8DstBuf[j], (uint32_t)&pu8DstBuf[j], pu8SrcBuf[j], (uint32_t)&pu8SrcBuf[j]);

        {
            uint32_t u32DMAV0 = 0, u32DMAV1 = 0;

            u32DMAV0 = SPIM_HYPER_Read2Word(SPIM0, ((uint32_t)&pu8DstBuf[j]) & 0x00FFFFFC);
            u32DMAV1 = SPIM_HYPER_Read2Word(SPIM0, ((uint32_t)&pu8SrcBuf[j]) & 0x00FFFFFC);
            TC_PRINTF("\tCMD Read BS=%d, 0x%08X@0x%08X != 0x%08X@0x%08X\n",  i32BatchSize, u32DMAV0, ((uint32_t)&pu8DstBuf[j]) & 0x00FFFFFC, u32DMAV1, ((uint32_t)&pu8SrcBuf[j]) & 0x00FFFFFC);
        }
    }

    /* Stop comparison. */
    PD6 = 1;
    __DSB();

    return (bFail ? -1 : 0);
}
