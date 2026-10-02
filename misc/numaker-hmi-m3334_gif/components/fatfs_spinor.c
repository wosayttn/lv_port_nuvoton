/**************************************************************************//**
 * @file     fatfs_spinor.c
 * @brief    FATFS over SPI NOR Flash initialization and management
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "spi_flash.h"
#include "fatfs_spinor.h"

static FATFS s_sFatFs;
static char s_szDriveLetter[] = "A:";

static bool is_gif_file(const char *name)
{
    int len = strlen(name);
    if (len < 4) return false;
    const char *ext = &name[len - 4];
    return (ext[0] == '.') &&
           (ext[1] == 'g' || ext[1] == 'G') &&
           (ext[2] == 'i' || ext[2] == 'I') &&
           (ext[3] == 'f' || ext[3] == 'F');
}

int fatfs_spinor_scan_gifs(char file_list[][32], int max_files)
{
    DIR dir;
    FILINFO fno;
    int count = 0;

    if (f_opendir(&dir, s_szDriveLetter) != FR_OK)
        return 0;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0 && count < max_files)
    {
        if (!(fno.fattrib & AM_DIR) && is_gif_file(fno.fname))
        {
            snprintf(file_list[count], 32, "A:%s", fno.fname);
            count++;
        }
    }
    f_closedir(&dir);
    return count;
}

int fatfs_spinor_init(void)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int gif_found = 0;

    printf("\n========================================\n");
    printf("   SPI NOR Flash (FAT) Initialization   \n");
    printf("========================================\n");

    if (SpiFlash_Init() != 0)
    {
        printf("[SPI NOR FATFS] Error: SPI NOR Flash initialization failed!\n");
        return -1;
    }

    printf("[SPI NOR FATFS] Flash JEDEC ID: 0x%06X, Capacity: %u KB\n",
           SpiFlash_ReadJedecID(), SpiFlash_GetCapacity() / 1024);

    /* Try mounting logical drive */
    res = f_mount(&s_sFatFs, s_szDriveLetter, 1);
    if (res != FR_OK)
    {
        printf("[SPI NOR FATFS] f_mount failed (res = %d).\n", res);
        return -1;
    }

    /* List files in root directory and check for GIF files */
    printf("[SPI NOR FATFS] Root Directory Content:\n");
    if (f_opendir(&dir, s_szDriveLetter) == FR_OK)
    {
        while (1)
        {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
                break;

            printf("  %s  %lu bytes\n", fno.fname, (unsigned long)fno.fsize);
            if (!(fno.fattrib & AM_DIR) && is_gif_file(fno.fname))
            {
                gif_found++;
            }
        }
        f_closedir(&dir);
    }

    if (gif_found == 0)
    {
        printf("[SPI NOR FATFS] Warning: No GIF files found on drive!\n");
        return -2;
    }

    printf("[SPI NOR FATFS] Mount successful! Found %d GIF file(s). Ready for LVGL!\n", gif_found);
    return 0;
}
