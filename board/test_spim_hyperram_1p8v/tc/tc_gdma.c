#include "tc.h"
#include "tc_gdma.h"
#include "dma350_lib.h"

int volatile g_bDone = 0;

/* DMA350 driver structures */
static const struct dma350_dev_cfg_t GDMA_DEV_CFG_S =
{
    .dma_sec_cfg = (DMASECCFG_TypeDef *)(GDMA_S + 0x0UL),
    .dma_sec_ctrl = (DMASECCTRL_TypeDef *)(GDMA_S + 0x100UL),
    .dma_nsec_ctrl = (DMANSECCTRL_TypeDef *)(GDMA_S + 0x200UL),
    .dma_info = (DMAINFO_TypeDef *)(GDMA_S + 0xF00UL)
};

static struct dma350_dev_data_t GDMA_DEV_DATA_S =
{
    .state = 0
};

struct dma350_dev_t GDMA_DEV_S =
{
    &(GDMA_DEV_CFG_S),
    &(GDMA_DEV_DATA_S)
};

static struct dma350_ch_dev_t GDMA_CH0_DEV_S =
{
    .cfg = {
        .ch_base = (DMACH_TypeDef *)(GDMA_S + 0x1000UL),
        .channel = 0
    },
    .data = {0}
};

static struct dma350_ch_dev_t GDMA_CH1_DEV_S =
{
    .cfg = {
        .ch_base = (DMACH_TypeDef *)(GDMA_S + 0x1100UL),
        .channel = 1
    },
    .data = {0}
};

struct dma350_ch_dev_t *const GDMA_CH_DEV_S[] =
{
    &GDMA_CH0_DEV_S,
    &GDMA_CH1_DEV_S
};

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

void tc_gdma_dsc_dump(S_CMDBUF *psCmdBufHead, int i32Length)
{
    int i, n;
    S_CMDBUF *next = psCmdBufHead;
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg;

    TC_PRINTF("\n\npsCmdBufHead: %08X\n", (uint32_t)psCmdBufHead);

    for (n = 0; n < i32Length; n++)
    {
        int n = 0;
        cmdlink_cfg = (struct dma350_cmdlink_gencfg_t *)next;
        uint32_t *pu32Cfg = (uint32_t *)&cmdlink_cfg->cfg;
        S_CMDBUF *tmp_next = NULL;
        uint32_t u32HdrVal = cmdlink_cfg->header & ~0x3; //Start bit2

        TC_PRINTF("\n[%08x %08x]====================================\n", (uint32_t)next, u32HdrVal);

        while ((i = nu_ctz(u32HdrVal)) < 32)
        {
            TC_PRINTF("[1<<%d] %s -> %08x\n", i, szGDMADscItemName[i], pu32Cfg[n]);

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

}

NVT_ITCM void gdma_isr(struct dma350_ch_dev_t *gdma)
{
    union dma350_ch_status_t status = dma350_ch_get_status(gdma);

    if (status.w & ~(DMA_CH_STATUS_STAT_DONE | DMA_CH_STATUS_INTR_DONE))
    {
        TC_PRINTF("%s status.w:0x%08x\n", __func__, status.w);
    }

    if (status.b.STAT_DONE)
    {
        g_bDone = 1;
    }

    gdma->cfg.ch_base->CH_STATUS = DMA350_CH_STAT_ALL;
    __ISB();
    __DSB();
}

NVT_ITCM void GDMACH0_IRQHandler(void)
{
    gdma_isr(GDMA_CH_DEV_S[0]);
}

NVT_ITCM void GDMACH1_IRQHandler(void)
{
    gdma_isr(GDMA_CH_DEV_S[1]);
}

