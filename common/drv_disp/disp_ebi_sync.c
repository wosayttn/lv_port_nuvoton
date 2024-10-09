/**************************************************************************//**
 * @file     disp_sync_ebi.c
 * @brief    Use EBI-16 with PDMA-M2M to simulate sync-type LCD timing.
 *
 * Note: Just only test with NuMaker-HMI-M55M1ES board.
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "disp.h"
#include "drv_pdma.h"

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
    DSCT_T         m_dscH[evHStageCNT];
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
static nu_pdma_desc_t s_head = (nu_pdma_desc_t) &s_sDscLCD;
static nu_pdma_desc_t s_end  = (nu_pdma_desc_t) &s_sDscLCD + (sizeof(s_sDscLCD) / sizeof(DSCT_T) - 1);
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

static void disp_pdma_dsc_dump(void)
{
    nu_pdma_desc_t next = s_head;

    printf("s_head: %08X, s_end: %08X\n", (uint32_t)s_head, (uint32_t)s_end);

    do
    {
        printf("next: %08X, CTL: %08X SA: %08X DA: %08X NEXT: %08X\n",
               (uint32_t)next, next->CTL, next->SA, next->DA, next->NEXT);

        next = (nu_pdma_desc_t)next->NEXT;

    }
    while (s_head != next);

}

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

static void disp_pdma_dsc_init(void)
{

    int i;
    uint16_t *pu16Buf = (uint16_t *)s_pu16BufAddr;
    nu_pdma_desc_t next = s_head; // first descriptor.

    for (i = 0; i < DEF_TOTAL_VLINES; i++)
    {
        E_HSTAGE evH;
        E_VSTAGE evV = get_current_vstage(i);

        /* Set each VSYNC lines. */
        for (evH = 0; evH < evHStageCNT; evH++)
        {
            uint32_t u32AddrSrc = (uint32_t)&s_u32DummyData;
            uint32_t u32AddrDst;
            uint32_t u32DataWidth = 16;
            uint32_t u32XferCount = s_au32HTiming[evH];
            nu_pdma_memctrl_t evMemCtrl = eMemCtl_SrcFix_DstFix;

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
                evMemCtrl = (evH == evHStageHACT) ? eMemCtl_SrcInc_DstFix : eMemCtl_SrcFix_DstFix;
                break;

            case evVStageVFP:
                /* Set each H stage in a VFP line. */
                /* Set source memory address is fixed and destination memory address is fixed. */
                u32AddrDst = (evH == evHStageHSYNC) ? (CONFIG_DISP_EBI_ADDR + CONFIG_DISP_HSYNC_ACTIVE) : (CONFIG_DISP_EBI_ADDR);
                break;

            default:
                break;
            }

            nu_pdma_m2m_desc_setup(next,
                                   u32DataWidth,
                                   u32AddrSrc,
                                   u32AddrDst,
                                   u32XferCount,
                                   evMemCtrl,
                                   next + 1,
                                   1);
            next++;

        } // for (evH = 0; evH < evHStageCNT; evH++)

    } // for (i = 0; i < DEF_TOTAL_VLINES; i++)

    /* Update NEXT of last descritor to link head. */
    s_end->NEXT = (uint32_t)s_head;

    /* Raise a blank-interrupt for switch data buffer if necessary. */
    s_end->CTL &= ~PDMA_DSCT_CTL_TBINTDIS_Msk;
}

void disp_set_vrambufaddr(void *pvBufAddr)
{
    s_pu16BufAddr = (volatile uint16_t *)pvBufAddr;
}

void disp_set_blankcb(void *pvpfnBlank)
{
    s_pfnBlankCb = pvpfnBlank;
}

static void nu_pdma_memfun_cb(void *pvUserData, uint32_t u32Events)
{
    if ((u32Events == NU_PDMA_EVENT_TRANSFER_DONE) && (s_sDscLCD.m_dscV[DEF_VACT_INDEX].m_dscH[evHStageHACT].SA != (uint32_t)s_pu16BufAddr))
    {
        // Switch new VRAM buffer address.
        int i;

        for (i = 0; i < s_au32VTiming[evVStageVACT]; i++)
        {
            /* Update every lines. */
            s_sDscLCD.m_dscV[DEF_VACT_INDEX + i].m_dscH[evHStageHACT].SA = (uint32_t)&s_pu16BufAddr[i * CONFIG_TIMING_HACT];
        }
    }
    else
    {
    }

    if (s_pfnBlankCb)
        s_pfnBlankCb((void *)s_pu16BufAddr);
}

int disp_init(void)
{
    static int s_i32Channel = -1;

    if (s_i32Channel < 0)
    {
        /* Allocate a PDMA channel resource. */
        s_i32Channel = nu_pdma_channel_allocate(PDMA_MEM);

        if (s_i32Channel < 0)
            return -1 ;
    }

    /* Initial all Lines descriptor-link. */
    disp_pdma_dsc_init();

    /* Dump all Lines descriptor-link. */
    // disp_pdma_dsc_dump();

    {
        struct nu_pdma_chn_cb sChnCB;

        /* Register ISR callback function */
        sChnCB.m_eCBType = eCBType_Event;
        sChnCB.m_pfnCBHandler = nu_pdma_memfun_cb;
        sChnCB.m_pvUserData = (void *)NULL;

        nu_pdma_filtering_set(s_i32Channel, NU_PDMA_EVENT_TRANSFER_DONE);
        nu_pdma_callback_register(s_i32Channel, &sChnCB);
    }

    /* Trigger scatter-gather transferring. */
    return nu_pdma_sg_transfer(s_i32Channel, s_head, 0);
}
