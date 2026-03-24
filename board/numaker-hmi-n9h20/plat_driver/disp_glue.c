/**************************************************************************//**
 * @file     disp_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "numaker_disp.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

#if (CONFIG_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;

#if defined(__FREERTOS__)
    static xQueueHandle s_VSyncQ = NULL;
    static uint8_t dummy = 0x87;
#endif

static int lcd_vpost_handler(UINT8 *pu8, UINT32 u32)
{
    s_vu32Displayblank++;

#if defined(__FREERTOS__)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(s_VSyncQ, &dummy, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

    return 0;
}
#endif


#if defined(__FREERTOS__)
    static SemaphoreHandle_t s_xVDMASem = NULL;
#endif

void vdmaISR(unsigned int arg)
{
#if defined(__FREERTOS__)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(s_xVDMASem, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}

int lcd_device_initialize(void)
{
    EDMA_Init();

#if defined(__FREERTOS__)
    s_xVDMASem = xSemaphoreCreateBinary();
    while (s_xVDMASem == NULL);
#endif

    return 0;
}

void EDMA_WaitForCompletion()
{
#if defined(__FREERTOS__)
    while (xSemaphoreTake(s_xVDMASem, portMAX_DELAY) != pdTRUE);
#else
    while (EDMA_IsBusy(0));
#endif
}

int lcd_device_open(void)
{
    LCDFORMATEX lcdFormat = {0};

    /* Specify pixel format. */
#if (DISP_COLOR_DEPTH==16)
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
#elif (DISP_COLOR_DEPTH==32)
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;
#else
#error "Wrong DISP_COLOR_DEPTH definition. Please correct".
#endif

    int ret = vpostLCMInit(&lcdFormat, (UINT32 *)s_au8FrameBuf);
    if (ret != 0)
    {
        sysprintf("vpostLCMInit failed\n");
        while (1);
    }

#if (CONFIG_DISP_FULL_REFRESH==1)

#if defined(__FREERTOS__)
    /* Create a queue of length 1 */
    s_VSyncQ = xQueueGenericCreate(1, sizeof(uint8_t), 0);
    while (s_VSyncQ == NULL);
#endif

    // Enable LCD interrupt
    PFN_DRVVPOST_INT_CALLBACK pfnOld;
    vpostInstallCallBack(eDRVVPOST_VINT, lcd_vpost_handler, &pfnOld);
    vpostEnableInt(eDRVVPOST_VINT);
    sysEnableInterrupt(IRQ_VPOST);

#endif

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
        vpostSetFrameBuffer((UINT32)argv);
    }
    break;

#if (CONFIG_DISP_FULL_REFRESH==1)
    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        {
#if defined(__FREERTOS__)
            /* First make sure the queue is empty, by trying to remove an element with 0 timeout. */
            xQueueReceive(s_VSyncQ, &dummy, 0);

            /* Wait for next VSYNC to occur. */
            xQueueReceive(s_VSyncQ, &dummy, portMAX_DELAY);
#else
            //Wait next blank coming;
            while (s_vu32Displayblank <  next);
#endif
        }
    }
    break;
#endif

    case evLCD_CTRL_RECT_UPDATE:
    {
        sysCleanDcache((UINT32)s_au8FrameBuf, (UINT32)CONFIG_VRAM_TOTAL_ALLOCATED_SIZE);
    }
    break;

    default:
        while (1);
    }

    return 0;
}

void lcd_device_close(void)
{
    vpostLCMDeinit();

#if (CONFIG_DISP_FULL_REFRESH==1)
    sysDisableInterrupt(IRQ_VPOST);
    vpostDisableInt(eDRVVPOST_VINT);
#endif
}

int lcd_device_finalize(void)
{
    EDMA_Exit();

    return 0;
}

