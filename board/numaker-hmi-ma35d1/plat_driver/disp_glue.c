/****************************************************************************
 * @file     disp_glue.c
 * @brief    MA35 display porting
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "disp.h"
#include "displib.h"
#include "numaker_disp.h"

#if defined(__FREERTOS__)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
#include "libgfx.h" /* gfx_open(), gfx_blt(), gfx_fill() */

void *g_gfx_disp_handle = NULL;

#ifndef CONFIG_GFX_POOL_SIZE
#define CONFIG_GFX_POOL_SIZE (4 * 1024 * 1024)
#endif
static uint8_t s_au8GfxPoolBuf[CONFIG_GFX_POOL_SIZE] __attribute__((aligned(128)));

#if defined(__FREERTOS__)
static SemaphoreHandle_t s_gfx_gpu_mutex = NULL;
static SemaphoreHandle_t s_gfx_mem_mutex = NULL;
static SemaphoreHandle_t s_gfx_ctx_mutex = NULL;

static int _gfx_lock_gpu(unsigned int timeout_ms) {
  if (s_gfx_gpu_mutex == NULL)
    return 0;
  TickType_t ticks = (timeout_ms == GFX_OSAL_WAIT_FOREVER)
                         ? portMAX_DELAY
                         : pdMS_TO_TICKS(timeout_ms);
  BaseType_t res = xSemaphoreTakeRecursive(s_gfx_gpu_mutex, ticks);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] lock_gpu timeout (%u ms)!\n", timeout_ms);
    return -1;
  }
  return 0;
}

static int _gfx_unlock_gpu(void) {
  if (s_gfx_gpu_mutex == NULL)
    return 0;
  BaseType_t res = xSemaphoreGiveRecursive(s_gfx_gpu_mutex);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] unlock_gpu failed!\n");
    return -1;
  }
  return 0;
}

static int _gfx_lock_mem(unsigned int timeout_ms) {
  if (s_gfx_mem_mutex == NULL)
    return 0;
  TickType_t ticks = (timeout_ms == GFX_OSAL_WAIT_FOREVER)
                         ? portMAX_DELAY
                         : pdMS_TO_TICKS(timeout_ms);
  BaseType_t res = xSemaphoreTakeRecursive(s_gfx_mem_mutex, ticks);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] lock_mem timeout (%u ms)!\n", timeout_ms);
    return -1;
  }
  return 0;
}

static int _gfx_unlock_mem(void) {
  if (s_gfx_mem_mutex == NULL)
    return 0;
  BaseType_t res = xSemaphoreGiveRecursive(s_gfx_mem_mutex);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] unlock_mem failed!\n");
    return -1;
  }
  return 0;
}

static int _gfx_lock_ctx(unsigned int timeout_ms) {
  if (s_gfx_ctx_mutex == NULL)
    return 0;
  TickType_t ticks = (timeout_ms == GFX_OSAL_WAIT_FOREVER)
                         ? portMAX_DELAY
                         : pdMS_TO_TICKS(timeout_ms);
  BaseType_t res = xSemaphoreTakeRecursive(s_gfx_ctx_mutex, ticks);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] lock_ctx timeout (%u ms)!\n", timeout_ms);
    return -1;
  }
  return 0;
}

static int _gfx_unlock_ctx(void) {
  if (s_gfx_ctx_mutex == NULL)
    return 0;
  BaseType_t res = xSemaphoreGiveRecursive(s_gfx_ctx_mutex);
  if (res != pdTRUE) {
    sysprintf("[GFX_LOCK] unlock_ctx failed!\n");
    return -1;
  }
  return 0;
}

static const gfx_osal_ops_t s_gfx_freertos_ops = {
    .lock_gpu = _gfx_lock_gpu,
    .unlock_gpu = _gfx_unlock_gpu,
    .lock_mem = _gfx_lock_mem,
    .unlock_mem = _gfx_unlock_mem,
    .lock_ctx = _gfx_lock_ctx,
    .unlock_ctx = _gfx_unlock_ctx,
};
#endif
#endif

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE]
    __attribute__((aligned(128)));

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

void sysDelay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
#if (CONFIG_DISP_DIRECT_REFRESH == 1)

#define DISP_ENABLE_INT()                                                      \
  (DISP->DisplayIntrEnable |= DISP_DisplayIntrEnable_DISP0_Msk)
#define DISP_GET_INTSTS() (DISP->DisplayIntr & DISP_DisplayIntr_DISP0_Msk)

static volatile uint32_t s_vu32Displayblank = 0;

static void lcd_disp_handler(void) {
  /* Get DISP INTSTS */
  if (DISP_GET_INTSTS()) {
    s_vu32Displayblank++;
  }
}
#endif

int lcd_device_initialize(void) {
  int ret;

  /* Unlock protected registers */
  SYS_UnlockReg();

  /* Select DISP pixel clock source to VPLL */
  DISP_GeneratePixelClk(LcdPanelInfo.sLcdTiming.u32PCF);

  /* Configure display attributes of LCD panel */
  DISPLIB_LCDInit(LcdPanelInfo);

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
#if defined(__FREERTOS__)
  if (s_gfx_gpu_mutex == NULL)
    s_gfx_gpu_mutex = xSemaphoreCreateRecursiveMutex();
  if (s_gfx_mem_mutex == NULL)
    s_gfx_mem_mutex = xSemaphoreCreateRecursiveMutex();
  if (s_gfx_ctx_mutex == NULL)
    s_gfx_ctx_mutex = xSemaphoreCreateRecursiveMutex();

  gfx_osal_register_ops(&s_gfx_freertos_ops);
#endif

  /* Initialize GFX Hardware Engine and Memory Allocator with Static Buffer Pool
   */
  int init_ret = libgfx_init(s_au8GfxPoolBuf, sizeof(s_au8GfxPoolBuf));
  sysprintf("[GFX Init Status] libgfx_init returned: %d\r\n", init_ret);
  if (init_ret != 0) {
    sysprintf("[FATAL] libgfx_init failed (%d), aborting GFX test suite!\r\n",
              init_ret);
    while (1) {
    }
  }
#endif

  /* Configure DISP Framebuffer settings  */
  DISPLIB_SetFBConfig(
#if (DISP_COLOR_DEPTH == 32)
      eFBFmt_A8R8G8B8,
#else
      eFBFmt_RGB565,
#endif
      DISP_HOR_RES_MAX, DISP_VER_RES_MAX, ptr_to_u32(s_au8FrameBuf));

#if (CONFIG_DISP_DIRECT_REFRESH == 1)
  IRQ_SetHandler((IRQn_ID_t)DISP_IRQn, lcd_disp_handler);
  IRQ_Enable((IRQn_ID_t)DISP_IRQn);
  DISP_ENABLE_INT();
#endif

#if defined(LV_USE_DRAW_GFX) && (LV_USE_DRAW_GFX == 1)
  /* Open libgfx device for display flush operations */
  ret = gfx_open(&g_gfx_disp_handle);
  if (ret != 0) {
    sysprintf("gfx_open for disp failed:%d\n", ret);
    goto fail;
  }

  sysprintf("GFX disp handle initialized: 0x%p (libgfx)!!\n", g_gfx_disp_handle);
#endif

  return 0;

fail:
  return -1;
}

int lcd_device_open(void) {
  /* Start to display */
  DISPLIB_EnableOutput(eLayer_Video);

  return 0;
}

int lcd_device_control(int cmd, void *argv) {
  switch (cmd) {
  case evLCD_CTRL_GET_INFO: {
    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)argv;

    psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
    psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
    psLCDInfo->u32ResWidth = DISP_HOR_RES_MAX;
    psLCDInfo->u32ResHeight = DISP_VER_RES_MAX;
    psLCDInfo->u32BytePerPixel = (DISP_COLOR_DEPTH / 8);
    psLCDInfo->evLCDType = evLCD_TYPE_SYNC;
  } break;

  case evLCD_CTRL_PAN_DISPLAY: {
    DISPLIB_SetFBAddr(ptr_to_u32(argv));
  } break;

#if (CONFIG_DISP_DIRECT_REFRESH == 1)
  case evLCD_CTRL_WAIT_VSYNC: {
    volatile uint32_t next = s_vu32Displayblank + 1;
    while (s_vu32Displayblank < next) {
      // Wait next blank coming;
    }
  } break;
#endif

  case evLCD_CTRL_RECT_UPDATE: {
    dcache_clean_by_mva(s_au8FrameBuf, CONFIG_VRAM_TOTAL_ALLOCATED_SIZE);
  } break;

  default:
    while (1) {
      // Invalid command. Just block here.
    };
  }

  return 0;
}

void lcd_device_close(void) {
  /* Stop to display */
  DISPLIB_DisableOutput(eLayer_Video);
}

int lcd_device_finalize(void) {
#if (CONFIG_DISP_DIRECT_REFRESH == 1)
  IRQ_Disable((IRQn_ID_t)DISP_IRQn);
#endif

  return 0;
}
