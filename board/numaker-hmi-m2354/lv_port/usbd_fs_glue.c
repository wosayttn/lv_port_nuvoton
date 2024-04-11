/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author           Notes
* 2023-8-8        Wayne            First version
*
******************************************************************************/
#include "NuMicro.h"
#include "usb_config.h"

//void USBD_IRQHandler(void)
//{
//    void CherryUSB_USBD_IRQHandler(uint8_t busid);
//    CherryUSB_USBD_IRQHandler(0);
//}

void usb_dc_low_level_init(void)
{
    uint32_t u32RegLockBackup = SYS_IsRegLocked();

    /* Initialize USB PHY */
    SYS_UnlockReg();

    CLK_EnableModuleClock(USBD_MODULE);

    /* Select USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
    SYS_ResetModule(USBD_RST);

    if (u32RegLockBackup)
        SYS_LockReg();

    NVIC_EnableIRQ(USBD_IRQn);
}

void usb_dc_low_level_deinit(void)
{
    uint32_t u32RegLockBackup = SYS_IsRegLocked();

    NVIC_DisableIRQ(USBD_IRQn);

    SYS_UnlockReg();

    CLK_DisableModuleClock(USBD_MODULE);

    if (u32RegLockBackup)
        SYS_LockReg();

}
