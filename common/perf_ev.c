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
 * Global 64-bit CPU cycle counter for performance measurement.
 * This counter is incremented in the SysTick interrupt handler.
 */
static uint64_t s_u64CPUCycleCount = 0;

/**
 * @brief Initialize SysTick timer for performance evaluation.
 *
 * Configures the ARM SysTick timer to generate interrupts at 10ms intervals.
 * The interrupt handler increments a 64-bit cycle counter, allowing measurement
 * of long durations that would overflow a 32-bit counter.
 *
 * @return 0 on success, non-zero error code on failure
 *
 * @note Must be called once at system initialization before using GetSysTickCycleCount()
 */
int InitSysTick(void)
{
    /* Calculate number of clock cycles for 10ms interval (100 Hz) */
    const uint32_t u32Ticks10ms = SystemCoreClock / 100 + 1;
    int i32Err = 0;

    /* Reset the 64-bit cycle counter to zero */
    s_u64CPUCycleCount = 0;

    /* Disable SysTick interrupt to prevent race conditions during configuration */
    NVIC_DisableIRQ(SysTick_IRQn);

    /* Configure and initialize SysTick timer - this also enables the interrupt */
    i32Err = SysTick_Config(u32Ticks10ms);

    /* Set SysTick to use processor clock (not external clock) */
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

    /* Re-enable the SysTick interrupt after configuration */
    NVIC_EnableIRQ(SysTick_IRQn);

    /* Wait for SysTick to start (VAL should be counting down) */
    while (!i32Err && !SysTick->VAL)
    {
        __NOP();    /* No operation, just wait */
    }

    return i32Err;
}

/**
 * @brief SysTick interrupt handler for cycle counting.
 *
 * Called every 10ms to increment the 64-bit cycle counter.
 * This allows measurement of elapsed time greater than what a 32-bit
 * counter can represent.
 */
void SysTick_Handler(void)
{
    /* Add the reload value to the high 64-bit counter.
       LOAD value represents cycles between interrupts. */
    s_u64CPUCycleCount += SysTick->LOAD + 1;

    /* Data barrier and instruction barrier to ensure proper synchronization */
    __DSB();
    __ISB();
}

/**
 * @brief Get current CPU cycle count from SysTick.
 *
 * Returns the current 64-bit CPU cycle count by combining the 64-bit
 * software counter with the 24-bit SysTick counter. Handles potential
 * race conditions by checking for consistency across multiple reads.
 *
 * @return Current CPU cycle count
 *
 * @note This function implements a double-check to avoid inconsistencies
 *       from SysTick interrupt occurring between reads.
 */
uint64_t GetSysTickCycleCount(void)
{
    uint64_t high1, high2;        /* High 64-bit counter value from before and after */
    uint32_t val;                 /* Current value of 24-bit SysTick counter */

    /* Loop until we get consistent reads of the high counter before and after
       reading the SysTick VAL register. This avoids race condition if interrupt
       occurs between the two high counter reads. */
    do
    {
        /* Read high counter before reading SysTick VAL */
        high1 = s_u64CPUCycleCount;
        /* Read current SysTick counter value (counts down from LOAD to 0) */
        val = SysTick->VAL & SysTick_VAL_CURRENT_Msk;
        /* Read high counter again */
        high2 = s_u64CPUCycleCount;
    }
    while (high1 != high2);  /* Repeat if high counter changed (interrupt occurred) */

    /* Return total cycles: high counter + remaining SysTick cycles.
       (LOAD - val) gives us the number of cycles elapsed in current interval */
    return high2 + (SysTick->LOAD - val);
}
