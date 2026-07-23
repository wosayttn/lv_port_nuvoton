/**************************************************************************//**
* @file     FreeRTOS_startup.s
* @brief    System Startup for MA35D1 Device Series with FreeRTOS
*
* SPDX-License-Identifier: Apache-2.0
* @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

.extern  SynchronousInterrupt
.extern  IRQInterrupt
.extern  FIQInterrupt
.extern  SErrorInterrupt
.extern  SystemInit1
.extern  SystemInit0

.global __el3_stack
.global __el0_stack
.global __el3_stack_s
.global __el0_stack_s

.set EL3_stack,		__el3_stack
.set EL0_stack,		__el0_stack
.set EL3_stack_s,	__el3_stack_s
.set EL0_stack_s,	__el0_stack_s

.globl start64
.globl _vector_table
.globl _freertos_vector_table

.globl SynchronousInterruptHandler
.globl IRQInterruptHandler
.globl FIQInterruptHandler
.globl SErrorInterruptHandler

.macro saveregister
	stp	X0,X1, [sp,#-0x10]!
	stp	X2,X3, [sp,#-0x10]!
	stp	X4,X5, [sp,#-0x10]!
	stp	X6,X7, [sp,#-0x10]!
	stp	X8,X9, [sp,#-0x10]!
	stp	X10,X11, [sp,#-0x10]!
	stp	X12,X13, [sp,#-0x10]!
	stp	X14,X15, [sp,#-0x10]!
	stp	X16,X17, [sp,#-0x10]!
	stp X18,X19, [sp,#-0x10]!
	stp X29,X30, [sp,#-0x10]!
.endm

.macro restoreregister
	ldp	X29,X30, [sp], #0x10
	ldp	X18,X19, [sp], #0x10
	ldp	X16,X17, [sp], #0x10
	ldp	X14,X15, [sp], #0x10
	ldp	X12,X13, [sp], #0x10
	ldp	X10,X11, [sp], #0x10
	ldp	X8,X9, [sp], #0x10
	ldp	X6,X7, [sp], #0x10
	ldp	X4,X5, [sp], #0x10
	ldp	X2,X3, [sp], #0x10
	ldp	X0,X1, [sp], #0x10
.endm


/******************************************************************************
 * Vector table to use when boot.
 *****************************************************************************/
.org 0
.section ".text.boot"

_vector_table:
.set	VBAR, _vector_table

.org VBAR
	b	start64

.org (VBAR + 0x80)
	b	IRQInterruptHandler /* make INT workable before FreeRTOS starts */

.org (VBAR + 0x100)
	b	.

.org (VBAR + 0x180)
	b	.

.org (VBAR + 0x200)
	b	.

.org (VBAR + 0x280)
	b	.

.org (VBAR + 0x300)
	b	.

.org (VBAR + 0x380)
	b	.

.org (VBAR + 0x400)
	b	.

.org (VBAR + 0x480)
	b	.

.org (VBAR + 0x500)
	b	.

.org (VBAR + 0x580)
	b	.

.org (VBAR + 0x600)
	b	.

.org (VBAR + 0x680)
	b	.

.org (VBAR + 0x700)
	b	.

.org (VBAR + 0x780)
	b	.

/******************************************************************************
 * Vector table to use when FreeRTOS is running.
 *****************************************************************************/
.set	FREERTOS_VBAR, (VBAR+0x1000)

.org(FREERTOS_VBAR)
_freertos_vector_table:
	b	FreeRTOS_SWI_Handler

.org (FREERTOS_VBAR + 0x80)
	b	FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x100)
	b	.

.org (FREERTOS_VBAR + 0x180)
	b	.

.org (FREERTOS_VBAR + 0x200)
	b	FreeRTOS_SWI_Handler

.org (FREERTOS_VBAR + 0x280)
	b	FreeRTOS_IRQ_Handler

.org (FREERTOS_VBAR + 0x300)
	b	.

.org (FREERTOS_VBAR + 0x380)
	b	.

.org (FREERTOS_VBAR + 0x400)
	b	.

.org (FREERTOS_VBAR + 0x480)
	b	.

.org (FREERTOS_VBAR + 0x500)
	b	.

.org (FREERTOS_VBAR + 0x580)
	b	.

.org (FREERTOS_VBAR + 0x600)
	b	.

.org (FREERTOS_VBAR + 0x680)
	b	.

.org (FREERTOS_VBAR + 0x700)
	b	.

.org (FREERTOS_VBAR + 0x780)
	b	.

.org (FREERTOS_VBAR + 0x800)


    // boot entry
start64:
	mov      x0, #0
	mov      x1, #0
	mov      x2, #0
	mov      x3, #0
	mov      x4, #0
	mov      x5, #0
	mov      x6, #0
	mov      x7, #0
	mov      x8, #0
	mov      x9, #0
	mov      x10, #0
	mov      x11, #0
	mov      x12, #0
	mov      x13, #0
	mov      x14, #0
	mov      x15, #0
	mov      x16, #0
	mov      x17, #0
	mov      x18, #0
	mov      x19, #0
	mov      x20, #0
	mov      x21, #0
	mov      x22, #0
	mov      x23, #0
	mov      x24, #0
	mov      x25, #0
	mov      x26, #0
	mov      x27, #0
	mov      x28, #0
	mov      x29, #0
	mov      x30, #0
	msr     SCTLR_EL3, x1

InitEL3:
	ldr	x1, =_vector_table
	msr	VBAR_EL3,x1
	msr	VBAR_EL1,x1
    mrs x0, S3_1_c15_c2_1
    orr x0, x0, #(1 << 6)
    msr S3_1_c15_c2_1, x0
    isb

	/* Enable Trapping of SIMD/FPU register for standalone BSP */
	mov      x0, #0
	msr      CPTR_EL3, x0
	isb      sy

	/* Configure SCR_EL3 */
	mov      w1, #0              	//; Initial value of register is unknown
	orr      w1, w1, #(1 << 11)  	//; Set ST bit (Secure EL1 can access CNTPS_TVAL_EL1, CNTPS_CTL_EL1 & CNTPS_CVAL_EL1)
									//; Set ST bit (disable trapping of timer control registers)
	orr      w1, w1, #(1 << 10)  	//; Set RW bit (EL1 is AArch64, as this is the Secure world)
									//; Set RW bit (next lower EL in aarch64)
	orr      w1, w1, #(1 << 3)   	//; Set EA bit (SError routed to EL3)
	orr      w1, w1, #(1 << 2)   	//; Set FIQ bit (FIQs routed to EL3)
	orr		 w1, w1, #(1 << 1)		//; Set IRQ bit (IRQs routed to EL3)
	msr      SCR_EL3, x1

	/* Enable SError Exception for asynchronous abort */
	mrs 	x1,DAIF
	bic	x1,x1,#(0x1<<8)
	msr	DAIF,x1

	/* Disable MMU first */
	mov	x1,#0x0
	msr     SCTLR_EL3, x1
	isb
	// read cpu id
	mrs     x1, mpidr_el1
	and     x1, x1, #3
	cbz	x1, PrimaryCore
	b	SecondaryCore

PrimaryCore:
	/*Define stack pointer for current exception level*/
	ldr	 x2,=EL3_stack
	mov	 sp,x2
	ldr	 x2,=EL0_stack
	msr	 SP_EL0, x2
	msr	 SPSel, #0

	/*
	 * Clear .bss section
	*/
	ldr   x1, =__bss_start
	ldr   x2, =__bss_end
	ldr   w3, =0
bss_clear_loop:
	cmp   x1, x2
	beq   bss_clear_loop_end
	str   w3, [x1], #4
	b     bss_clear_loop
bss_clear_loop_end:
    bl 	 SystemInit0
	b 	 main		//jump to start

SecondaryCore:
	/*Define stack pointer for current exception level*/
	ldr	 x2,=EL3_stack_s
	mov	 sp,x2
	ldr	 x2,=EL0_stack_s
	msr	 SP_EL0, x2
	msr	 SPSel, #0
	bl		SystemInit1
	b 	 	main1		//jump to start


    // synchronous
SynchronousInterruptHandler:
    msr	 SPSel, #0
    saveregister
    bl       SynchronousInterrupt
    restoreregister
    eret


    // IRQ
IRQInterruptHandler:
    msr	 SPSel, #0
    saveregister
    bl       IRQInterrupt
    restoreregister
    eret


    // FIQ
FIQInterruptHandler:
    msr	 SPSel, #0
    saveregister
    bl       FIQInterrupt
    restoreregister
    eret


    // SError
SErrorInterruptHandler:
    msr	 SPSel, #0
    saveregister
    b       SErrorInterrupt
    restoreregister
    eret


.end
