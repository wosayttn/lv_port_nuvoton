/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "NuMicro.h"

#if defined(__FREERTOS__)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "glue_nuvoton.h"

void usb_dc_low_level_init(uint8_t busid)
{
    if (busid == DEF_DC_USBID_HS)
    {
        SYS_UnlockReg();

        /* Enable HSUSBD clock to access registers */
        CLK_EnableModuleClock(HSUSBD_MODULE);
        SYS_ResetModule(HSUSBD_RST);

        /* 1. Force Disconnect (SE0): disable DP pull-up and deactivate PHY.
         *    Holding disconnect for >= 200ms ensures the PC host hub detects
         *    device detachment upon MCU reset and clears its internal state. */
        HSUSBD->PHYCTL &= ~HSUSBD_PHYCTL_DPPUEN_Msk;
        SYS->USBPHY &= ~(SYS_USBPHY_HSUSBACT_Msk | SYS_USBPHY_HSUSBEN_Msk);

#if defined(__FREERTOS__)
        vTaskDelay(pdMS_TO_TICKS(200));
#else
        for (volatile int i = 0; i < 0x200000; i++);
#endif

        /* 2. Set PHY to Device role and enable PHY */
        SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;

#if defined(__FREERTOS__)
        vTaskDelay(pdMS_TO_TICKS(20));
#else
        for (volatile int i = 0; i < 0x20000; i++);
#endif

        SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

#if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
        NVIC_SetPriority(HSUSBD_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#endif
    }
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    if (busid == DEF_DC_USBID_HS)
    {
        NVIC_DisableIRQ(HSUSBD_IRQn);
        SYS->USBPHY &= ~(SYS_USBPHY_HSUSBACT_Msk | SYS_USBPHY_HSUSBEN_Msk);
        CLK_DisableModuleClock(HSUSBD_MODULE);
    }
}
