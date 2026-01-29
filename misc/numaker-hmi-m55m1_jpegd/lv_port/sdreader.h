/**************************************************************************//**
 * @file     SDReader.h
 * @version  V1.00
 * @brief    Read model file from SD card function
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __SDREADER_H__
#define __SDREADER_H__

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"         /* FatFs lower layer API */
#include "diskio.h"     /* FatFs lower layer API */
#include "NuMicro.h"

//-----------------------------------------------------------------------------
// Type declaration
//-----------------------------------------------------------------------------
typedef signed int          BOOL;
typedef unsigned char       BYTE;
typedef signed int          INT32;
typedef const char         *PCSTR;
typedef unsigned char      *PUINT8;
typedef unsigned short     *PUINT16;
typedef unsigned int       *PUINT32;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef void                VOID;

// Read file functions
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
SDReader_Initialize(
    PCSTR               pszOutFileName
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
INT32
SDReader_ReadData(
    BYTE               *pbyData,
    INT32              i32DataSize
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
SDReader_Finish(VOID);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
INT32
SDReader_FileSize(VOID);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID
SDReader_Rewind(VOID);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
INT32
SDReader_LoadFile(const char *pcFilePath, void *pvDstAddr);

#ifdef  __cplusplus
}
#endif

#endif
