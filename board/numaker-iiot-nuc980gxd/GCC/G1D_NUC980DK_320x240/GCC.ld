;
ENTRY(__vector)
MEMORY
{
    RAM (rwx) : ORIGIN = 0x00008000, LENGTH = 0x03FF8000
}
stack_size = 0x00018400;
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
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x00018400;
        __HeapLimit = .;
        __heap_end__ = .;
    } > RAM
    heap_size = __HeapLimit - __HeapBase;
    .stack (NOLOAD) :
    {
        __StackLimit = .;
        . += 0x00010000;
        __StackTop = .;
        . += 0x00018400 - 0x00010000;
    } > RAM
    ASSERT(__HeapBase <= __HeapLimit, "RAM overflowed before system heap")
}
