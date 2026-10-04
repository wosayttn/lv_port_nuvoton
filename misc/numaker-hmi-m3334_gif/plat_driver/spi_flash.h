/**************************************************************************//**
 * @file     spi_flash.h
 * @brief    QSPI0 NOR Flash Driver for NuMaker-M3334 (PC0~PC5)
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include <stdint.h>
#include <stdbool.h>

#define SPI_FLASH_PORT          QSPI0
#define SPI_FLASH_SECTOR_SIZE   4096
#define SPI_FLASH_PAGE_SIZE     256

int      SpiFlash_Init(void);
uint32_t SpiFlash_ReadJedecID(void);
uint32_t SpiFlash_GetCapacity(void);
uint8_t  SpiFlash_ReadStatusReg(void);
uint8_t  SpiFlash_ReadStatusReg2(void);
void     SpiFlash_WriteStatusReg(uint8_t u8Value1, uint8_t u8Value2);
void     SpiFlash_EnableQE(void);
void     SpiFlash_Unprotect(void);
void     SpiFlash_Read(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len);
void     SpiFlash_QPI_FastRead(uint32_t u32Addr, uint8_t *pu8Buf, uint32_t u32Len);
void     SpiFlash_WriteSector(uint32_t u32SectorAddr, const uint8_t *pu8Buf);
void     SpiFlash_SectorErase(uint32_t u32SectorAddr);
void     SpiFlash_PageProgram(uint32_t u32Addr, const uint8_t *pu8Buf, uint32_t u32Len);
int32_t  SpiFlash_WaitReady(void);

#endif /* __SPI_FLASH_H__ */
