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
#define USB_OS_DELAY_MS(ms)   vTaskDelay(pdMS_TO_TICKS(ms))
#elif defined(__RTTHREAD__) || defined(RT_USING_COMPONENTS_INIT)
#include <rtthread.h>
#define USB_OS_DELAY_MS(ms)   rt_thread_mdelay(ms)
#else
#define USB_OS_DELAY_MS(ms)   do { for (volatile int _i = 0; _i < (ms) * 0x2000; _i++); } while(0)
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

        USB_OS_DELAY_MS(200);

        /* 2. Set PHY to Device role and enable PHY */
        SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;

        USB_OS_DELAY_MS(20);

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
