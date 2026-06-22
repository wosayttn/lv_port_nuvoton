/**************************************************************************//**
 * @file        hyperflash_code.h
 * @version     V3.00
 * @brief       HyperFlash device driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef __HYPERFLASH_CODE_H__
#define __HYPERFLASH_CODE_H__

//------------------------------------------------------------------------------
// HyperFlash Operation Command
//------------------------------------------------------------------------------
//#define HFLH_START_ADDR                     (0x0)
#define HFLH_PAGE_SIZE                      (0x100)
#define HFLH_BUFF_SIZE                      (0x100)

#define HFLH_SECTOR_SIZE                    (0x40000)
#define HFLH_ERASED_VALUE                   0xFFFFFFFF

/*
 * @Note: one sector = 256K
 * @Note: S26KL512S and S26KS512S Sector Count = 256
 * @Note: S26KL256S and S26KS256S Sector Count = 128
*/
#define HFLH_MAX_SECTOR                     (256)

//------------------------------------------------------------------------------
#define HF_CMD_NOOP_CODE                    (0x00)
#define HF_CMD_COMMON_555                   (0x555)
#define HF_CMD_COMMON_AA                    (0xAA)
#define HF_CMD_COMMON_2AA                   (0x2AA)
#define HF_CMD_COMMON_55                    (0x55)

#define HF_CMD_70                           (0x70) //Read Status Register
#define HF_CMD_71                           (0x71) //Clear Status Register
#define HF_CMD_B9                           (0xB9) //Enter Power Down
#define HF_CMD_34                           (0x34) //Set Power Down Reset Timer
#define HF_CMD_3C                           (0x3C) //Read Power Down Reset Timer
#define HF_CMD_36                           (0x36) //Load Interrupt Config Register
#define HF_CMD_C4                           (0xC4) //Read Interrupt Config Register
#define HF_CMD_37                           (0x37) //Load Interrupt Status Register
#define HF_CMD_C5                           (0xC5) //Read Interrupt Status Register
#define HF_CMD_38                           (0x38) //Load Volatile Config Register
#define HF_CMD_C7                           (0xC7) //Read Volatile Config Register
#define HF_CMD_39                           (0x39) //Program Non-Volatile Config Register
#define HF_CMD_C8                           (0xC8) //Erase Non-Volatile Config Register
#define HF_CMD_C6                           (0xC6) //Read Non-Volatile Config Register
#define HF_CMD_A0                           (0xA0) //Word Program
#define HF_CMD_25                           (0x25) //Write Buffer
#define HF_CMD_29                           (0x29) //Program Buffer to Flash (Confirm)
#define HF_CMD_F0                           (0xF0) //Write To Buffer Abort Reset
#define HF_CMD_98                           (0x98) //Read ID Command
#define HF_CMD_50                           (0x50) //Clear ECC Error
#define HF_CMD_75                           (0x75) //ECC Status Enter
#define HF_CMD_90                           (0x90) //ID Entry
#define HF_CMD_FF                           (0xFF) //ASO Exit

#define HF_CMD_80                           (0x80) //Erase first commnad
#define HF_CMD_10                           (0x10) //Erase chip commnad
#define HF_CMD_30                           (0x30) //Erase sector Commnad
#define HF_CMD_33                           (0x33) //Blank check
#define HF_CMD_WORD_PROGRAM                 (0xA0)

#define HF_IDCFI_CM_ID                      (0x0000) //Cypress Manufacturer ID
#define HF_IDCFI_DEVICE_ID                  (0x0001) //Device ID
#define HF_IDCFI_DENSITY_SIZE               (0x000E) //Flash Density Size


//------------------------------------------------------------------------------
// HyperFlash Register Address
//------------------------------------------------------------------------------
#define WRITE_PWR_ON_TIME_REG               (0x34)
#define READ_PWR_ON_TIME_REG                (0x3C)
#define ERASE_NVCR_REG                      (0xC8)
#define LOAD_VCR_REG                        (0x38)
#define READ_VCR_REG                        (0xC7)
#define WRITE_NVCR_REG                      (0x39)
#define READ_NVCR_REG                       (0xC6)

//------------------------------------------------------------------------------
// HyperFlash Common Macro
//------------------------------------------------------------------------------
#define HFLH_MAX_CS_LOW                     (0xFFFFF)
#define HFLH_WR_ACCTIME                     (1)
#define HFLH_RD_ACCTIME                     (11)

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
void SPIM_HyperFlash_Init(SPIM_T *spim);

#endif  /* __HYPERFLASH_CODE_H__ */

/*** (C) COPYRIGHT 2019 Nuvoton Technology Corp. ***/
