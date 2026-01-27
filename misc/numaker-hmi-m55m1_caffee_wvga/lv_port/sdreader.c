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
static FATFS  _FatfsVolSd0;
static FATFS  _FatfsVolSd1;

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

    if (bInit) return SDH_OK;

    SDH_Open(sdh, u32CardDetSrc);

    if (SDH_Probe(sdh))
    {
        printf("SD initial fail!!\n");
        return SDH_ERR_FAIL;
    }

    _Path[1] = ':';
    _Path[2] = 0;

    if (sdh == SDH0)
    {
        _Path[0] = '0';
        f_mount(&_FatfsVolSd0, _Path, 1);
    }
    else
    {
        _Path[0] = '1';
        f_mount(&_FatfsVolSd1, _Path, 1);
    }

    bInit = 1;
    return SDH_OK;
}

void SDH_Close_Disk(SDH_T *sdh)
{
    if (sdh == SDH0)
    {
        memset(&SD0, 0, sizeof(SDH_INFO_T));
        f_mount(NULL, _Path, 1);
        memset(&_FatfsVolSd0, 0, sizeof(FATFS));
    }
    else
    {
        memset(&SD1, 0, sizeof(SDH_INFO_T));
        f_mount(NULL, _Path, 1);
        memset(&_FatfsVolSd1, 0, sizeof(FATFS));
    }
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

