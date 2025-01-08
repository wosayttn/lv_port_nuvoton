#include "tc.h"
#include "tc_gdma.h"
#include "string.h"
#include <math.h>

#define CONFIG_GDMADESC_NUNBER        1

static S_CMDBUF s_sGDMADsc[CONFIG_GDMADESC_NUNBER] = {0};
static int volatile s_i32ErrCount = 0;

static void tc005_gdma_dsc_init(S_CMDBUF *psCmdBufHead, int i32DescNum, uint32_t u32BaseAddr, int i32BatchSize, int i32XferSize)
{
    const static uint32_t au32TS[] =
    {
        DMA350_CH_TRANSIZE_8BITS,    // 1
        DMA350_CH_TRANSIZE_16BITS,   // 2
        DMA350_CH_TRANSIZE_32BITS,   // 4
        DMA350_CH_TRANSIZE_64BITS,   // 8
        DMA350_CH_TRANSIZE_128BITS,  // 16
        DMA350_CH_TRANSIZE_256BITS,  // 32
        DMA350_CH_TRANSIZE_512BITS,  // 64
        DMA350_CH_TRANSIZE_1024BITS  // 128
    };

    int i;
    S_CMDBUF *next = psCmdBufHead; // first descriptor.
    struct dma350_cmdlink_gencfg_t cmdlink_cfg;

    uint32_t u32XferCount = i32BatchSize / i32XferSize;
    uint32_t u32AddrSrc;
    uint32_t u32AddrDst;

    if ((i32XferSize < 1)  || (i32XferSize > 128))
        return;

    for (i = 0; i < i32DescNum; i++)
    {
        uint32_t X;
        u32AddrSrc = u32BaseAddr + i * i32BatchSize;
        u32AddrDst = u32BaseAddr + (i + 1) * i32BatchSize;

        dma350_cmdlink_init(&cmdlink_cfg);
        //dma350_cmdlink_set_regclear(&cmdlink_cfg);
        dma350_cmdlink_set_src_des(&cmdlink_cfg, (const void *)u32AddrSrc, (void *)u32AddrDst, u32XferCount, u32XferCount);

        if (u32XferCount > 0xFFFF)
        {
            dma350_cmdlink_set_xsize32(&cmdlink_cfg, u32XferCount, u32XferCount);
        }
        else
        {
            dma350_cmdlink_set_xsize16(&cmdlink_cfg, (uint16_t)u32XferCount, (uint16_t)u32XferCount);
        }

        X = (uint32_t) log2(8 * i32XferSize) - 3;
        dma350_cmdlink_set_transize(&cmdlink_cfg, au32TS[X]);
        dma350_cmdlink_set_xtype(&cmdlink_cfg, DMA350_CH_XTYPE_CONTINUE);
        dma350_cmdlink_set_ytype(&cmdlink_cfg, DMA350_CH_YTYPE_DISABLE);
        dma350_cmdlink_set_xaddrinc(&cmdlink_cfg, 1, 1); //Src, Dst address move 1 unit.

#if 0
        if (i == (i32DescNum - 1))
        {
            dma350_cmdlink_disable_linkaddr(&cmdlink_cfg);
            // Final cmdlink, raise a event.
            dma350_cmdlink_enable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
            dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, NULL);
        }
        else
#endif
        {
            dma350_cmdlink_enable_linkaddr(&cmdlink_cfg);
            dma350_cmdlink_disable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
            dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, (uint32_t)(next + 1));
        }

        dma350_cmdlink_generate(&cmdlink_cfg, (uint32_t *)next, (uint32_t *)((uint32_t)next + sizeof(S_CMDBUF) - sizeof(uint32_t)));

        next++;

    } // for (i = 0; i < i32Length; i++)

}

static void tc005_exec(void)
{
    int i32BS, i32TS;
    uint32_t u32Count = 0;

    const static uint32_t au32XferSize[] = {1/*, 2, 4, 8, 16, 32, 64, 128*/};

    for (i32TS = 0; i32TS < sizeof(au32XferSize) / sizeof(uint32_t); i32TS++)
    {
        int i32RunCount = 0;
        s_i32ErrCount = 0;
        for (i32BS = 22; i32BS <= 23; i32BS += au32XferSize[i32TS])
        {
            tc_prepare(CONFIG_BASE_ADDRESS, i32BS);

            /* Initial all Lines descriptor-link. */
            memset(&s_sGDMADsc[0], 0, sizeof(s_sGDMADsc));
            tc005_gdma_dsc_init(&s_sGDMADsc[0], CONFIG_GDMADESC_NUNBER, CONFIG_BASE_ADDRESS, i32BS, au32XferSize[i32TS]);
            //tc_gdma_dsc_dump(&s_sGDMADsc[0], CONFIG_GDMADESC_NUNBER);

            /* Link to external command */
            dma350_ch_enable_linkaddr(GDMA_CH_DEV_S[1]);
            dma350_ch_set_linkaddr32(GDMA_CH_DEV_S[1], (uint32_t) &s_sGDMADsc[0]);
            dma350_ch_disable_intr(GDMA_CH_DEV_S[1], DMA350_CH_INTREN_DONE);

            g_bDone = 0;

            {
                union dma350_ch_status_t status = dma350_ch_get_status(GDMA_CH_DEV_S[1]);
                TC_PRINTF("@@@@@@@@@@@@@@@@@@ %d, status.w: %08x\n", u32Count, status.w);
            }

            __ISB();
            __DSB();
            dma350_ch_cmd(GDMA_CH_DEV_S[1], DMA350_CH_CMD_ENABLECMD);
            __ISB();
            __DSB();

            TC_PRINTF("[TS=%02dB][BS=%04dB] check busy\n", au32XferSize[i32TS], i32BS);

            /* Hang up issue: CANNOT WAIT FOR ANY STAT. */
            //PH4 = 0;
            /* Reference implementation with busy wait */
            u32Count = 0;
            do
            {
                union dma350_ch_status_t status = dma350_ch_get_status(GDMA_CH_DEV_S[1]);
                u32Count++;

                TC_PRINTF("%04d, status.w: 0x%08x, ERRINFO: 0x%08x, DMM_TIMEOUT_FLAG_STS:%08x\n", u32Count, status.w, GDMA_CH_DEV_S[1]->cfg.ch_base->CH_ERRINFO, SPIM0->DMM_TIMEOUT_FLAG_STS);
            }
            while (dma350_ch_is_busy(GDMA_CH_DEV_S[1]));
            GDMA_CH_DEV_S[1]->cfg.ch_base->CH_STATUS = DMA350_CH_STAT_ALL;
            TC_PRINTF("[TS=%02dB][BS=%04dB] check busy done\n", au32XferSize[i32TS], i32BS);

            if (tc_compare(CONFIG_BASE_ADDRESS, i32BS) < 0)
            {
                s_i32ErrCount++;

#if (_DEBUG==0)
                while (bFail);
#endif
            }

            i32RunCount++;
        }

        TC_PRINTF("Finish XferSize: %dB!! (%d/%d, Error percentage: %f%%)\n", au32XferSize[i32TS], s_i32ErrCount, i32RunCount, (float)s_i32ErrCount * 100 / i32RunCount);
    }
}

static int tc005_init(void)
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

static int tc005_cleanup(void)
{
    return 0;
}

TC_EXPORT(tc005_exec, "SPIM_HYPER_TIMEOUT_GDMA_TS=1B_BS=23B", tc005_init, tc005_cleanup);

