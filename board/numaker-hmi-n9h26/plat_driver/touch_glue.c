/**************************************************************************//**
 * @file     touch_glue.c
 * @brief    NuMaker Display Driver glue code for EBI 8080 interface LCD panel
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "numaker_touch.h"
#include "touch_adc.h"
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#define CONFIG_EXTERN_FREQUENCY     sysGetExternalClock()

#if !defined(STORAGE_SD)
#define NAND_2            1  // comment to use 1 disk foor NAND, uncomment to use 2 disk
#define NAND1_1_SIZE     32  /* MB unit */

static NDISK_T ptNDisk;
static NDRV_T _nandDiskDriver0 =
{
    nandInit0,
    nandpread0,
    nandpwrite0,
    nand_is_page_dirty0,
    nand_is_valid_block0,
    nand_ioctl,
    nand_block_erase0,
    nand_chip_erase0,
    0
};
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
    sysprintf("total sectors (%d)\n", sicSdOpen0());

#endif

    return 0;
}

int nuvoton_fs_write(int hFile, void *buf, int size)
{
    int wbytes, nStatus;
    if (fsFileSeek(hFile, 0, SEEK_SET) < 0)
    {
        sysprintf("CANNOT seek the calibration into file\n");
        return -1;
    }

    nStatus = fsWriteFile(hFile, (UINT8 *)buf, size, &wbytes);
    if (nStatus < 0)
    {
        sysprintf("CANNOT write the calibration into file, %d\n", wbytes);
        return -1;
    }

    return (size == wbytes) ? wbytes : 0;
}

int nuvoton_fs_read(int hFile, void *buf, int size)
{
    int wbytes, nStatus;
    if (fsFileSeek(hFile, 0, SEEK_SET) < 0)
    {
        sysprintf("CANNOT seek the calibration into file\n");
        return -1;
    }

    nStatus = fsReadFile(hFile, (UINT8 *)buf, size, &wbytes);
    if (nStatus < 0)
    {
        sysprintf("CANNOT read the calibration into file, %d\n", wbytes);
        return -1;
    }

    return wbytes;
}

static int nuvoton_fs_fini(void)
{
    return 0;
}
#if defined(CONFIG_INDEV_TOUCH_AD)

#if defined(__320x240__)
S_CALIBRATION_MATRIX g_sCalMat = { 5873, 41, -1646858, 17, -4398, 16618786, 65536 };
#elif defined(__800x480__)
S_CALIBRATION_MATRIX g_sCalMat = { 13605, -12, -2163964, -126, -8477, 32548474, 65536 };
#endif

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
    sysprintf("file = %d\n", hFile);
    if (hFile < 0)
    {
        // file does not exists, so do calibration
        hFile = fsOpenFile(szCalibrationFile, szFileName, O_CREATE | O_RDWR | O_FSEEK);
        if (hFile < 0)
        {
            sysprintf("CANNOT create the calibration file\n");
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

int touchpad_device_read(numaker_indev_data_t *psInDevData)
{
    int32_t adc_x, adc_y;

    adc_x = 0;
    adc_y = 0;

    if (IsPenDown())
    {
        psInDevData->state = adc_read(0, (uint16_t *)&adc_x, (uint16_t *)&adc_y)
                             ? NUMAKER_INDEV_STATE_PRESSED : NUMAKER_INDEV_STATE_RELEASED;
    }
    else
    {
        psInDevData->state = NUMAKER_INDEV_STATE_RELEASED;
    }

    if (psInDevData->state == NUMAKER_INDEV_STATE_PRESSED)
    {
        extern int ad_touch_map(int32_t *sumx, int32_t *sumy);
        if (ad_touch_map((int32_t *)&adc_x, (int32_t *)&adc_y) == 0)
        {

            psInDevData->point.x = ((int16_t)adc_x < 0) ? 0 :
                                   ((int16_t)adc_x >= DISP_HOR_RES_MAX) ? (DISP_HOR_RES_MAX - 1) :
                                   adc_x;

            psInDevData->point.y = ((int16_t)adc_y < 0) ? 0 :
                                   ((int16_t)adc_y >= DISP_VER_RES_MAX) ? (DISP_VER_RES_MAX - 1) :
                                   adc_y;
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
#endif
