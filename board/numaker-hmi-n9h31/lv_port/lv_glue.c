/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for N9H31 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * (LV_COLOR_DEPTH/8) * CONFIG_LCD_FB_NUM), DEF_CACHE_LINE_SIZE)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

extern S_CALIBRATION_MATRIX g_sCalMat;
#if defined(__800x480__)

#define USE_DISP_PANEL      5

#if (USE_DISP_PANEL==5)
S_CALIBRATION_MATRIX g_sCalMat = { 13742, -81, -1906400, -103, -8624, 33161548, 65536  };
#elif (USE_DISP_PANEL==7)
S_CALIBRATION_MATRIX g_sCalMat = { 13321, -53, -1069280, 96, 8461, -1863312, 65536 };
#endif

#endif

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;

#if (LV_USE_OS==LV_OS_FREERTOS)
    static xQueueHandle s_VSyncQ = NULL;
    static uint8_t dummy = 0x87;
#endif

static void lcd_vpost_handler(void)
{
    /* clear VPOST interrupt state */
    uint32_t uintstatus = inpw(REG_LCM_INT_CS);

    if (uintstatus & VPOSTB_DISP_F_INT)
    {
        outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_DISP_F_INT);
        s_vu32Displayblank++;

#if (LV_USE_OS==LV_OS_FREERTOS)
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        xQueueSendFromISR(s_VSyncQ, &dummy, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

    }
    else if (uintstatus & VPOSTB_BUS_ERROR_INT)
    {
        outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_BUS_ERROR_INT);
    }
}
#endif


void dump_lcd_timings(void)
{
    int id;

    for (id = 0 ; id < DIS_PANEL_CNT; id++)
    {
        VPOST_BF_T *psLCMInstance = (VPOST_BF_T *) vpostLCMGetInstance(id);
        sysprintf("[%d]============================================================================\n", id);

        sysprintf("CRTCSIZE: 0x%08X\n", psLCMInstance->u32Reg_CRTCSIZE);
        sysprintf("\tHorizontal Total Pixels: %d (HBP+XRES+HFP)\n", psLCMInstance->sCRTCSIZE.HTT);
        sysprintf("\tVertical Total Pixels: %d (VBP+YRES+VFP)\n", psLCMInstance->sCRTCSIZE.VTT);

        sysprintf("CRTCDEND: 0x%08X\n", psLCMInstance->u32Reg_CRTCDEND);
        sysprintf("\tHorizontal Display Enable End: %d (XRES)\n", psLCMInstance->sCRTCDEND.HDEND);
        sysprintf("\tVertical Display Enable End: %d (YRES)\n", psLCMInstance->sCRTCDEND.VDEND);

        sysprintf("CRTCHR: 0x%08X\n", psLCMInstance->u32Reg_CRTCHR);
        sysprintf("\tInternal Horizontal Retrace Start Timing: %d (XRES+1)\n", psLCMInstance->sCRTCHR.HRS);
        sysprintf("\tInternal Horizontal Retrace End Low: %d (XRES+5)\n", psLCMInstance->sCRTCHR.HRE);

        sysprintf("CRTCHSYNC: 0x%08X\n", psLCMInstance->u32Reg_CRTCHSYNC);
        sysprintf("\tHorizontal Sync Start Timing: %d (XRES+HFP+0)\n", psLCMInstance->sCRTCHSYNC.HSYNC_S);
        sysprintf("\tHorizontal Sync End Timing: %d (XRES+HFP+HPW)\n", psLCMInstance->sCRTCHSYNC.HSYNC_E);
        sysprintf("\tHsync Signal Adjustment For Multi-Cycles Per Pixel Mode Of Sync-Based Unipac-LCD: %d\n", psLCMInstance->sCRTCHSYNC.HSYNC_SHIFT);

        sysprintf("CRTCVR: 0x%08X\n", psLCMInstance->u32Reg_CRTCVR);
        sysprintf("\tInternal Vertical Retrace Start Timing: %d (YRES+VFP+0)\n", psLCMInstance->sCRTCVR.VRS);
        sysprintf("\tInternal Vertical Retrace End Low: %d (YRES+VFP+VPW)\n", psLCMInstance->sCRTCVR.VRE);

        sysprintf("LCD_TIMING_WIDTH(XRES): %d\n", psLCMInstance->sCRTCDEND.HDEND);
        sysprintf("LCD_TIMING_HEIGHT(YRES): %d\n", psLCMInstance->sCRTCDEND.VDEND);
        sysprintf("LCD_TIMING_MARGIN_LEFT(HBP): %d\n", psLCMInstance->sCRTCSIZE.HTT - psLCMInstance->sCRTCHSYNC.HSYNC_S);
        sysprintf("LCD_TIMING_MARGIN_RIGHT(HFP): %d\n", psLCMInstance->sCRTCHSYNC.HSYNC_S - psLCMInstance->sCRTCDEND.HDEND);
        sysprintf("LCD_TIMING_HSYNC_LEN(HPW): %d\n", psLCMInstance->sCRTCHSYNC.HSYNC_E - psLCMInstance->sCRTCHSYNC.HSYNC_S);

        sysprintf("LCD_TIMING_MARGIN_UPPER(VBP): %d\n", psLCMInstance->sCRTCSIZE.VTT - psLCMInstance->sCRTCVR.VRS);
        sysprintf("LCD_TIMING_MARGIN_LOWER(VFP): %d\n", psLCMInstance->sCRTCVR.VRS - psLCMInstance->sCRTCDEND.VDEND);
        sysprintf("LCD_TIMING_VSYNC_LEN(VPW): %d\n", psLCMInstance->sCRTCVR.VRE - psLCMInstance->sCRTCVR.VRS);

        sysprintf("[%d]@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n", id);

    }
}

#if (LV_USE_OS==LV_OS_FREERTOS)
    static SemaphoreHandle_t s_xGDMASem = NULL;
#endif

static void gdma0ISR(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    outpw(REG_GDMA_INTCS, 0x100);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(s_xGDMASem, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}

void GDMA_M2M_Transfer(uint32_t u32DestAddr, uint32_t u32SrcAddr, uint32_t u32TransferSize)
{
    sysCleanDcache((UINT32)u32SrcAddr, u32TransferSize);

    /* Disable/Reset GDMA Channel 0 */
    outpw(REG_GDMA_CTL0,  0);

    /* GDMA Channel 0 Source Base Address Register */
    outpw(REG_GDMA_SRCB0, (uint32_t)u32SrcAddr);

    /* GDMA Channel 0 Destination Base Address Register */
    outpw(REG_GDMA_DSTB0, (uint32_t)u32DestAddr);

    /* GDMA Channel 0 Transfer byte count */
    outpw(REG_GDMA_TCNT0, u32TransferSize >> 5);

    /* Configurae GDMAEN, BME, TWS=2, Data-width=32.*/
    outpw(REG_GDMA_CTL0, 0x2003);

#if (LV_USE_OS==LV_OS_FREERTOS)
    /* Enable Channel-0 interrupt .*/
    outpw(REG_GDMA_INTCS, inpw(REG_GDMA_INTCS) | 0x1);
#else
    /* Disable Channel-0 interrupt .*/
    outpw(REG_GDMA_INTCS, inpw(REG_GDMA_INTCS) & ~0x1);
#endif

    /* SoftReq field of GDMA Channel 0.*/
    outpw(REG_GDMA_CTL0, inpw(REG_GDMA_CTL0) | 0x10000);
}

void GDMA_WaitForCompletion(void)
{
#if (LV_USE_OS==LV_OS_FREERTOS)
    while (xSemaphoreTake(s_xGDMASem, portMAX_DELAY) != pdTRUE);
#else
    while (!(inpw(REG_GDMA_INTCS) & 0x100));
    outpw(REG_GDMA_INTCS, 0x100);
#endif
}

int lcd_device_initialize(void)
{
    int i32DisplayPanel;

    /* GDMA engine clock */
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x00001000);

#if (LV_USE_OS==LV_OS_FREERTOS)
    s_xGDMASem = xSemaphoreCreateBinary();
    LV_ASSERT(s_xGDMASem != NULL);
	
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, GDMA0_IRQn, (PVOID)gdma0ISR);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(GDMA0_IRQn);
#endif

#if defined(__800x480__)

    // Override
#if (USE_DISP_PANEL==5)

#define LCD_TIMING_WIDTH         800   /*!< XRES */
#define LCD_TIMING_HEIGHT        480   /*!< YRES */

#define LCD_TIMING_MARGIN_LEFT   88    /*!< HBP (Horizontal Back Porch) */
#define LCD_TIMING_MARGIN_RIGHT  255   /*!< HFP (Horizontal Front Porch) */
#define LCD_TIMING_HSYNC_LEN     255   /*!< HPW (HSYNC plus width) */

#define LCD_TIMING_MARGIN_UPPER  32    /*!< VBP (Vertical Back Porch) */
#define LCD_TIMING_MARGIN_LOWER  13    /*!< VFP (Vertical Front Porch) */
#define LCD_TIMING_VSYNC_LEN     3     /*!< VPW (VSYNC width) */
    i32DisplayPanel = DIS_PANEL_E50A2V1;

#elif (USE_DISP_PANEL==7)

#define LCD_TIMING_WIDTH         800   /*!< XRES */
#define LCD_TIMING_HEIGHT        480   /*!< YRES */

#define LCD_TIMING_MARGIN_LEFT   40    /*!< HBP (Horizontal Back Porch) */
#define LCD_TIMING_MARGIN_RIGHT  196   /*!< HFP (Horizontal Front Porch) */
#define LCD_TIMING_HSYNC_LEN     40    /*!< HPW (HSYNC plus width) */

#define LCD_TIMING_MARGIN_UPPER  23    /*!< VBP (Vertical Back Porch) */
#define LCD_TIMING_MARGIN_LOWER  19    /*!< VFP (Vertical Front Porch) */
#define LCD_TIMING_VSYNC_LEN     3     /*!< VPW (VSYNC width) */
    i32DisplayPanel = DIS_PANEL_FW070TFT;

#endif

#if 0
    VPOST_BF_T *psLCMInstance             = (VPOST_BF_T *)vpostLCMGetInstance(i32DisplayPanel);
    psLCMInstance->sCRTCSIZE.HTT          = LCD_TIMING_MARGIN_LEFT + LCD_TIMING_WIDTH + LCD_TIMING_MARGIN_RIGHT;
    psLCMInstance->sCRTCSIZE.VTT          = LCD_TIMING_MARGIN_UPPER + LCD_TIMING_HEIGHT + LCD_TIMING_MARGIN_LOWER;

    psLCMInstance->sCRTCDEND.HDEND        = LCD_TIMING_WIDTH;
    psLCMInstance->sCRTCDEND.VDEND        = LCD_TIMING_HEIGHT;

    psLCMInstance->sCRTCHR.HRS            = LCD_TIMING_WIDTH + 1;
    psLCMInstance->sCRTCHR.HRE            = LCD_TIMING_WIDTH + 5;

    psLCMInstance->sCRTCHSYNC.HSYNC_S     = LCD_TIMING_WIDTH + LCD_TIMING_MARGIN_RIGHT;
    psLCMInstance->sCRTCHSYNC.HSYNC_E     = LCD_TIMING_WIDTH + LCD_TIMING_MARGIN_RIGHT + LCD_TIMING_HSYNC_LEN;
    psLCMInstance->sCRTCHSYNC.HSYNC_SHIFT = 0 ;

    psLCMInstance->sCRTCVR.VRS            = LCD_TIMING_HEIGHT + LCD_TIMING_MARGIN_LOWER;
    psLCMInstance->sCRTCVR.VRE            = LCD_TIMING_HEIGHT + LCD_TIMING_MARGIN_LOWER + LCD_TIMING_VSYNC_LEN;

#endif

#elif defined(__480x272__)

    i32DisplayPanel = DIS_PANEL_FW043TFT;

#else

#error "Unsupported resolution definition. Please correct".

#endif

    vpostLCMInit(i32DisplayPanel);

    // Set scale to 1:1
    vpostVAScalingCtrl(1, 0, 1, 0, VA_SCALE_INTERPOLATION);

#if (LV_COLOR_DEPTH==32)
    vpostSetVASrc(VA_SRC_RGB888);
#elif (LV_COLOR_DEPTH==16)
    vpostSetVASrc(VA_SRC_RGB565);
#endif

    vpostSetFrameBuffer(s_au8FrameBuf);

#if (CONFIG_LV_DISP_FULL_REFRESH==1)

#if (LV_USE_OS==LV_OS_FREERTOS)
    /* Create a queue of length 1 */
    s_VSyncQ = xQueueGenericCreate(1, sizeof(uint8_t), 0);
    LV_ASSERT(s_VSyncQ != NULL);
#endif

    // Enable LCD interrupt
    outpw(REG_LCM_DCCS, inpw(REG_LCM_DCCS) | VPOSTB_DISP_INT_EN);
    outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_DISP_F_EN);

    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, LCD_IRQn, (PVOID)lcd_vpost_handler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(LCD_IRQn);
#endif

    return 0;
}

int lcd_device_open(void)
{
    vpostVAStartTrigger();

    return 0;
}

int lcd_device_control(int cmd, void *argv)
{
    switch (cmd)
    {
    case evLCD_CTRL_GET_INFO:
    {
        S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)argv;

        LV_ASSERT(argv != NULL);

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = LV_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = LV_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (LV_COLOR_DEPTH / 8);
        psLCDInfo->evLCDType = evLCD_TYPE_SYNC;
    }
    break;

    case evLCD_CTRL_PAN_DISPLAY:
    {
        LV_ASSERT(argv != NULL);
        vpostSetFrameBuffer((uint8_t *)argv);
    }
    break;

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        {
#if (LV_USE_OS==LV_OS_FREERTOS)
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
        LV_ASSERT(0);
    }

    return 0;
}

void lcd_device_close(void)
{
    vpostVAStopTrigger();
    vpostLCMDeinit();
}

int lcd_device_finalize(void)
{
    return 0;
}

int touchpad_device_initialize(void)
{
    return 0;
}

static volatile bool s_bPenDown = false;
int32_t PenDownCallback(UINT32 status, UINT32 userData)
{
    if (!s_bPenDown)
        adcIoctl(PEPOWER_OFF, 0, 0);

    s_bPenDown = true;

    return 0;
}

int touchpad_device_open(void)
{
    adcOpen();

    /* ADC Sample Counter - Set the counter value to extend the ADC start signal period to get more sampling time for precise conversion. */
    outpw(REG_ADC_CONF, (inpw(REG_ADC_CONF) & ~(0xfful << 24)) | 0xfful << 24);

    adcIoctl(T_ON, 0, 0);
    adcIoctl(Z_ON, 0, 0);
    adcIoctl(PEDEF_ON, (UINT32)PenDownCallback, 0);

    s_bPenDown = false;
    adcIoctl(PEPOWER_ON, 0, 0);

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    return 0;
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
#define ADC_TOUCH_Z0_ACTIVE 20
    static lv_indev_data_t sLastInDevData = {0};

    int32_t adc_x, adc_y, z0, z1;

    LV_ASSERT(psInDevData);

    adc_x = adc_y = z0 = z1 = 0;

    psInDevData->state = LV_INDEV_STATE_RELEASED;

    if (s_bPenDown)
    {
        adcIoctl(START_MST, 0, 0);
        adcReadXY((short *)&adc_x, (short *)&adc_y, 1);
        adcReadZ((short *)&z0, (short *)&z1, 1);

        if (z0 < ADC_TOUCH_Z0_ACTIVE)
        {
            s_bPenDown = false;

            adcIoctl(PEPOWER_ON, 0, 0);
        }
        else
        {
            psInDevData->state = LV_INDEV_STATE_PRESSED;
        }
    }

    if (psInDevData->state == LV_INDEV_STATE_PRESSED)
    {
        extern int ad_touch_map(int32_t *sumx, int32_t *sumy);
        if (ad_touch_map((int32_t *)&adc_x, (int32_t *)&adc_y) == 0)
        {

            psInDevData->point.x = ((int16_t)adc_x < 0) ? 0 :
                                   ((int16_t)adc_x >= LV_HOR_RES_MAX) ? (LV_HOR_RES_MAX - 1) :
                                   adc_x;

            psInDevData->point.y = ((int16_t)adc_y < 0) ? 0 :
                                   ((int16_t)adc_y >= LV_VER_RES_MAX) ? (LV_VER_RES_MAX - 1) :
                                   adc_y;
        }
        else
        {
            psInDevData->point.x = (int16_t)adc_x;

            psInDevData->point.y = (int16_t)adc_y;
        }

        sLastInDevData.point.x  = psInDevData->point.x;
        sLastInDevData.point.y  = psInDevData->point.y;
    }
    else
    {
        psInDevData->point.x = sLastInDevData.point.x;
        psInDevData->point.y = sLastInDevData.point.y;
    }

    LV_LOG_INFO("%s (%d, %d)", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    adcClose();
}

int touchpad_device_finalize(void)
{
    return 0;
}
