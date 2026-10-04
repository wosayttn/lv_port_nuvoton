/*
 * Copyright (c) 2025, Kenny Tseng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USB_DC_NUVOTON_H
#define USB_DC_NUVOTON_H

#include "usbd_core.h"
#include "NuMicro.h"

#if defined(__M3331_H__)
#ifndef HSUSBD_IRQn
#define HSUSBD_IRQn USBD20_IRQn
#endif
#ifndef EPBUFSTART
#define EPBUFSTART EPBUFST
#endif
#ifndef CEPBUFSTART
#define CEPBUFSTART CEPBUFST
#endif
#endif

#if (CONFIG_USBDEV_MAX_BUS==2)
#define DEF_DC_USBID_FS    0
#define DEF_DC_USBID_HS    1
#else
#define DEF_DC_USBID_FS    0
#define DEF_DC_USBID_HS    0
#endif

#if (CONFIG_USBHOST_MAX_BUS==2)
#define DEF_HC_USBID_FS    0
#define DEF_HC_USBID_HS    1
#else
#define DEF_HC_USBID_FS    0
#define DEF_HC_USBID_HS    0
#endif

typedef struct  
{
    int (*usb_dc_init_cb)(uint8_t busid);
    int (*usb_dc_deinit_cb)(uint8_t busid);
    int (*usbd_set_address_cb)(uint8_t busid, const uint8_t addr);
    int (*usbd_set_remote_wakeup_cb)(uint8_t busid);
    uint8_t (*usbd_get_port_speed_cb)(uint8_t busid);
    int (*usbd_ep_open_cb)(uint8_t busid, const struct usb_endpoint_descriptor *ep);
    int (*usbd_ep_close_cb)(uint8_t busid, const uint8_t ep);
    int (*usbd_ep_set_stall_cb)(uint8_t busid, const uint8_t ep);
    int (*usbd_ep_clear_stall_cb)(uint8_t busid, const uint8_t ep);
    int (*usbd_ep_is_stalled_cb)(uint8_t busid, const uint8_t ep, uint8_t *stalled);
    int (*usbd_ep_start_write_cb)(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len);
    int (*usbd_ep_start_read_cb)(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len);
} S_USBD_CB;

#if defined(BSP_USING_USBD)
    int usb_fs_dc_init(uint8_t busid);
    int usb_fs_dc_deinit(uint8_t busid);
    int usbd_fs_set_address(uint8_t busid, const uint8_t addr);
    int usbd_fs_set_remote_wakeup(uint8_t busid);
    uint8_t usbd_fs_get_port_speed(uint8_t busid);
    int usbd_fs_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep);
    int usbd_fs_ep_close(uint8_t busid, const uint8_t ep);
    int usbd_fs_ep_set_stall(uint8_t busid, const uint8_t ep);
    int usbd_fs_ep_clear_stall(uint8_t busid, const uint8_t ep);
    int usbd_fs_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled);
    int usbd_fs_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len);
    int usbd_fs_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len);
#endif

#if defined(BSP_USING_HSUSBD)
    int usb_hs_dc_init(uint8_t busid);
    int usb_hs_dc_deinit(uint8_t busid);
    int usbd_hs_set_address(uint8_t busid, const uint8_t addr);
    int usbd_hs_set_remote_wakeup(uint8_t busid);
    uint8_t usbd_hs_get_port_speed(uint8_t busid);
    int usbd_hs_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep);
    int usbd_hs_ep_close(uint8_t busid, const uint8_t ep);
    int usbd_hs_ep_set_stall(uint8_t busid, const uint8_t ep);
    int usbd_hs_ep_clear_stall(uint8_t busid, const uint8_t ep);
    int usbd_hs_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled);
    int usbd_hs_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len);
    int usbd_hs_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len);
#endif


void usb_dc_low_level_init(uint8_t busid);
void usb_dc_low_level_deinit(uint8_t busid);

#endif /* USB_DC_NUVOTON_H */
