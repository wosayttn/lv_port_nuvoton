/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for N9H26 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * (LV_COLOR_DEPTH/8) * CONFIG_LCD_FB_NUM), DEF_CACHE_LINE_SIZE)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

extern S_CALIBRATION_MATRIX g_sCalMat;

#if defined(__320x240__)
S_CALIBRATION_MATRIX g_sCalMat = { 5873, 41, -1646858, 17, -4398, 16618786, 65536 };
#elif defined(__800x480__)
S_CALIBRATION_MATRIX g_sCalMat = { 13605, -12, -2163964, -126, -8477, 32548474, 65536 };
#endif

static int nuvoton_fs_init(void)
{
#if defined(STORAGE_SD)

    fsInitFileSystem();

    /*-----------------------------------------------------------------------*/
    /*  Init SD card                                                         */
    /*-----------------------------------------------------------------------*/
    /* clock from PLL */
    sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, CONFIG_EXTERN_FREQUENCY), 0, 0);
    sicOpen();
    LV_LOG_INFO("total sectors (%d)", sicSdOpen0());

#endif

    return 0;
}

int nuvoton_fs_write(int hFile, void *buf, int size)
{
    int wbytes, nStatus;
    if (fsFileSeek(hFile, 0, SEEK_SET) < 0)
    {
        LV_LOG_ERROR("CANNOT seek the calibration into file");
        return -1;
    }

    nStatus = fsWriteFile(hFile, (UINT8 *)buf, size, &wbytes);
    if (nStatus < 0)
    {
        LV_LOG_ERROR("CANNOT write the calibration into file, %d", wbytes);
        return -1;
    }

    return (size == wbytes) ? wbytes : 0;
}

int nuvoton_fs_read(int hFile, void *buf, int size)
{
    int wbytes, nStatus;
    if (fsFileSeek(hFile, 0, SEEK_SET) < 0)
    {
        LV_LOG_ERROR("CANNOT seek the calibration into file");
        return -1;
    }

    nStatus = fsReadFile(hFile, (UINT8 *)buf, size, &wbytes);
    if (nStatus < 0)
    {
        LV_LOG_ERROR("CANNOT read the calibration into file, %d", wbytes);
        return -1;
    }

    return wbytes;
}

static int nuvoton_fs_fini(void)
{
    return 0;
}

static volatile uint32_t s_vu32Displayblank = 0;

int lcd_vpost_handler(UINT8 *pu8, UINT32 u32)
{
    s_vu32Displayblank++;

    return 0;
}

int lcd_device_initialize(void)
{
    EDMA_Init();

    return 0;
}

int lcd_device_open(void)
{
    LCDFORMATEX lcdFormat = {0};

    /* Specify pixel format. */
#if (LV_COLOR_DEPTH==16)
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGB565;
#elif (LV_COLOR_DEPTH==32)
    lcdFormat.ucVASrcFormat = DRVVPOST_FRAME_RGBx888;
#else
#error "Wrong LV_COLOR_DEPTH definition. Please correct".
#endif

    LV_ASSERT(vpostLCMInit(&lcdFormat, (UINT32 *)s_au8FrameBuf) == 0);

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
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
        vpostSetFrameBuffer((UINT32)argv);
    }
    break;

    case evLCD_CTRL_WAIT_VSYNC:
    {
        volatile uint32_t next = s_vu32Displayblank + 1;
        while (s_vu32Displayblank <  next)
        {
            //Wait next blank coming;
        }
    }
    break;

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
    vpostLCMDeinit();

#if (CONFIG_LV_DISP_FULL_REFRESH==1)
    sysDisableInterrupt(IRQ_VPOST);
    vpostDisableInt(eDRVVPOST_VINT);
#endif
}

int lcd_device_finalize(void)
{
    EDMA_Exit();

    return 0;
}

int touchpad_device_initialize(void)
{
    return 0;
}

int touchpad_device_open(void)
{

    DrvADC_Open();

#if 0
    char szFileName[32];
    char szCalibrationFile[32];
    int hFile;

    nuvoton_fs_init();
    sprintf(szFileName, "C:\\ts_calib");
    fsAsciiToUnicode(szFileName, szCalibrationFile, TRUE);
    hFile = fsOpenFile(szCalibrationFile, szFileName, O_RDONLY | O_FSEEK);
    LV_LOG_INFO("file = %d", hFile);
    if (hFile < 0)
    {
        // file does not exists, so do calibration
        hFile = fsOpenFile(szCalibrationFile, szFileName, O_CREATE | O_RDWR | O_FSEEK);
        if (hFile < 0)
        {
            LV_LOG_ERROR("CANNOT create the calibration file");
            return -1;
        }
        extern int ad_touch_calibrate(void);
        ad_touch_calibrate();
        nuvoton_fs_write(hFile, &g_sCalMat, sizeof(S_CALIBRATION_MATRIX));
    }
    else
    {
        nuvoton_fs_read(hFile, &g_sCalMat, sizeof(S_CALIBRATION_MATRIX));
    }
    fsCloseFile(hFile);

    nuvoton_fs_fini();
#else
    //extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();
#endif

    return 0;
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
    int32_t adc_x, adc_y;

    LV_ASSERT(psInDevData);

    adc_x = 0;
    adc_y = 0;

    if (IsPenDown())
    {
        psInDevData->state = adc_read(0, (uint16_t *)&adc_x, (uint16_t *)&adc_y)
                             ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }
    else
    {
        psInDevData->state = LV_INDEV_STATE_RELEASED;
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

        return 1;
    }

    return 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    DrvADC_Close();
}

int touchpad_device_finalize(void)
{
    return 0;
}
