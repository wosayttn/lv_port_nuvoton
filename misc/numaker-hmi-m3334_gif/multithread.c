/**************************************************************************//**
 * @file     multithread.c
 * @brief    Arm C library multithreading lock interface for FreeRTOS
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * Arm C Library multithreading interface.
 *
 * The Arm C library provides hooks for thread-safety (_mutex_initialize,
 * _mutex_acquire, _mutex_release, _mutex_free) which protect heap allocation
 * (malloc, free, realloc), stdio streams, etc.
 *
 * Suspending task scheduling via vTaskSuspendAll() ensures mutual exclusion
 * across all tasks without recursion or deadlock hazards, and avoids
 * allocating dynamic resources for mutex handles.
 */

static inline uint32_t is_thread_mode(void)
{
    return (__get_IPSR() == 0U);
}

__attribute__((used)) int _mutex_initialize(void *mutex)
{
    (void)mutex;
    return 1;
}

__attribute__((used)) void _mutex_acquire(void *mutex)
{
    (void)mutex;
    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) && is_thread_mode())
    {
        vTaskSuspendAll();
    }
}

__attribute__((used)) void _mutex_release(void *mutex)
{
    (void)mutex;
    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) && is_thread_mode())
    {
        (void)xTaskResumeAll();
    }
}

__attribute__((used)) void _mutex_free(void *mutex)
{
    (void)mutex;
}
