/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for N9H20 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * LV_VER_RES_MAX * (LV_COLOR_DEPTH/8) * CONFIG_LCD_FB_NUM), DEF_CACHE_LINE_SIZE)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(DEF_CACHE_LINE_SIZE)));

extern S_CALIBRATION_MATRIX g_sCalMat;

#if defined(__480x272__)
S_CALIBRATION_MATRIX g_sCalMat = { 35449, 105, -2482568, -79, -26579, 22557014, 65536 };
#elif defined(__800x480__)
S_CALIBRATION_MATRIX g_sCalMat = { 55392, 419, -2233488, -1037, -35995, 34183424, 65536 };
#endif

#if !defined(STORAGE_SD)

#define NAND_2      1   // comment to use 1 disk foor NAND, uncomment to use 2 disk
#define NAND1_1_SIZE     32 /* MB unit */

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
    fsInitFileSystem();

#if !defined(STORAGE_SD)

    uint32_t block_size, free_size, disk_size;
    uint32_t u32TotalSize;

    fsAssignDriveNumber('C', DISK_TYPE_SMART_MEDIA, 0, 1);

#if defined(NAND_2)
    fsAssignDriveNumber('D', DISK_TYPE_SMART_MEDIA, 0, 2);
#endif

    sicOpen();

    /* Initialize GNAND */
    if (GNAND_InitNAND(&_nandDiskDriver0, &ptNDisk, TRUE) < 0)
    {
        LV_LOG_ERROR("GNAND_InitNAND error");
        goto exit_nuvoton_fs_init;
    }

    if (GNAND_MountNandDisk(&ptNDisk) < 0)
    {
        LV_LOG_ERROR("GNAND_MountNandDisk error");
        goto exit_nuvoton_fs_init;
    }

    /* Get NAND disk information*/
    u32TotalSize = ptNDisk.nZone * ptNDisk.nLBPerZone * ptNDisk.nPagePerBlock * ptNDisk.nPageSize;
    LV_LOG_INFO("Total Disk Size %d", u32TotalSize);

    /* Format NAND if necessery */
#if defined(NAND_2)
    if ((fsDiskFreeSpace('C', (UINT32 *) &block_size, (UINT32 *)&free_size, (UINT32 *)&disk_size) < 0) ||
            (fsDiskFreeSpace('D', (UINT32 *)&block_size, (UINT32 *)&free_size, (UINT32 *)&disk_size) < 0))
    {
        LV_LOG_INFO("unknow disk type, format device .....");
        if (fsTwoPartAndFormatAll((PDISK_T *)ptNDisk.pDisk, NAND1_1_SIZE * 1024, (u32TotalSize - NAND1_1_SIZE * 1024)) < 0)
        {
            LV_LOG_ERROR("Format failed");
            goto exit_nuvoton_fs_init;
        }
        fsSetVolumeLabel('C', "NAND1-1\n", strlen("NAND1-1"));
        fsSetVolumeLabel('D', "NAND1-2\n", strlen("NAND1-2"));
    }
#endif

    return 0;

exit_nuvoton_fs_init:

    return -1;

#else

    /*-----------------------------------------------------------------------*/
    /*  Init SD card                                                         */
    /*-----------------------------------------------------------------------*/
    /* clock from PLL */
    sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputKhz(eSYS_UPLL, CONFIG_EXTERN_FREQUENCY), 0, 0);
    sicOpen();
    LV_LOG_INFO("total sectors (%d)", sicSdOpen0());

    return 0;

#endif
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
#if !defined(STORAGE_SD)

    GNAND_UnMountNandDisk(&ptNDisk);
    sicClose();

#else


#endif

    return 0;
}


#if (CONFIG_LV_DISP_FULL_REFRESH==1)
static volatile uint32_t s_vu32Displayblank = 0;

#if (LV_USE_OS==LV_OS_FREERTOS)
    static xQueueHandle s_VSyncQ = NULL;
    static uint8_t dummy = 0x87;
#endif

static int lcd_vpost_handler(UINT8 *pu8, UINT32 u32)
{
    s_vu32Displayblank++;

#if (LV_USE_OS==LV_OS_FREERTOS)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(s_VSyncQ, &dummy, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
    return 0;
}
#endif


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

#if (LV_USE_OS==LV_OS_FREERTOS)
    /* Create a queue of length 1 */
    s_VSyncQ = xQueueGenericCreate(1, sizeof(uint8_t), 0);
    LV_ASSERT(s_VSyncQ != NULL);
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
    adc_init();
    return 0;
}

int touchpad_device_open(void)
{

    adc_open(ADC_TS_4WIRE, LV_HOR_RES_MAX, LV_VER_RES_MAX);

#if 1
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
    extern int ad_touch_calibrate(void);
    ad_touch_calibrate();
#endif
    return 0;
}

int touchpad_device_read(lv_indev_data_t *psInDevData)
{
    int32_t adc_x, adc_y;

    LV_ASSERT(psInDevData);

    adc_x = 0;
    adc_y = 0;
    psInDevData->state = adc_read(ADC_NONBLOCK, (uint16_t *)&adc_x, (uint16_t *)&adc_y)
                         ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

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
    adc_close();
}

int touchpad_device_finalize(void)
{
    return 0;
}
