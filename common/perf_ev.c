/**************************************************************************//**
 * @file     perf_ev.c
 * @brief    NuMaker Display Driver performance evaluation routines
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"
#include <stdint.h>

/**
 * SysTick initialisation
 */
static uint64_t s_u64CPUCycleCount = 0;    /* 64-bit cpu cycle counter */
int InitSysTick(void)
{
    const uint32_t u32Ticks10ms = SystemCoreClock / 100 + 1;
    int i32Err = 0;

    /* Reset CPU cycle count value. */
    s_u64CPUCycleCount = 0;
    /* Changing configuration for sys tick => guard from being
     * interrupted. */
    NVIC_DisableIRQ(SysTick_IRQn);
    /* SysTick init - this will enable interrupt too. */
    i32Err = SysTick_Config(u32Ticks10ms);
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

    /* Enable interrupt again. */
    NVIC_EnableIRQ(SysTick_IRQn);

    /* Wait for SysTick to kick off */
    while (!i32Err && !SysTick->VAL)
    {
        __NOP();
    }

    return i32Err;
}

/**
 * SysTick IRQ handler
 */
void SysTick_Handler(void)
{
    /* Increment the cycle counter based on load value. */
    s_u64CPUCycleCount += SysTick->LOAD + 1;
    __DSB();
    __ISB();
}

/**
 * Gets the current SysTick derived counter value
 */
uint64_t GetSysTickCycleCount(void)
{
    uint64_t high1, high2;
    uint32_t val;

    do
    {
        high1 = s_u64CPUCycleCount;
        val = SysTick->VAL & SysTick_VAL_CURRENT_Msk;
        high2 = s_u64CPUCycleCount;
    }
    while (high1 != high2);

    return high2 + (SysTick->LOAD - val);
}
