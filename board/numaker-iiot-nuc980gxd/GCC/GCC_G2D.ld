;/*****************************************************************************
; * @file     GCC.ld
; * @brief    NUC980 link file
; *
; * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
; *****************************************************************************/

#define RAM_ORIGIN 0x00008000
#define MEMSIZE 0x07FF8000
#define STACK_RESERVE 0x00018400
#define SVC_STACK_RESERVE 0x00010000
#include "../../../common/arm9_heap.ld"

