#include "tc.h"
#include "tc_gdma.h"
#include "string.h"
#include <math.h>

#define CONFIG_GDMADESC_NUNBER        1

#define CONFIG_SRAM_ADDRESS           0x20110000
#define CONFIG_HYPERRAM_ADDRESS       0x82000000

#if 1
    #define CONFIG_SRC_BUFFER_ADDRESS     CONFIG_SRAM_ADDRESS
    #define CONFIG_DST_BUFFER_ADDRESS     CONFIG_HYPERRAM_ADDRESS
#else
    #define CONFIG_SRC_BUFFER_ADDRESS     CONFIG_HYPERRAM_ADDRESS
    #define CONFIG_DST_BUFFER_ADDRESS     CONFIG_SRAM_ADDRESS
#endif

static void tc006_prepare(int i32DataWidth, int i32XferCount, int i32Hopping)
{
    int i = 0;
    volatile uint8_t *pu8SrcBufAddr = (volatile uint8_t *)CONFIG_SRC_BUFFER_ADDRESS;
    volatile uint8_t *pu8DstBufAddr = (volatile uint8_t *)CONFIG_DST_BUFFER_ADDRESS;

    for (i = 0; i < i32XferCount; i++)
    {
        memset((void *)(pu8SrcBufAddr + (i32DataWidth * i)), i % 256, i32DataWidth);
        memset((void *)(pu8DstBufAddr + (i32Hopping * i32DataWidth * i)), 0xA5, i32DataWidth);
    }

    __ISB();
    __DSB();
}

static int tc006_compare(int i32DataWidth, int i32XferCount, int i32Hopping)
{
    int i;
    int bFail = 0;
    volatile uint8_t *pu8SrcBufAddr = (volatile uint8_t *)CONFIG_SRC_BUFFER_ADDRESS;
    volatile uint8_t *pu8DstBufAddr = (volatile uint8_t *)CONFIG_DST_BUFFER_ADDRESS;

    /* Start comparison. */
    PD6 = 0;

    for (i = 0; i < i32XferCount; i++)
    {
        //if (pu8SrcBufAddr[i * i32DataWidth] != (pu8DstBufAddr[i * i32DataWidth * i32Hopping]))
        if (memcmp((void *)(pu8SrcBufAddr + (i32DataWidth * i)), (void *)(pu8DstBufAddr + (i32Hopping * i32DataWidth * i)), i32DataWidth) != 0)
        {
            bFail = 1;
            PH4 = 0;
            goto exit_tc006_compare;
        }
    }

    /* Stop comparison. */
    PD6 = 1;

    return 0;

exit_tc006_compare:

    if (0 & bFail)
    {
        TC_PRINTF("[BS=%04dB] Compare [0x%08X] and [0x%08X] -> %s\n",
                  i32XferCount,
                  (uint32_t)pu8SrcBufAddr + (i32DataWidth * i),
                  (uint32_t)pu8DstBufAddr + (i32Hopping * i32DataWidth * i),
                  bFail ? "Fail" : "Okay");

        TC_PRINTF("\tFirst: DataWidth=%d, XferCount=%d, @0x%08X != @0x%08X\n", i32DataWidth, i32XferCount, (uint32_t)&pu8SrcBufAddr[i], (uint32_t)&pu8DstBufAddr[i]);
    }

    return -1;
}


static void tc006_gdma_dsc_init(S_CMDBUF *psCmdBufHead, int i32DescNum, uint32_t i32Hop, int i32BatchSize, int i32XferSize)
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

    uint32_t u32XferCount = i32BatchSize;
    uint32_t u32AddrSrc;
    uint32_t u32AddrDst;

    if ((i32XferSize < 1)  || (i32XferSize > 128))
        return;

    for (i = 0; i < i32DescNum; i++)
    {
        uint32_t X;
        u32AddrSrc = CONFIG_SRC_BUFFER_ADDRESS;
        u32AddrDst = CONFIG_DST_BUFFER_ADDRESS;

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
        dma350_cmdlink_set_xaddrinc(&cmdlink_cfg, 1, i32Hop); //Src move 1 unit, Dst address move i32Hop unit.

        if (i == (i32DescNum - 1))
        {
            dma350_cmdlink_disable_linkaddr(&cmdlink_cfg);
            // Final cmdlink, raise a event.
            dma350_cmdlink_disable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
            dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, NULL);
        }
        else
        {
            dma350_cmdlink_enable_linkaddr(&cmdlink_cfg);
            dma350_cmdlink_disable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
            dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, (uint32_t)(next + 1));
        }

        dma350_cmdlink_generate(&cmdlink_cfg, (uint32_t *)next, (uint32_t *)((uint32_t)next + sizeof(S_CMDBUF) - sizeof(uint32_t)));

        next++;

    } // for (i = 0; i < i32Length; i++)

}

static int tc006_exec(void)
{
    int i32BS, i32TS, i32Hopping;
    uint32_t u32Count = 0;
    int i32ReportErrCount = 0;
    S_CMDBUF s_sGDMADsc[CONFIG_GDMADESC_NUNBER];

    const static uint32_t au32XferSize[] = {1, 2, 4, 8};

    for (i32TS = 0; i32TS < sizeof(au32XferSize) / sizeof(uint32_t); i32TS++)
    {
        int i32RunCount = 0;
        int i32ErrCount = 0;

        for (i32Hopping = au32XferSize[i32TS]; i32Hopping <= 32; i32Hopping += au32XferSize[i32TS])
        {
            for (i32BS = au32XferSize[i32TS]; i32BS <= 256; i32BS += au32XferSize[i32TS])
            {

                tc006_prepare(au32XferSize[i32TS], i32BS, i32Hopping);

                /* Initial all Lines descriptor-link. */
                memset(&s_sGDMADsc[0], 0, sizeof(s_sGDMADsc));
                tc006_gdma_dsc_init(&s_sGDMADsc[0], CONFIG_GDMADESC_NUNBER, i32Hopping, i32BS, au32XferSize[i32TS]);
                //tc_gdma_dsc_dump(&s_sGDMADsc[0], CONFIG_GDMADESC_NUNBER);

                /* Link to external command */
                dma350_ch_enable_linkaddr(GDMA_CH_DEV_S[1]);
                dma350_ch_set_linkaddr32(GDMA_CH_DEV_S[1], (uint32_t) &s_sGDMADsc[0]);
                dma350_ch_disable_intr(GDMA_CH_DEV_S[1], DMA350_CH_INTREN_DONE);

                g_bDone = 0;

                __ISB();
                __DSB();
                dma350_ch_cmd(GDMA_CH_DEV_S[1], DMA350_CH_CMD_ENABLECMD);
                __ISB();
                __DSB();
                PH4 = 0;

                /* Hang up issue: CANNOT WAIT FOR ANY STAT. */
                /* Reference implementation with busy wait */
                u32Count = 0;
                union dma350_ch_status_t status;

                do
                {
                    status = dma350_ch_get_status(GDMA_CH_DEV_S[1]);
                    u32Count++;
                    PH4 = u32Count & 0x1;

                    if (u32Count > 20480)
                    {
                        TC_PRINTF("%04d, status.w: 0x%08x, ERRINFO: 0x%08x, DMM_TIMEOUT_FLAG_STS:%08x\n", u32Count, status.w, GDMA_CH_DEV_S[1]->cfg.ch_base->CH_ERRINFO, SPIM0->DMM_TIMEOUT_FLAG_STS);
                    }
                } while (dma350_ch_is_busy(GDMA_CH_DEV_S[1]));

                GDMA_CH_DEV_S[1]->cfg.ch_base->CH_STATUS = DMA350_CH_STAT_ALL;
                PH4 = 1;

                if (tc006_compare(au32XferSize[i32TS], i32BS, i32Hopping) < 0)
                {
                    i32ErrCount++;

#if (_DEBUG==0)

                    while (1);

#endif
                }

                i32RunCount++;
            }
        }

        TC_PRINTF("Finish XferSize: %dB!! (%04d/%04d, Error percentage: %f%%)\n", au32XferSize[i32TS], i32ErrCount, i32RunCount, (float)i32ErrCount * 100 / i32RunCount);

        i32ReportErrCount += i32ErrCount;

    }

    return (i32ReportErrCount > 0) ? -1 : 0;
}

static int tc006_init(void)
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

static int tc006_cleanup(void)
{
    return 0;
}

TC_EXPORT(tc006_exec, "SPIM_HYPER_GDMA_SRAM_HYPERRAM_COPY_HOPPING", tc006_init, tc006_cleanup);

