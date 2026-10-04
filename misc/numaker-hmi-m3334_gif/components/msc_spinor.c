/**************************************************************************//**
 * @file     msc_spinor.c
 * @brief    CherryUSB Mass Storage Device (MSC) export for on-board SPI NOR Flash
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "NuMicro.h"
#include "spi_flash.h"
#include "msc_spinor.h"

#include "usbd_core.h"
#include "usbd_msc.h"

#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

#define USBD_VID           0x0416  /* Nuvoton */
#define USBD_PID           0x5020  /* Mass Storage */
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + MSC_DESCRIPTOR_LEN)

static const uint8_t s_au8DeviceDescriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t s_au8ConfigDescriptorHS[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_HS, 0x02)
};

static const uint8_t s_au8ConfigDescriptorFS[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x02)
};

static const uint8_t s_au8DeviceQualityDescriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t s_au8OtherSpeedConfigDescriptorHS[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x02)
};

static const uint8_t s_au8OtherSpeedConfigDescriptorFS[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_HS, 0x02)
};

static const char *s_apszStringDescriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* LangID: English (0x0409) */
    "Nuvoton",                    /* Manufacturer */
    "NuMaker M3334 Flash Disk",   /* Product */
    "20261004",                   /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return s_au8DeviceDescriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return s_au8ConfigDescriptorHS;
    } else if (speed == USB_SPEED_FULL) {
        return s_au8ConfigDescriptorFS;
    } else {
        return NULL;
    }
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return s_au8DeviceQualityDescriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return s_au8OtherSpeedConfigDescriptorHS;
    } else if (speed == USB_SPEED_FULL) {
        return s_au8OtherSpeedConfigDescriptorFS;
    } else {
        return NULL;
    }
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(s_apszStringDescriptors) / sizeof(char *))) {
        return NULL;
    }
    return s_apszStringDescriptors[index];
}

static const struct usb_descriptor s_sMscDescriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

static volatile bool s_bUsbConnected = false;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    switch (event) {
        case USBD_EVENT_RESET:
            printf("[USB MSC] Bus Reset\n");
            break;
        case USBD_EVENT_CONNECTED:
            s_bUsbConnected = true;
            printf("[USB MSC] Connected to Host\n");
            break;
        case USBD_EVENT_DISCONNECTED:
            s_bUsbConnected = false;
            printf("[USB MSC] Disconnected\n");
            break;
        case USBD_EVENT_RESUME:
            printf("[USB MSC] Resume\n");
            break;
        case USBD_EVENT_SUSPEND:
            printf("[USB MSC] Suspend\n");
            break;
        case USBD_EVENT_CONFIGURED:
            printf("[USB MSC] Configured by Host - Ready for Transfer\n");
            break;
        default:
            break;
    }
}

/* CherryUSB MSC storage callbacks */
void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    (void)busid;
    (void)lun;
    uint32_t u32Cap = SpiFlash_GetCapacity();
    if (u32Cap == 0)
    {
        u32Cap = 2 * 1024 * 1024; /* Fallback default 2MB */
    }
    *block_size = SPI_FLASH_SECTOR_SIZE; /* 4096 bytes per sector */
    *block_num  = u32Cap / SPI_FLASH_SECTOR_SIZE;
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;
    (void)lun;
    uint32_t u32Cap = SpiFlash_GetCapacity();
    uint32_t u32Addr = sector * SPI_FLASH_SECTOR_SIZE;

    if (u32Addr >= u32Cap)
    {
        return -1;
    }
    if (u32Addr + length > u32Cap)
    {
        length = u32Cap - u32Addr;
    }

    SpiFlash_QPI_FastRead(u32Addr, buffer, length);
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;
    (void)lun;
    uint32_t u32Cap = SpiFlash_GetCapacity();
    uint32_t u32Addr = sector * SPI_FLASH_SECTOR_SIZE;

    if (u32Addr >= u32Cap)
    {
        return -1;
    }

    uint32_t u32Written = 0;
    static uint8_t s_au8VerifyBuf[SPI_FLASH_SECTOR_SIZE];

    while (u32Written < length)
    {
        uint32_t u32CurAddr = u32Addr + u32Written;
        uint32_t u32Chunk = length - u32Written;

        if (u32CurAddr >= u32Cap)
        {
            return -1;
        }

        if (u32Chunk >= SPI_FLASH_SECTOR_SIZE)
        {
            /* Verify if sector is already identical before erasing */
            SpiFlash_QPI_FastRead(u32CurAddr, s_au8VerifyBuf, SPI_FLASH_SECTOR_SIZE);
            if (memcmp(s_au8VerifyBuf, buffer + u32Written, SPI_FLASH_SECTOR_SIZE) != 0)
            {
                SpiFlash_WriteSector(u32CurAddr, buffer + u32Written);
            }
            u32Written += SPI_FLASH_SECTOR_SIZE;
        }
        else
        {
            /* Partial sector: read-modify-write */
            SpiFlash_QPI_FastRead(u32CurAddr, s_au8VerifyBuf, SPI_FLASH_SECTOR_SIZE);
            if (memcmp(s_au8VerifyBuf, buffer + u32Written, u32Chunk) != 0)
            {
                memcpy(s_au8VerifyBuf, buffer + u32Written, u32Chunk);
                SpiFlash_WriteSector(u32CurAddr, s_au8VerifyBuf);
            }
            u32Written += u32Chunk;
        }
    }
    return 0;
}

static struct usbd_interface s_sMscIntf;
static bool s_bMscInitialized = false;

int msc_spinor_init(void)
{
    if (s_bMscInitialized)
        return 0;

    printf("\n[USB MSC] Starting CherryUSB MSC for SPI NOR Flash (HSUSBD)...\n");

    /* Ensure SPI NOR Flash hardware is initialized */
    if (SpiFlash_Init() != 0)
    {
        printf("[USB MSC] Error: SPI Flash initialization failed!\n");
        return -1;
    }

    printf("[USB MSC] SPI Flash Capacity: %u KB, Block Size: %u B\n",
           SpiFlash_GetCapacity() / 1024, SPI_FLASH_SECTOR_SIZE);

    usbd_desc_register(0, &s_sMscDescriptor);
    usbd_add_interface(0, usbd_msc_init_intf(0, &s_sMscIntf, MSC_OUT_EP, MSC_IN_EP));

    int ret = usbd_initialize(0, HSUSBD_BASE, usbd_event_handler);
    if (ret != 0)
    {
        printf("[USB MSC] Error: usbd_initialize failed (%d)!\n", ret);
        return ret;
    }

    s_bMscInitialized = true;
    printf("[USB MSC] MSC active! Connect USB port to PC to access flash disk.\n\n");
    return 0;
}

bool msc_spinor_is_connected(void)
{
    return s_bUsbConnected;
}
