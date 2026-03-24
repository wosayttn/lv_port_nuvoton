/**************************************************************************//**
 * @file        norflash_code.h
 * @version     V1.00
 * @brief       NorFlash device driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef __NOR_FLASH_CODE_H__
#define __NOR_FLASH_CODE_H__

#include "NuMicro.h"

void SPIM_NorFlash_Init(SPIM_T *spim);

/* Program Command Phase */
extern SPIM_PHASE_T gsWb02hWrCMD;
extern SPIM_PHASE_T gsWb12hWrCMD;

/* Standard Read Command Phase */
extern SPIM_PHASE_T gsWb0BhRdCMD;

/* Dual Read Command Phase */
extern SPIM_PHASE_T gsWbBBhRdCMD;
extern SPIM_PHASE_T gsWbBChRdCMD;

/* Quad Read Command Phase */
extern SPIM_PHASE_T gsWbEBhRdCMD;
extern SPIM_PHASE_T gsWbEChRdCMD;

#endif  /* __NOR_FLASH_CODE_H__ */

/*** (C) COPYRIGHT 2023 Nuvoton Technology Corp. ***/
