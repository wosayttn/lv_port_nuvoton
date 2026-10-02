/**************************************************************************//**
 * @file     fatfs_spinor.h
 * @brief    FATFS over SPI NOR Flash initialization and management
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __FATFS_SPINOR_H__
#define __FATFS_SPINOR_H__

#include <stdint.h>
#include <stdbool.h>

int fatfs_spinor_init(void);
int fatfs_spinor_scan_gifs(char file_list[][32], int max_files);

#endif /* __FATFS_SPINOR_H__ */
