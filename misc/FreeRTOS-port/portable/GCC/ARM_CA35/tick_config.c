/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**************************************************************************//**
 * @file     FreeRTOS_tick_config.c
 *
 * @brief    Timer interrupt for FreeRTOS tick.
 *
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

/* Nuvoton includes. */
#include "NuMicro.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------*/
#define CONFIG_GTIM_CLOCK    (12000000)
#define CONFIG_GTIM_LOAD     (CONFIG_GTIM_CLOCK/configTICK_RATE_HZ)

void vConfigureTickInterrupt(void)
{
    extern void FreeRTOS_Tick_Handler(void);

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Disable Generic Timer and set load value */
    EL0_SetControl(0);

    EL0_SetPhysicalCompareValue(CONFIG_GTIM_LOAD + EL0_GetCurrentPhysicalValue());

    /* Install FreeRTOS_Tick_Handler as the interrupt function for */
    IRQ_SetHandler((IRQn_ID_t)NonSecPhysicalTimer_IRQn, FreeRTOS_Tick_Handler);

    /* Determine number of implemented priority bits */
    IRQ_SetPriority((IRQn_ID_t)NonSecPhysicalTimer_IRQn, IRQ_PRIORITY_Msk);

    /* Set lowest priority -1 */
    IRQ_SetPriority((IRQn_ID_t)NonSecPhysicalTimer_IRQn, GIC_GetPriority((IRQn_ID_t)NonSecPhysicalTimer_IRQn) - 1);

    /* Enable IRQ */
    IRQ_Enable((IRQn_ID_t)NonSecPhysicalTimer_IRQn);

    /* Enable timer control */
    EL0_SetControl(1U);         // non-secure timer

    vClearTickInterrupt();

    /* Lock protected registers */
    //SYS_LockReg();
}
/*-----------------------------------------------------------*/

void vClearTickInterrupt(void)
{
    EL0_SetPhysicalCompareValue(CONFIG_GTIM_LOAD + raw_read_cntp_cval_el0());

    __asm volatile("DSB SY");
    __asm volatile("ISB SY");
}
/*-----------------------------------------------------------*/

/* IRQ take over by FreeRTOS kernel */
void vApplicationIRQHandler(uint32_t ulICCIAR)
{
    /* Interrupts cannot be re-enabled until the source of the interrupt is
    cleared. The ID of the interrupt is obtained by bitwise ANDing the ICCIAR
    value with 0x3FF. */

    IRQHandler_t handler;
    IRQn_ID_t num = (int32_t)ulICCIAR;

    /* Call the function installed in the array of installed handler
    functions. */
    handler = IRQ_GetHandler(num);
    if (handler != 0)
        (*handler)();
    IRQ_EndOfInterrupt(num);
}
