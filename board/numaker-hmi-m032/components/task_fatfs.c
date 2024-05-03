/**************************************************************************//**
 * @file     task_fatfs.c
 * @brief    FATFS for SFUD porting.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"
#include "ff.h"
#include "diskio.h"

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

static FATFS FatFs[FF_VOLUMES];       /* File system object for logical drive */
static char logic_nbr[3] = {'0', ':', 0};

int task_fatfs_init(void)
{
    DIR dir;                /* Directory object */
    FRESULT res;

    /* Mount a logical drive */
    if ((res = f_mount(&FatFs[0], logic_nbr, 1)) != 0)
    {
        put_rc(res);
#if (FF_USE_MKFS)
        {
            BYTE Buff[FF_MAX_SS];          /* Working buffer */

            MKFS_PARM opt = {0};
            opt.fmt = FM_ANY | FM_SFD;

            /* Create a FAT volume */
            res = f_mkfs(logic_nbr, &opt, Buff, sizeof(Buff));
            if (res)
            {
                put_rc(res);
                goto _Exit;
            }
            else if ((res = f_mount(&FatFs[0], logic_nbr, 1)) != 0)
            {
                goto _Exit;
            }
        }
#else
goto _Exit:
#endif
    }

    /* List directory information */
    if ((res = f_opendir(&dir, logic_nbr)) != 0)
    {
        put_rc(res);
    }
    else
    {
        FILINFO Finfo;
        FATFS *fs;              /* Pointer to file system object */
        uint32_t p1, s1, s2;
        p1 = s1 = s2 = 0;

        for (;;)
        {
            /* Read directory entries in sequence */
            res = f_readdir(&dir, &Finfo);

            if ((res != FR_OK) || !Finfo.fname[0])
                break;

            if (Finfo.fattrib & AM_DIR)
            {
                s2++;
            }
            else
            {
                s1++;
                p1 += Finfo.fsize;
            }

            printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n",
                   (Finfo.fattrib & AM_DIR) ? 'D' : '-',
                   (Finfo.fattrib & AM_RDO) ? 'R' : '-',
                   (Finfo.fattrib & AM_HID) ? 'H' : '-',
                   (Finfo.fattrib & AM_SYS) ? 'S' : '-',
                   (Finfo.fattrib & AM_ARC) ? 'A' : '-',
                   (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
                   (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, (unsigned long)Finfo.fsize, Finfo.fname);
        }

        printf("%4u File(s),%10u bytes total\n%4u Dir(s)", s1, p1, s2);

        /* Get number of free clusters */
        if (f_getfree(logic_nbr, (DWORD *)&p1, &fs) == FR_OK)
            printf(", %10u bytes free\n", p1 * fs->csize * 4096);
    }

_Exit:

    return 0;
}
