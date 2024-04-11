/**************************************************************************//**
 * @file     main.c
 * @brief    M2354 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

int task_msc_sfd_init(void)
{
    void msc_sfud_init(uint8_t busid, uint32_t reg_base);
    msc_sfud_init(0, USBD_BASE);

    return 0;
}
