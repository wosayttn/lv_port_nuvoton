/**************************************************************************//**
 * @file     perf_ev.h
 * @brief    NuMaker Display Driver performance evaluation routines
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __PERF_EV_H__
#define __PERF_EV_H__

int InitSysTick(void);
uint64_t GetSysTickCycleCount(void);

#endif //__PERF_EV_H__
