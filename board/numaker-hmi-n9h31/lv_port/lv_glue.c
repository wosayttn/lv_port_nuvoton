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
S_CALIBRATION_MATRIX g_sCalMat = { 13292, 56, -1552344, -79, 8401, -1522648, 65536  };
#endif


#if (CONFIG_LV_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;
static void lcd_vpost_handler(void)
{
    /* clear VPOST interrupt state */
    uint32_t uintstatus = inpw(REG_LCM_INT_CS);

    if (uintstatus & VPOSTB_DISP_F_INT)
    {
        outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_DISP_F_INT);
        s_vu32Displayblank++;
    }
    else if (uintstatus & VPOSTB_BUS_ERROR_INT)
    {
        outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_BUS_ERROR_INT);
    }
}
#endif

int lcd_device_initialize(void)
{

#if defined(__800x480__)
    vpostLCMInit(DIS_PANEL_FW070TFT);
#elif defined(__480x272__)
    vpostLCMInit(DIS_PANEL_FW043TFT);
#else
#error "Unsupported resolution definition. Please correct".
#endif

    // Set scale to 1:1
    vpostVAScalingCtrl(1, 0, 1, 0, VA_SCALE_INTERPOLATION);

#if (LV_COLOR_DEPTH==32)
    vpostSetVASrc(VA_SRC_RGB888);
#elif (LV_COLOR_DEPTH==16)
    vpostSetVASrc(VA_SRC_RGB565);
#endif

    vpostSetFrameBuffer(s_au8FrameBuf);

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
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

    /* GDMA engine clock */
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x00001000);

#if (CONFIG_LV_GPU_USE_N9H31_2DGE==1)
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
        while (s_vu32Displayblank <  next)
        {
            //Wait next blank coming;
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

#if (CONFIG_LV_GPU_USE_N9H31_2DGE==1)
#endif

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
