/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include "sfud.h"
#include "sfud_cfg.h"

#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */

#define DEF_SECTOR_SIZE     512

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

    if (stat != RES_OK)
    {
        //sfud_flash *flash = sfud_get_device(SFUD_W25_DEVICE_INDEX);
        if (sfud_init() == SFUD_SUCCESS)
            stat = RES_OK;
        /* Enable qspi fast read mode, set four data lines width */
        //sfud_qspi_fast_read_enable(flash, 4);
    }

    return stat;
}

typedef enum
{
    SFUD_READ,
    SFUD_ERASE_WRITE,
    SFUD_WRITE,
} E_MODE;

static sfud_err sfud_transfer(E_MODE mode, uint8_t *pu8Buf, uint32_t sector, uint32_t count)
{
    sfud_err result;
    sfud_flash *flash = sfud_get_device(SFUD_W25_DEVICE_INDEX);
    uint32_t u32SecSize = flash->chip.erase_gran;
    uint32_t u32Addr, u32Size;

    u32Addr = sector * u32SecSize;
    u32Size = count * u32SecSize;

    // printf("%s Mode: %d, StartSecAddr: %d, Size: %d\n", __func__, mode, u32Addr, u32Size);
    if (mode == SFUD_READ)
    {
        result = sfud_read(flash, u32Addr, u32Size, (uint8_t *)pu8Buf);
    }
    else if (mode == SFUD_ERASE_WRITE)
    {
        result = sfud_erase_write(flash, u32Addr, u32Size, (uint8_t *)pu8Buf);
    }
    else if (mode == SFUD_WRITE)
    {
        result = sfud_write(flash, u32Addr, u32Size, (uint8_t *)pu8Buf);
    }

    return result;
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
    if (pdrv || (count == 0 || count >= 2))
    {
        return RES_ERROR;
    }

    return (sfud_transfer(SFUD_READ, buff, sector, count) == SFUD_SUCCESS) ? RES_OK : RES_ERROR;
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
    if (pdrv || (count == 0 || count >= 2))
    {
        return RES_ERROR;
    }

    return (sfud_transfer(SFUD_ERASE_WRITE, (BYTE *)buff, sector, count) == SFUD_SUCCESS) ? RES_OK : RES_ERROR;
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
        *(DWORD *)buff = sfud_get_device(SFUD_W25_DEVICE_INDEX)->chip.capacity / sfud_get_device(SFUD_W25_DEVICE_INDEX)->chip.erase_gran;
        break;

    case GET_SECTOR_SIZE :  /* Get R/W sector size (WORD) */
        *(DWORD *)buff = sfud_get_device(SFUD_W25_DEVICE_INDEX)->chip.erase_gran;
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

