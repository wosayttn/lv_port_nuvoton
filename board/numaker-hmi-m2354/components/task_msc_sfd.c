/**************************************************************************//**
 * @file     task_msc_sfd.c
 * @brief    USB MSC SFD exporter.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

int task_msc_sfd_init(void)
{
    void msc_sfud_init(uint8_t busid, uint32_t reg_base);
    msc_sfud_init(0, USBD_BASE);

    return 0;
}
