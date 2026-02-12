/******************************************************************************
 * @file     gcc_arm.ld
 * @brief    GNU Linker Script for MA35H0 based device
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved. 
 ******************************************************************************/ 
_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x4000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x100000;

 MEMORY
{
   DDR_BASEADDR : ORIGIN = 0x80000000, LENGTH = 0x8000000
}

SECTIONS
{
	.text : {
		KEEP(*(.text.boot))
		__start__ = .;
		*(.text .text.* .gnu.linkonce.t*)
	} > DDR_BASEADDR
	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r*)
	} > DDR_BASEADDR
	. = ALIGN(4096);
	PROVIDE(_data = .);
	.data : {
		*(.data .data.* .gnu.linkonce.d*)
	} > DDR_BASEADDR
    .bss (NOLOAD) : {
		. = ALIGN(16);
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(16);
		__bss_end = .;
    } > DDR_BASEADDR
	.heap (NOLOAD) : {
		. = ALIGN(64);
		PROVIDE ( end = . );
		PROVIDE ( _end = . );
		_heap_end = .;
	   . += _HEAP_SIZE;
	   _heap = .;
	   HeapBase = .;
		_heap_start = .;
	   HeapLimit = .;
	} > DDR_BASEADDR
	.stack (NOLOAD) : {
	   . = ALIGN(64);
	   _stack_end = .;
	   _el3_stack_end = .;
	   . += _STACK_SIZE;
	   __el3_stack = .;
	   . += _STACK_SIZE;
	   __el0_stack = .;
	   . += _STACK_SIZE;
	   __el3_stack_s = .;
	   . += _STACK_SIZE;
	   __el0_stack_s = .;
	   _stack = .;
	} > DDR_BASEADDR

	/DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;
