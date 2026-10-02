/**************************************************************************//**
 * @file     diskio.c
 * @brief    FatFs Disk I/O module for SPI NOR Flash
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "ff.h"
#include "diskio.h"
#include "spi_flash.h"

#define DEV_SPI_FLASH    0

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != DEV_SPI_FLASH)
        return STA_NOINIT;

    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != DEV_SPI_FLASH)
        return STA_NOINIT;

    if (SpiFlash_Init() != 0)
        return STA_NOINIT;

    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_SPI_FLASH || count == 0)
        return RES_PARERR;

    SpiFlash_QPI_FastRead(sector * SPI_FLASH_SECTOR_SIZE, buff, count * SPI_FLASH_SECTOR_SIZE);

    return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    UINT i;

    if (pdrv != DEV_SPI_FLASH || count == 0)
        return RES_PARERR;

    for (i = 0; i < count; i++)
    {
        SpiFlash_WriteSector((sector + i) * SPI_FLASH_SECTOR_SIZE,
                             buff + i * SPI_FLASH_SECTOR_SIZE);
    }

    return RES_OK;
}

#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != DEV_SPI_FLASH)
        return RES_PARERR;

    switch (cmd)
    {
    case CTRL_SYNC:
        SpiFlash_WaitReady();
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD *)buff = SPI_FLASH_SECTOR_SIZE;
        return RES_OK;

    case GET_SECTOR_COUNT:
        *(DWORD *)buff = SpiFlash_GetCapacity() / SPI_FLASH_SECTOR_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1; /* 1 sector per erase block */
        return RES_OK;

    default:
        return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    /* 2026-10-01 12:00:00 */
    return ((DWORD)(2026 - 1980) << 25) |
           ((DWORD)10 << 21) |
           ((DWORD)1 << 16) |
           ((DWORD)12 << 11) |
           ((DWORD)0 << 5) |
           ((DWORD)0 >> 1);
}
