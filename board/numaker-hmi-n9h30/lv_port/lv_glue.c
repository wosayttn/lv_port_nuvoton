/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for N9H30 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t) * CONFIG_LCD_FB_NUM), DEF_CACHE_LINE_SIZE)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

extern S_CALIBRATION_MATRIX g_sCalMat;

static int nuvoton_fs_init(void)
{
    return 0;
}

int nuvoton_fs_write(int hFile, void *buf, int size)
{
    return size;
}

int nuvoton_fs_read(int hFile, void *buf, int size)
{
    return size;
}

static int nuvoton_fs_fini(void)
{
    return 0;
}

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
    // Configure multi-function pin for LCD interface
    //GPG6 (CLK), GPG7 (HSYNC)
    outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL) & ~0xFF000000) | 0x22000000);
    //GPG8 (VSYNC), GPG9 (DEN)
    outpw(REG_SYS_GPG_MFPH, (inpw(REG_SYS_GPG_MFPH) & ~0xFF) | 0x22);

    //DATA pin
    /* GPA0 ~ GPA7 (DATA0~7) */
    outpw(REG_SYS_GPA_MFPL, 0x22222222);
    /* GPA8 ~ GPA15 (DATA8~15) */
    outpw(REG_SYS_GPA_MFPH, 0x22222222);
    /* GPD8 ~ GPD15 (DATA16~23) */
    outpw(REG_SYS_GPD_MFPH, 0x22222222);

#if (LV_COLOR_DEPTH==32)
    /* LCD clock is selected from UPLL and divide to 20MHz */
    outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0xE18);

    /* LCD clock is selected from UPLL and divide to 30MHz */
    //outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0x918);

    /* LCD clock is selected from UPLL and divide to 33.3MHz */
    //outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0x818);
#elif (LV_COLOR_DEPTH==16)
    /* LCD clock is selected from UPLL and divide to 10MHz */
    outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0xE19);
#else
#error "Wrong LV_COLOR_DEPTH definition. Please correct".
#endif

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

#if (CONFIG_LV_GPU_USE_N9H30_2DGE==1)
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
        psLCDInfo->u32BytePerPixel = sizeof(lv_color_t);
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

    default:
        LV_ASSERT(0);
    }

    return 0;
}

void lcd_device_close(void)
{
    vpostVAStopTrigger();
    vpostLCMDeinit();

#if (CONFIG_LV_GPU_USE_N9H30_2DGE==1)
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

    nuvoton_fs_init();

    //extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    nuvoton_fs_fini();

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

            LV_LOG_INFO("[%d, %d]", psInDevData->point.x, psInDevData->point.y);
        }
        else
        {
            psInDevData->point.x = (int16_t)adc_x;

            psInDevData->point.y = (int16_t)adc_y;
        }

        sLastInDevData.point.x  = psInDevData->point.x;
        sLastInDevData.point.y  = psInDevData->point.y;

        return 1;
    }
    else
    {
        psInDevData->point.x = sLastInDevData.point.x;
        psInDevData->point.y = sLastInDevData.point.y;
    }


    return 0;
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
