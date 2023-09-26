/**************************************************************************//**
 * @file     sys_cache_ex.c
 * @brief    Extend system cache operations.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef __SYS_CACHE_EX_H__
#define __SYS_CACHE_EX_H__

#define DEF_CACHE_LINE_SIZE     32

void sys_cache_clean_invalidated_dcache(uint32_t buffer, uint32_t size);

void sys_cache_clean_dcache(uint32_t buffer, uint32_t size);

void sys_cache_invalidate_dcache(uint32_t buffer, uint32_t size);

void sys_cache_invalidate_dcache_all(void);

#endif
