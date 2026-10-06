ENTRY(__vector)

#ifndef RAM_ORIGIN
#define RAM_ORIGIN 0x00000000
#endif

#ifndef SVC_STACK_RESERVE
#define SVC_STACK_RESERVE STACK_RESERVE
#endif

MEMORY
{
    RAM (rwx) : ORIGIN = RAM_ORIGIN, LENGTH = MEMSIZE
}

stack_size = STACK_RESERVE;
_stack_start = ORIGIN(RAM) + LENGTH(RAM);
_stack_end = _stack_start - stack_size;

SECTIONS
{
    .text :
    {
        __image_start = .;
        __text_start = .;
        __vectors_start = .;
        KEEP(*(.vectors))
        . = ALIGN(4);
        __vectors_end = .;
        *(.init)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        __text_end = .;
        etext = .;
        _etext = .;
    } > RAM

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > RAM

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > RAM

    .data :
    {
        __data_start__ = .;
        _data = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end__ = .;
        _edata = .;
        edata = .;
    } > RAM

    .bss :
    {
        __bss_start__ = .;
        sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(32);
        __bss_end__ = .;
        ebss = .;
    } > RAM
    bss_size = __bss_end__ - __bss_start__;

    .heap (NOLOAD) :
    {
        __HeapBase = .;
        __heap_start__ = .;
        __end__ = .;
        end = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - STACK_RESERVE;
        __HeapLimit = .;
        __heap_end__ = .;
    } > RAM
    heap_size = __HeapLimit - __HeapBase;

    .stack (NOLOAD) :
    {
        __StackLimit = .;
        . += SVC_STACK_RESERVE;
        __StackTop = .;
        . += STACK_RESERVE - SVC_STACK_RESERVE;
    } > RAM

    ASSERT(__HeapBase <= __HeapLimit, "RAM overflowed before system heap")
}
