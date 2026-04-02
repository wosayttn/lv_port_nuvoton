/**************************************************************************//**
 * @file     nu_misc.c
 * @brief    Miscellaneous memory and timing utility functions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdint.h>
#include "nu_misc.h"
#include <math.h>

/* Include FreeRTOS memory allocation if available, otherwise use standard malloc */
#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
#else
    #include <stdlib.h>
#endif

/**
 * @brief Allocate aligned memory block.
 *
 * This function allocates a memory block with the specified alignment.
 * It works with both FreeRTOS (pvPortMalloc) and standard C library (malloc).
 * The original pointer is stored right before the aligned pointer for later deallocation.
 *
 * @param size[in]   Size of memory to allocate in bytes
 * @param align[in]  Alignment requirement in bytes (must be power of 2)
 *
 * @return Pointer to aligned memory block, or NULL if allocation fails
 *
 * @note The caller must use nvt_free_align() to free the memory allocated by this function
 */
void *nvt_malloc_align(uint32_t size, uint32_t align)
{
    void *ptr;                    /* Pointer to the allocated block */
    uint32_t align_size;          /* Total size needed including alignment overhead */

    /* Ensure alignment is at least the size of a pointer */
    align = NVT_ALIGN(align, sizeof(void *));

    /* Calculate total size: aligned size + alignment padding */
    align_size = NVT_ALIGN(size, sizeof(void *)) + align;

    /* Allocate memory using FreeRTOS or standard malloc */
#if defined(__FREERTOS__)
    if ((ptr = pvPortMalloc(align_size)) != NULL)
#else
    if ((ptr = malloc(align_size)) != NULL)
#endif
    {
        void *align_ptr;          /* Pointer to the aligned address */

        /* Check if allocated memory is already aligned */
        if (((uintptr_t)ptr & (align - 1)) == 0)
        {
            /* Already aligned, add offset to store original pointer */
            align_ptr = (void *)((uintptr_t)ptr + align);
        }
        else
        {
            /* Not aligned, align the pointer */
            align_ptr = (void *)NVT_ALIGN((uintptr_t)ptr, align);
        }

        /* Store the original pointer right before the aligned pointer for later freeing */
        *((uint32_t *)((uintptr_t)align_ptr - sizeof(void *))) = (uintptr_t)ptr;

        /* Return the aligned pointer */
        ptr = align_ptr;
    }

    return ptr;
}

/**
 * @brief Free aligned memory block.
 *
 * This function frees memory that was allocated by nvt_malloc_align().
 * It retrieves the original pointer from storage and uses the appropriate
 * free function (vPortFree for FreeRTOS, free for standard C).
 *
 * @param ptr[in]  Pointer to aligned memory block (as returned by nvt_malloc_align)
 *
 * @note This function will do nothing if ptr is NULL
 */
void nvt_free_align(void *ptr)
{
    if (ptr == NULL) return;     /* Return early if pointer is NULL */

    /* Free using FreeRTOS or standard free based on compilation flags */
#if defined(__FREERTOS__)
    /* Retrieve and free the original pointer using vPortFree */
    vPortFree((void *) * ((uintptr_t *)((uintptr_t)ptr - sizeof(void *))));
#else
    /* Retrieve and free the original pointer using standard free */
    free((void *) * ((uintptr_t *)((uintptr_t)ptr - sizeof(void *))));
#endif
}

/**
 * @brief Convert nanoseconds to CPU cycles (ceiling).
 *
 * Converts a time duration in nanoseconds to the equivalent number of CPU cycles
 * at a given clock frequency, rounding up to the nearest integer.
 *
 * @param ns[in]      Time duration in nanoseconds
 * @param clk_hz[in]  CPU clock frequency in Hz
 *
 * @return Number of CPU cycles (rounded up)
 *
 * @note Returns 0 if ns or clk_hz is <= 0
 */
uint32_t ns_to_cycles_ceil(double ns, double clk_hz)
{
    /* Validate input parameters */
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    /* Calculate cycles: ceil(ns * clk_hz / 1e9) */
    double cycles = (ns * clk_hz) / 1e9;
    return (uint32_t)ceil(cycles);
}

/**
 * @brief Convert frequency in Hz to time period in nanoseconds (ceiling).
 *
 * Converts a frequency in Hz to the equivalent time period in nanoseconds,
 * rounding up to the nearest integer.
 *
 * @param hz[in]  Frequency in Hz
 *
 * @return Time period in nanoseconds (rounded up)
 *
 * @note Returns 0 if hz is <= 0
 */
uint32_t hz_to_ns_ceil(double hz)
{
    /* Validate input parameter */
    if (hz <= 0.0)
        return 0;

    /* Calculate nanoseconds: ceil(1e9 / hz) */
    double ns = 1e9 / hz;
    return (uint32_t)ceil(ns);
}

/**
 * @brief Convert nanoseconds to CPU cycles (floating point).
 *
 * Converts a time duration in nanoseconds to the equivalent number of CPU cycles
 * at a given clock frequency, returning a floating-point result.
 *
 * @param ns[in]      Time duration in nanoseconds
 * @param clk_hz[in]  CPU clock frequency in Hz
 *
 * @return Number of CPU cycles as a floating-point value
 *
 * @note Returns 0 if ns or clk_hz is <= 0
 */
double ns_to_cycles(double ns, double clk_hz)
{
    /* Validate input parameters */
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    /* Calculate cycles: ns * clk_hz / 1e9 */
    return (ns * clk_hz) / 1e9;
}

/**
 * @brief Convert frequency in Hz to time period in nanoseconds (floating point).
 *
 * Converts a frequency in Hz to the equivalent time period in nanoseconds,
 * returning a floating-point result.
 *
 * @param hz[in]  Frequency in Hz
 *
 * @return Time period in nanoseconds as a floating-point value
 *
 * @note Returns 0 if hz is <= 0
 */
double hz_to_ns(double hz)
{
    /* Validate input parameter */
    if (hz <= 0.0)
        return 0;

    /* Calculate nanoseconds: 1e9 / hz */
    return 1e9 / hz;
}

