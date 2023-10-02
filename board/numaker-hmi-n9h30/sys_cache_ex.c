/**************************************************************************//**
 * @file     sys_cache_ex.c
 * @brief    Extend system cache operations.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "stdint.h"
#include "sys_cache_ex.h"

/*----- Keil -----------------------------------------------------------------*/
#ifdef __CC_ARM

void sys_cache_clean_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm volatile { MCR p15, 0, ptr, c7, c10, 1 }
        ptr += DEF_CACHE_LINE_SIZE;
    }
}

void sys_cache_invalidate_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm volatile { MCR p15, 0, ptr, c7, c6, 1 }
        ptr += DEF_CACHE_LINE_SIZE;
    }
}

void sys_cache_invalidate_dcache_all()
{
    register uint32_t value;

    value = 0;

    __asm volatile { mcr p15, 0, value, c7, c6, 0 }
}

void sys_cache_clean_invalidated_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm volatile { MCR p15, 0, ptr, c7, c14, 1 }
        ptr += DEF_CACHE_LINE_SIZE;
    }
}

#elif defined(__GNUC__)

void sys_cache_clean_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm volatile("mcr p15, 0, %0, c7, c10, 1": :"r"(ptr));

        ptr += DEF_CACHE_LINE_SIZE;
    }
}

void sys_cache_invalidate_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm volatile("mcr p15, 0, %0, c7, c6, 1": :"r"(ptr));

        ptr += DEF_CACHE_LINE_SIZE;
    }
}

void sys_cache_invalidate_dcache_all()
{
    asm volatile("mcr p15, 0, %0, c7, c6, 0": :"r"(0));
}

void sys_cache_clean_invalidated_dcache(uint32_t buffer, uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(DEF_CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm volatile("mcr p15, 0, %0, c7, c14, 1": :"r"(ptr));

        ptr += DEF_CACHE_LINE_SIZE;
    }
}

#endif
