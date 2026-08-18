/**************************************************************************//**
 * @file     disp_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "numaker_disp.h"
#include "disp.h"
#include "displib.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#if defined(LV_USE_DRAW_GFX)
    #include "gfxlib.h"     /* gfxlib_init(), GFXLIB_Clear(), GFXLIB_Blit(), GFX_STATUS_* */
    #include "gfx_osal.h"   /* gfx_osal_printf(), gfx_va_to_pa() */
    #include "gfx_mem.h"    /* gfx_mem_init(), gfx_mem_alloc() */
    #include "gfx_disp.h"   /* gfx_disp_init(), gfx_disp_enable() */

    #define GFX_2D_DISPDEMO_POOL_SIZE    (512*1024)      /* 512 KB */
    static uint8_t s_au8GfxBuf[GFX_2D_DISPDEMO_POOL_SIZE] __attribute__((aligned(64)));
#endif

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(128)));

/* LCD attributes 1024x600 */
#if defined(__1024x600__)

const static DISP_LCD_INFO LcdPanelInfo =
{
    /* Panel Resolution */
    1024,
    600,
    /* DISP_LCD_TIMING */
    {
        51000000,
        1024,
        1,
        160,
        160,
        600,
        1,
        23,
        12,
        ePolarity_Positive,
        ePolarity_Positive
    },
    /* DISP_PANEL_CONF */
    {
        eDPIFmt_D24,
        ePolarity_Positive,
        ePolarity_Positive,
        ePolarity_Positive
    },
};
#endif

void sysDelay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
#if (CONFIG_DISP_FULL_REFRESH==1)

#define DISP_ENABLE_INT()     (DISP->DisplayIntrEnable |=  DISP_DisplayIntrEnable_DISP0_Msk)
#define DISP_GET_INTSTS()     (DISP->DisplayIntr & DISP_DisplayIntr_DISP0_Msk)

static volatile uint32_t s_vu32Displayblank = 0;
static void lcd_disp_handler(void)
{
    /* Get DISP INTSTS */
    if (DISP_GET_INTSTS())
    {
        s_vu32Displayblank++;
    }
}
#endif

int lcd_device_initialize(void)
{
    int         ret;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Select DISP pixel clock source to VPLL */
    DISP_GeneratePixelClk(LcdPanelInfo.sLcdTiming.u32PCF);

    /* Configure display attributes of LCD panel */
    DISPLIB_LCDInit(LcdPanelInfo);

    /* Configure DISP Framebuffer settings  */
    DISPLIB_SetFBConfig(
#if (DISP_COLOR_DEPTH==32)
        eFBFmt_A8R8G8B8,
#else
        eFBFmt_RGB565,
#endif
        DISP_HOR_RES_MAX,
        DISP_VER_RES_MAX,
        ptr_to_u32(s_au8FrameBuf));

#if (CONFIG_DISP_FULL_REFRESH==1)
    IRQ_SetHandler((IRQn_ID_t)DISP_IRQn, lcd_disp_handler);
    IRQ_Enable((IRQn_ID_t)DISP_IRQn);
    DISP_ENABLE_INT();
#endif

#if defined(LV_USE_DRAW_GFX)
    /* Full SoC-level GC520L bring-up. gfx_disp_init() must run after this
     * (gfx_disp.h precondition; gfx_hw.c is the sole GPU/EPLL authority). */
    ret = gfxlib_init();
    if (ret != GFX_STATUS_OK)
    {
        gfx_osal_printf("gfxlib_init:%d\n", ret);
        goto fail;
    }

    /* Initialise the GPU-visible uncached pool -- must exist before
     * gfx_disp_init() is called (gfx_disp.h precondition). */
    ret = gfx_mem_init((gfx_va_t)(0x100000000ULL + s_au8GfxBuf), GFX_2D_DISPDEMO_POOL_SIZE);
    if (ret != GFX_STATUS_OK)
    {
        gfx_osal_printf("GFX_MEM_UNCACHED_ALIAS_BASE=0x%09llx\n", (unsigned long long)(uintptr_t)GFX_MEM_UNCACHED_ALIAS_BASE);
        gfx_osal_printf("s_au8GfxBuf=0x%09llx\n", (unsigned long long)(uintptr_t)s_au8GfxBuf);
        gfx_osal_printf("gfx_mem_init:%d, VA=0x%09llx\n", ret, (unsigned long long)(uintptr_t)(GFX_MEM_UNCACHED_ALIAS_BASE + s_au8GfxBuf));
        goto fail;
    }

    gfx_osal_printf("GFX initialized!!\n");
#endif

    return 0;

fail:
    return -1;
}

int lcd_device_open(void)
{
    /* Start to display */
    DISPLIB_EnableOutput(eLayer_Video);

    return 0;
}

int lcd_device_control(int cmd, void *argv)
{
    switch (cmd)
    {
    case evLCD_CTRL_GET_INFO:
    {
        S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)argv;

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = DISP_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = DISP_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (DISP_COLOR_DEPTH / 8);
        psLCDInfo->evLCDType = evLCD_TYPE_SYNC;
    }
    break;

    case evLCD_CTRL_PAN_DISPLAY:
    {
        DISPLIB_SetFBAddr(ptr_to_u32(argv));
    }
    break;

#if (CONFIG_DISP_FULL_REFRESH==1)
    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        while (s_vu32Displayblank <= next)
        {
            //Wait next blank coming;
        }
    }
    break;
#endif

    case evLCD_CTRL_RECT_UPDATE:
    {
        dcache_clean_by_mva(s_au8FrameBuf, CONFIG_VRAM_TOTAL_ALLOCATED_SIZE);
    }
    break;

    default:
        while (1)
        {
            //Invalid command. Just block here.
        };
    }

    return 0;
}

void lcd_device_close(void)
{
    /* Stop to display */
    DISPLIB_DisableOutput(eLayer_Video);
}

int lcd_device_finalize(void)
{
#if (CONFIG_DISP_FULL_REFRESH==1)
    IRQ_Disable((IRQn_ID_t)DISP_IRQn);
#endif

    return 0;
}
