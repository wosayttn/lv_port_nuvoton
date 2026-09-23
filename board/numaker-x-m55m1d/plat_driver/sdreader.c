/**************************************************************************//**
 * @file     SDReader.c
 * @version  V1.00
 * @brief    Read model file from SD card function
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "sdreader.h"

#define DEF_SDH_PORT SDH0

static FIL     FileObject;
static TCHAR  _Path[3];
static FATFS  _FatfsVolSd;

static void put_rc(FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0INVALID_PARAMETER\0");

    uint32_t i;

    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while (*p++) ;
    }

    printf(_T("rc=%u FR_%s\n"), (UINT)rc, p);
}

static const char* get_fname(FILINFO *fno)
{
    return fno->fname;
}

static void print_size(unsigned long size)
{
    if (size < 1024)
    {
        printf("%6lu B ", size);
    }
    else if (size < (1024UL * 1024))
    {
        printf("%6lu KB", size >> 10);
    }
    else
    {
        printf("%6lu MB", size >> 20);
    }
}

static void fatfs_ls(const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;

    unsigned long total_files = 0;
    unsigned long total_dirs  = 0;
    unsigned long total_size  = 0;

    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        printf("opendir failed (%d)\n", res);
        return;
    }

    printf("\nATTR   DATE        TIME   NAME                     SIZE\n");
    printf("-----------------------------------------------------------\n");

    while (1)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
            break;

        if (fno.fname[0] == '.')
            continue;

        const char *name = get_fname(&fno);

        /* attributes */
        printf("%c%c%c%c%c  ",
            (fno.fattrib & AM_DIR) ? 'D' : '-',
            (fno.fattrib & AM_RDO) ? 'R' : '-',
            (fno.fattrib & AM_HID) ? 'H' : '-',
            (fno.fattrib & AM_SYS) ? 'S' : '-',
            (fno.fattrib & AM_ARC) ? 'A' : '-');

        /* date */
        printf("%04u/%02u/%02u  ",
            (fno.fdate >> 9) + 1980,
            (fno.fdate >> 5) & 15,
            fno.fdate & 31);

        /* time */
        printf("%02u:%02u  ",
            (fno.ftime >> 11),
            (fno.ftime >> 5) & 63);

        /* name */
        printf("%-24s  ", name);

        /* size */
        if (fno.fattrib & AM_DIR)
        {
            printf("<DIR>\n");
            total_dirs++;
        }
        else
        {
            print_size(fno.fsize);
            printf("\n");
            total_files++;
            total_size += fno.fsize;
        }
    }

    f_closedir(&dir);

    printf("-----------------------------------------------------------\n");
    printf("%4lu File(s), ", total_files);
    print_size(total_size);
    printf("\n");
    printf("%4lu Dir(s)\n\n", total_dirs);
}


static void SDH_ISR(SDH_T *sdh)
{
    unsigned int volatile isr;
    unsigned int volatile ier;
    SDH_INFO_T *psSDInfo;

    if (sdh == SDH0)
        psSDInfo = &SD0;
    else
        psSDInfo = &SD1;

    // FMI data abort interrupt
    if (sdh->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        sdh->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = sdh->INTSTS;
    ier = sdh->INTEN;

    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        if (sdh == SDH0)
            psSDInfo->DataReadyFlag = TRUE;
        sdh->INTSTS = SDH_INTSTS_BLKDIF_Msk;
        //printf("SD block down\r\n");
    }

    if ((ier & SDH_INTEN_CDIEN_Msk) &&
            (isr & SDH_INTSTS_CDIF_Msk))    // card detect
    {
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK

            for (i = 0; i < 0x500; i++); // delay to make sure got updated value from REG_SDISR.

            isr = sdh->INTSTS;
        }

#if (DEF_CARD_DETECT_SOURCE == CardDetect_From_DAT3)

        if (!(isr & SDH_INTSTS_CDSTS_Msk))
#else
        if (isr & SDH_INTSTS_CDSTS_Msk)
#endif
        {
            printf("\n***** card remove !\n");
            psSDInfo->IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            //memset(psSDInfo, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("***** card insert !\n");
        }

        sdh->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!psSDInfo->R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }

        sdh->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        sdh->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        sdh->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }

    __DSB();
    __ISB();
}

void SDH0_IRQHandler(void)
{
    SDH_ISR(SDH0);

}
void SDH1_IRQHandler(void)
{
    SDH_ISR(SDH1);
}

//----------------------------------------------------------------------------
// Public functions
//----------------------------------------------------------------------------
BOOL
SDReader_Initialize(
    PCSTR               pszOutFileName
)
{
    FRESULT res = f_open(&FileObject, (const TCHAR *)pszOutFileName, FA_OPEN_EXISTING | FA_READ);      //USBH:0 , SD0: 1

    if (res != FR_OK)
    {
        printf("Open file error!\n");
        return FALSE;
    }

    return TRUE;
}

BOOL
SDReader_Finish(VOID)
{
    if (0 != f_close(&FileObject))
        return FALSE;

    return TRUE;
}

INT32
SDReader_ReadData(
    BYTE               *pbyData,
    INT32              i32DataSize
)
{
    FRESULT res;
    size_t  ReturnSize;

    if (f_eof(&FileObject))
        return -1;

    res = f_read(&FileObject, pbyData, i32DataSize, &ReturnSize);
    if (res != FR_OK)
    {
        return -2;
    }

    return ReturnSize;
}

INT32
SDReader_FileSize(VOID)
{
    return f_size(&FileObject);
}

VOID
SDReader_Rewind(VOID)
{
    f_rewind(&FileObject);
}

int32_t SDH_Open_Disk(SDH_T *sdh, uint32_t u32CardDetSrc)
{
    static int bInit = 0;
    FRESULT res;

    if (bInit) return SDH_OK;

    SDH_Open(sdh, u32CardDetSrc);

    if (SDH_Probe(sdh))
    {
        printf("SD initial fail!!\n");
        return SDH_ERR_FAIL;
    }

    _Path[0] = 'A';
    _Path[1] = ':';
    _Path[2] = 0;

    if ((res = f_mount(&_FatfsVolSd, _Path, 1)) != 0)
    {
        put_rc(res);
        return SDH_ERR_FAIL;
    }
		fatfs_ls(_Path);

    bInit = 1;
    return SDH_OK;
}

void SDH_Close_Disk(SDH_T *sdh)
{
		memset(&SD0, 0, sizeof(SDH_INFO_T));
		f_mount(NULL, _Path, 1);
		memset(&_FatfsVolSd, 0, sizeof(FATFS));
}

DWORD get_fattime(void)
{
    unsigned long g_u64Tmr;

    g_u64Tmr = 0x00000;

    return g_u64Tmr;
}

INT32 SDReader_LoadFile(const char *pcFilePath, void *pvDstAddr)
{
    INT32 i32DataSize = 0;

    if (SDH_Open_Disk(DEF_SDH_PORT, CardDetect_From_GPIO) != SDH_OK)
    {
        printf("Failed to OpenDisk\n");
        goto Fail_LoadFileFromSDCard;
    }
    else if (SDReader_Initialize((PCSTR)pcFilePath) != TRUE)
    {
        printf("Failed to SDReader_Initialize\n");
        goto Fail_LoadFileFromSDCard;
    }
    else if ((i32DataSize = SDReader_FileSize()) < 0)
    {
        printf("Failed to SDReader_FileSize\n");
        goto Fail_LoadFileFromSDCard;
    }
    else if ((SDReader_ReadData((BYTE *)pvDstAddr, i32DataSize) != i32DataSize))
    {
        printf("Failed to SDReader_ReadData\n");
        goto Fail_LoadFileFromSDCard;
    }
    else if (SDReader_Finish() != TRUE)
    {
        printf("Failed to SDReader_Finish\n");
        goto Fail_LoadFileFromSDCard;
    }

    printf("SDReader: Load %s to %08x OK!! (%dB)\n", pcFilePath, (uint32_t)pvDstAddr, i32DataSize);
		
    return i32DataSize;

Fail_LoadFileFromSDCard:

    SDReader_Finish();

    SDH_Close_Disk(DEF_SDH_PORT);

    return -1;
}

