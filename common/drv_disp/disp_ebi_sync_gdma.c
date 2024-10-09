/**************************************************************************//**
 * @file     disp_sync_ebi.c
 * @brief    Use EBI-16 with PDMA-M2M to simulate sync-type LCD timing.
 *
 * Note: Just only test with NuMaker-HMI-M55M1ES board.
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "dma350_lib.h"
#include "dma350_ch_drv.h"
#include "disp.h"

/* Don't touch me */
#define CONFIG_DISP_VSYNC_BITMASK        (1<<CONFIG_DISP_VSYNC_BITIDX)
#define CONFIG_DISP_HSYNC_BITMASK        (1<<CONFIG_DISP_HSYNC_BITIDX)
#define CONFIG_DISP_DE_BITMASK           (1<<CONFIG_DISP_DE_BITIDX)
#define CONFIG_DISP_VSYNC_ACTIVE         (CONFIG_DISP_VPW_ACTIVE_LOW?(-1*CONFIG_DISP_VSYNC_BITMASK):CONFIG_DISP_VSYNC_BITMASK)
#define CONFIG_DISP_HSYNC_ACTIVE         (CONFIG_DISP_HPW_ACTIVE_LOW?(-1*CONFIG_DISP_HSYNC_BITMASK):CONFIG_DISP_HSYNC_BITMASK)
#define CONFIG_DISP_DE_ACTIVE            (CONFIG_DISP_DE_ACTIVE_LOW?(-1*CONFIG_DISP_DE_BITMASK):CONFIG_DISP_DE_BITMASK)
#define CONFIG_DISP_EBI_ADDR             (EBI_BANK0_BASE_ADDR+(CONFIG_DISP_EBI*EBI_MAX_SIZE) + \
                                         (CONFIG_DISP_VPW_ACTIVE_LOW<<CONFIG_DISP_VSYNC_BITIDX) + \
                                         (CONFIG_DISP_HPW_ACTIVE_LOW<<CONFIG_DISP_HSYNC_BITIDX) + \
                                         (CONFIG_DISP_DE_ACTIVE_LOW<<CONFIG_DISP_DE_BITIDX))

#define CONFIG_VRAM_BUF_SIZE             (CONFIG_TIMING_HACT * CONFIG_TIMING_VACT * sizeof(uint16_t))

typedef enum
{
    evHStageHFP,
    evHStageHSYNC,
    evHStageHBP,
    evHStageHACT,
    evHStageCNT
} E_HSTAGE;

typedef enum
{
    evVStageVFP,
    evVStageVSYNC,
    evVStageVBP,
    evVStageVACT,
    evVStageCNT
} E_VSTAGE;

typedef struct
{
#define DEF_CMDBUF_SIZE   16
    uint32_t m_cmdbuf[DEF_CMDBUF_SIZE];
} S_CMDBUF;

typedef struct
{
    S_CMDBUF m_dscH[evHStageCNT];
} S_DSC_HLINE;

typedef struct
{
#define  DEF_TOTAL_VLINES   (CONFIG_TIMING_VPW+CONFIG_TIMING_VBP+CONFIG_TIMING_VACT+CONFIG_TIMING_VFP)
    S_DSC_HLINE    m_dscV[DEF_TOTAL_VLINES];
} S_DSC_LCD;

#if defined(NVT_NONCACHEABLE)
    NVT_NONCACHEABLE static S_DSC_LCD s_sDscLCD;
#endif
static uint32_t s_u32DummyData = 0xffffffff;
static S_CMDBUF *s_head = (S_CMDBUF *) &s_sDscLCD;
static S_CMDBUF *s_end  = (S_CMDBUF *) &s_sDscLCD + (sizeof(s_sDscLCD) / sizeof(S_CMDBUF) - 1);
static volatile uint16_t *s_pu16BufAddr = NULL;
static void(*s_pfnBlankCb)(void *p);

static const uint32_t s_au32HTiming[evHStageCNT] =
{
    CONFIG_TIMING_HFP,
    CONFIG_TIMING_HPW,
    CONFIG_TIMING_HBP,
#define DEF_HACT_INDEX   (CONFIG_TIMING_HFP+CONFIG_TIMING_HPW+CONFIG_TIMING_HBP)
    CONFIG_TIMING_HACT
};

static const uint32_t s_au32VTiming[evVStageCNT] =
{
    CONFIG_TIMING_VFP,
    CONFIG_TIMING_VPW,
    CONFIG_TIMING_VBP,
#define DEF_VACT_INDEX   (CONFIG_TIMING_VFP+CONFIG_TIMING_VPW+CONFIG_TIMING_VBP)
    CONFIG_TIMING_VACT
};

static E_VSTAGE get_current_vstage(int i32LineIdx)
{
    int sum = 0;
    E_VSTAGE i;

    for (i = 0; i < evVStageCNT; i++)
    {
        sum += s_au32VTiming[i];
        if (i32LineIdx < sum)
        {
            return i;
        }
    }

    return 0;
}

extern struct dma350_ch_dev_t *const GDMA_CH_DEV_S[];

static void disp_gdma_dsc_init(void)
{

    int i;
    uint16_t *pu16Buf = (uint16_t *)s_pu16BufAddr;
    S_CMDBUF *next = s_head; // first descriptor.
    struct dma350_cmdlink_gencfg_t cmdlink_cfg;

    for (i = 0; i < DEF_TOTAL_VLINES; i++)
    {
        E_HSTAGE evH;
        E_VSTAGE evV = get_current_vstage(i);

        /* Set each VSYNC lines. */
        for (evH = 0; evH < evHStageCNT; evH++)
        {
            uint32_t u32AddrSrc = (uint32_t)&s_u32DummyData;
            uint32_t u32AddrDst;
            uint32_t u32XferCount = s_au32HTiming[evH];
            uint16_t u16AddrSrcInc = 0;
            uint16_t u16AddrDstInc = 0;

            switch (evV)
            {
            case evVStageVSYNC:
                /* Set each H stage in a VSYNC line. */
                /* Set source memory address is fixed and destination memory address is fixed. */
                u32AddrDst = (evH == evHStageHSYNC) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_VSYNC_ACTIVE + CONFIG_DISP_HSYNC_ACTIVE) :
                             (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_VSYNC_ACTIVE);
                break;

            case evVStageVBP:
                /* Set each H stage in a VBP line. */
                /* Set source memory address is fixed and destination memory address is fixed. */
                u32AddrDst = (evH == evHStageHSYNC) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_HSYNC_ACTIVE) : (CONFIG_DISP_EBI_ADDR);
                break;

            case evVStageVACT:
                /* Set each H stage in a VACT line. */
                /* evHStageHACT stage: Set source memory address is incremented and destination memory address is fixed. */
                /* Others stage: Set source memory address is fixed and destination memory address is fixed. */
                if (evH == evHStageHACT)
                {
                    u32AddrSrc = (uint32_t)pu16Buf;
                    pu16Buf = pu16Buf + CONFIG_TIMING_HACT;
                }

                u32AddrDst = (evH == evHStageHSYNC) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_HSYNC_ACTIVE) :
                             (evH == evHStageHACT) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_DE_ACTIVE) :
                             (CONFIG_DISP_EBI_ADDR);
                u16AddrSrcInc = (evH == evHStageHACT) ? 1 : 0;
                break;

            case evVStageVFP:
                /* Set each H stage in a VFP line. */
                /* Set source memory address is fixed and destination memory address is fixed. */
                u32AddrDst = (evH == evHStageHSYNC) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_HSYNC_ACTIVE) : (CONFIG_DISP_EBI_ADDR);
                break;

            default:
                break;
            }

            dma350_cmdlink_init(&cmdlink_cfg);
            dma350_cmdlink_set_regclear(&cmdlink_cfg);
            dma350_cmdlink_set_src_des(&cmdlink_cfg, (const void *)u32AddrSrc, (void *)u32AddrDst, u32XferCount, u32XferCount);
            dma350_cmdlink_set_xsize16(&cmdlink_cfg, (uint16_t)u32XferCount, (uint16_t)u32XferCount);
            dma350_cmdlink_set_transize(&cmdlink_cfg, DMA350_CH_TRANSIZE_16BITS);
            dma350_cmdlink_set_xtype(&cmdlink_cfg, DMA350_CH_XTYPE_CONTINUE);
            dma350_cmdlink_set_ytype(&cmdlink_cfg, DMA350_CH_YTYPE_DISABLE);
            dma350_cmdlink_set_xaddrinc(&cmdlink_cfg, u16AddrSrcInc, u16AddrDstInc);

            dma350_cmdlink_enable_linkaddr(&cmdlink_cfg);

            if (next == s_end)
            {
                dma350_cmdlink_enable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
                dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, (uint32_t)s_head);
            }
            else
            {
                dma350_cmdlink_disable_intr(&cmdlink_cfg, DMA350_CH_INTREN_DONE);
                dma350_cmdlink_set_linkaddr32(&cmdlink_cfg, (uint32_t)(next + 1));
            }

            dma350_cmdlink_generate(&cmdlink_cfg, (uint32_t *)next, (uint32_t *)((uint32_t)next + sizeof(S_CMDBUF) - sizeof(uint32_t)));

            next++;

        } // for (evH = 0; evH < evHStageCNT; evH++)

    } // for (i = 0; i < DEF_TOTAL_VLINES; i++)
}

void disp_set_vrambufaddr(void *pvBufAddr)
{
    s_pu16BufAddr = (volatile uint16_t *)pvBufAddr;
}

void disp_set_blankcb(void *pvpfnBlank)
{
    s_pfnBlankCb = pvpfnBlank;
}

static const char *szGDMADscItemName[] =
{
    "REGCLEAR_SET",     //(0x1UL <<  0)
    "",                 //(0x1UL <<  1)
    "INTREN_SET",       //(0x1UL <<  2)
    "CTRL_SET",         //(0x1UL <<  3)
    "SRC_ADDR_SET",     //(0x1UL <<  4)
    "SRC_ADDRHI_SET",   //(0x1UL <<  5)
    "DES_ADDR_SET",     //(0x1UL <<  6)
    "DES_ADDRHI_SET",   //(0x1UL <<  7)
    "XSIZE_SET",        //(0x1UL <<  8)
    "XSIZEHI_SET",      //(0x1UL <<  9)
    "SRCTRANSCFG_SET",  //(0x1UL << 10)
    "DESTRANSCFG_SET",  //(0x1UL << 11)
    "XADDRINC_SET",     //(0x1UL << 12)
    "YADDRSTRIDE_SET",  //(0x1UL << 13)
    "FILLVAL_SET",      //(0x1UL << 14)
    "YSIZE_SET",        //(0x1UL << 15)
    "TMPLTCFG_SET",     //(0x1UL << 16)
    "SRCTMPLT_SET",     //(0x1UL << 17)
    "DESTMPLT_SET",     //(0x1UL << 18)
    "SRCTRIGINCFG_SET", //(0x1UL << 19)
    "DESTRIGINCFG_SET", //(0x1UL << 20)
    "TRIGOUTCFG_SET",   //(0x1UL << 21)
    "GPOEN0_SET",       //(0x1UL << 22)
    "",                 //(0x1UL << 23)
    "GPOVAL0_SET",      //(0x1UL << 24)
    "",                 //(0x1UL << 25)
    "STREAMINTCFG_SET", //(0x1UL << 26)
    "",                 //(0x1UL << 27)
    "LINKATTR_SET",     //(0x1UL << 28)
    "AUTOCFG_SET",      //(0x1UL << 29)
    "LINKADDR_SET",     //(0x1UL << 30)
    "LINKADDRHI_SET"    //(0x1UL << 31)
};

static void disp_gdma_dsc_dump(void)
{
    int i;
    S_CMDBUF *next = s_head;
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg;

    printf("s_head: %08X, s_end: %08X\n", (uint32_t)s_head, (uint32_t)s_end);
    do
    {
        int n = 0;
        cmdlink_cfg = (struct dma350_cmdlink_gencfg_t *)next;
        uint32_t *pu32Cfg = (uint32_t *)&cmdlink_cfg->cfg;
        S_CMDBUF *tmp_next = NULL;
        uint32_t u32HdrVal = cmdlink_cfg->header & ~0x3; //Start bit2

        printf("[%08x %08x]====================================\n", (uint32_t)next, u32HdrVal);

        while ((i = nu_ctz(u32HdrVal)) < 32)
        {
            printf("[1<<%d] %s -> %08x\n", i, szGDMADscItemName[i], pu32Cfg[n]);

            switch (1 << i)
            {
            case DMA350_CMDLINK_LINKADDR_SET:     //(0x1UL << 30)
                tmp_next = (S_CMDBUF *)((uint32_t)pu32Cfg[n] & DMA_CH_LINKADDR_LINKADDR_Msk);
                break;

            case DMA350_CMDLINK_REGCLEAR_SET:     //(0x1UL)
            case DMA350_CMDLINK_INTREN_SET:       //(0x1UL << 2)
            case DMA350_CMDLINK_CTRL_SET:         //(0x1UL << 3)
            case DMA350_CMDLINK_SRC_ADDR_SET:     //(0x1UL << 4)
            case DMA350_CMDLINK_SRC_ADDRHI_SET:   //(0x1UL << 5)
            case DMA350_CMDLINK_DES_ADDR_SET:     //(0x1UL << 6)
            case DMA350_CMDLINK_DES_ADDRHI_SET:   //(0x1UL << 7)
            case DMA350_CMDLINK_XSIZE_SET:        //(0x1UL << 8)
            case DMA350_CMDLINK_XSIZEHI_SET:      //(0x1UL << 9)
            case DMA350_CMDLINK_SRCTRANSCFG_SET:  //(0x1UL << 10)
            case DMA350_CMDLINK_DESTRANSCFG_SET:  //(0x1UL << 11)
            case DMA350_CMDLINK_XADDRINC_SET:     //(0x1UL << 12)
            case DMA350_CMDLINK_YADDRSTRIDE_SET:  //(0x1UL << 13)
            case DMA350_CMDLINK_FILLVAL_SET:      //(0x1UL << 14)
            case DMA350_CMDLINK_YSIZE_SET:        //(0x1UL << 15)
            case DMA350_CMDLINK_TMPLTCFG_SET:     //(0x1UL << 16)
            case DMA350_CMDLINK_SRCTMPLT_SET:     //(0x1UL << 17)
            case DMA350_CMDLINK_DESTMPLT_SET:     //(0x1UL << 18)
            case DMA350_CMDLINK_SRCTRIGINCFG_SET: //(0x1UL << 19)
            case DMA350_CMDLINK_DESTRIGINCFG_SET: //(0x1UL << 20)
            case DMA350_CMDLINK_TRIGOUTCFG_SET:   //(0x1UL << 21)
            case DMA350_CMDLINK_GPOEN0_SET:       //(0x1UL << 22)
            case DMA350_CMDLINK_GPOVAL0_SET:      //(0x1UL << 24)
            case DMA350_CMDLINK_STREAMINTCFG_SET: //(0x1UL << 26)
            case DMA350_CMDLINK_LINKATTR_SET:     //(0x1UL << 28)
            case DMA350_CMDLINK_AUTOCFG_SET:      //(0x1UL << 29)
            case DMA350_CMDLINK_LINKADDRHI_SET:   //(0x1UL << 31)
            default:
                break;
            }

            n++;
            u32HdrVal &= ~(1 << i);
        }

        if (tmp_next)
            next = tmp_next;
    }
    while (s_head != next);

}

static uint32_t gdma_dsc_find_srcaddr_index(S_CMDBUF *psCmdBuf)
{
    int i;
    int n = 0;
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg = (struct dma350_cmdlink_gencfg_t *)psCmdBuf;
    uint32_t *pu32Cfg = (uint32_t *)&cmdlink_cfg->cfg;
    uint32_t u32HdrVal = cmdlink_cfg->header & ~0x3; //Start bit2

    while ((i = nu_ctz(u32HdrVal)) < 32)
    {
        switch (1 << i)
        {
        case DMA350_CMDLINK_SRC_ADDR_SET:     //(0x1UL << 4)
            return n;
        default:
            break;
        }

        n++;
        u32HdrVal &= ~(1 << i);
    }

    return 0xffffffff;
}

NVT_ITCM void GDMACH1_IRQHandler(void)
{
    union dma350_ch_status_t status = dma350_ch_get_status(GDMA_CH_DEV_S[1]);

    if (status.b.STAT_DONE)
    {
        uint32_t u32SrcBufAddrIdx = gdma_dsc_find_srcaddr_index(&s_sDscLCD.m_dscV[DEF_VACT_INDEX].m_dscH[evHStageHACT]) + 1;

        GDMA_CH_DEV_S[1]->cfg.ch_base->CH_STATUS = DMA350_CH_STAT_DONE;

        if ((s_sDscLCD.m_dscV[DEF_VACT_INDEX].m_dscH[evHStageHACT].m_cmdbuf[u32SrcBufAddrIdx] != (uint32_t)s_pu16BufAddr))
        {
            int i;

            // Switch new VRAM buffer address.
            for (i = 0; i < s_au32VTiming[evVStageVACT]; i++)
            {
                /* Update every lines. */
                s_sDscLCD.m_dscV[DEF_VACT_INDEX + i].m_dscH[evHStageHACT].m_cmdbuf[u32SrcBufAddrIdx] = (uint32_t)&s_pu16BufAddr[i * CONFIG_TIMING_HACT];
            }
        }

        if (s_pfnBlankCb)
            s_pfnBlankCb((void *)s_pu16BufAddr);
    }
    else
    {
    }
}

int disp_init(void)
{
    enum dma350_lib_error_t lib_err;

    /* Initial all Lines descriptor-link. */
    disp_gdma_dsc_init();
    //disp_gdma_dsc_dump();

    /* Link to external command */
    dma350_ch_enable_linkaddr(GDMA_CH_DEV_S[1]);
    dma350_ch_set_linkaddr32(GDMA_CH_DEV_S[1], (uint32_t) s_head);
    dma350_ch_disable_intr(GDMA_CH_DEV_S[1], DMA350_CH_INTREN_DONE);
    dma350_ch_cmd(GDMA_CH_DEV_S[1], DMA350_CH_CMD_ENABLECMD);

    return 0;
}
