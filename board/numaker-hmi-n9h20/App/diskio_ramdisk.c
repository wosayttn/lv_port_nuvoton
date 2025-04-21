/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */

#define DEF_SECTOR_SIZE     4096
extern const uint8_t incbin_fatimg_start[];
extern const uint8_t incbin_fatimg_end[];

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat = 0x00;

    if (pdrv)
        stat =   STA_NOINIT;

    return stat;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    static DSTATUS stat = STA_NOINIT;

    //sysprintf("[%s] %d %08x %08x\n", __func__, incbin_fatimg_end-incbin_fatimg_start, incbin_fatimg_start, incbin_fatimg_end);
    if (stat != RES_OK)
    {
        stat = RES_OK;
    }

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    if (pdrv || (count == 0))
    {
        return RES_ERROR;
    }

    //sysprintf("[%s] %08x %08x %08x\n", __func__, buff, (void*)&incbin_fatimg_start[sector*DEF_SECTOR_SIZE], DEF_SECTOR_SIZE*count);
    memcpy((void *)buff, (void *)&incbin_fatimg_start[sector * DEF_SECTOR_SIZE], DEF_SECTOR_SIZE * count);

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    if (pdrv || (count == 0))
    {
        return RES_ERROR;
    }

    //sysprintf("[%s] %08x %08x %08x\n", __func__, (void*)&incbin_fatimg_start[sector*DEF_SECTOR_SIZE], (void*)buff, DEF_SECTOR_SIZE*count);
    memcpy((void *)&incbin_fatimg_start[sector * DEF_SECTOR_SIZE], (void *)buff, DEF_SECTOR_SIZE * count);

    return RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res = RES_OK;

    if (pdrv)
        return RES_PARERR;

    switch (cmd)
    {
    case CTRL_SYNC :        /* Make sure that no pending write process */
        break;

    case GET_SECTOR_COUNT : /* Get number of sectors on the disk (DWORD) */
        *(DWORD *)buff = (incbin_fatimg_end - incbin_fatimg_start) / DEF_SECTOR_SIZE;
        break;

    case GET_SECTOR_SIZE :  /* Get R/W sector size (WORD) */
        *(DWORD *)buff = DEF_SECTOR_SIZE;
        break;

    case GET_BLOCK_SIZE :   /* Get erase block size in unit of sector (DWORD) */
        *(DWORD *)buff = 1;
        break;

    default:
        res = RES_PARERR;
    }

    return res;
}

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */
/*---------------------------------------------------------*/
DWORD get_fattime(void)
{
    return 0x00000;
}
