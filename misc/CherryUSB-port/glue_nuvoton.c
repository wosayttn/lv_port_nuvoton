/*
 * Copyright (c) 2025, Kenny Tseng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "glue_nuvoton.h"

const S_USBD_CB g_usbd_cb[CONFIG_USBDEV_MAX_BUS] = 
{
#if defined(BSP_USING_USBD)
    [DEF_DC_USBID_FS] =
    {
        .usb_dc_init_cb = usb_fs_dc_init,
        .usb_dc_deinit_cb = usb_fs_dc_deinit,
        .usbd_set_address_cb = usbd_fs_set_address,
        .usbd_set_remote_wakeup_cb = usbd_fs_set_remote_wakeup,
        .usbd_get_port_speed_cb = usbd_fs_get_port_speed,
        .usbd_ep_open_cb = usbd_fs_ep_open,
        .usbd_ep_close_cb = usbd_fs_ep_close,
        .usbd_ep_set_stall_cb = usbd_fs_ep_set_stall,
        .usbd_ep_clear_stall_cb = usbd_fs_ep_clear_stall,
        .usbd_ep_is_stalled_cb = usbd_fs_ep_is_stalled,
        .usbd_ep_start_write_cb = usbd_fs_ep_start_write,
        .usbd_ep_start_read_cb = usbd_fs_ep_start_read,
    },
#endif
#if defined(BSP_USING_HSUSBD)
    [DEF_DC_USBID_HS] =
    {
        .usb_dc_init_cb = usb_hs_dc_init,
        .usb_dc_deinit_cb = usb_hs_dc_deinit,
        .usbd_set_address_cb = usbd_hs_set_address,
        .usbd_set_remote_wakeup_cb = usbd_hs_set_remote_wakeup,
        .usbd_get_port_speed_cb = usbd_hs_get_port_speed,
        .usbd_ep_open_cb = usbd_hs_ep_open,
        .usbd_ep_close_cb = usbd_hs_ep_close,
        .usbd_ep_set_stall_cb = usbd_hs_ep_set_stall,
        .usbd_ep_clear_stall_cb = usbd_hs_ep_clear_stall,
        .usbd_ep_is_stalled_cb = usbd_hs_ep_is_stalled,
        .usbd_ep_start_write_cb = usbd_hs_ep_start_write,
        .usbd_ep_start_read_cb = usbd_hs_ep_start_read,
    },
#endif
};

int usb_dc_init(uint8_t busid)
{
    usb_dc_low_level_init(busid);

    return g_usbd_cb[busid].usb_dc_init_cb(busid);
}

int usb_dc_deinit(uint8_t busid)
{
    int ret;

    ret = g_usbd_cb[busid].usb_dc_deinit_cb(busid);
    usb_dc_low_level_deinit(busid);

    return ret;
}

int usbd_set_address(uint8_t busid, const uint8_t addr)
{
    return g_usbd_cb[busid].usbd_set_address_cb(busid, addr);
}

int usbd_set_remote_wakeup(uint8_t busid)
{
    return g_usbd_cb[busid].usbd_set_remote_wakeup_cb(busid);
}

uint8_t usbd_get_port_speed(uint8_t busid)
{
    return g_usbd_cb[busid].usbd_get_port_speed_cb(busid);
}

int usbd_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep)
{
    return g_usbd_cb[busid].usbd_ep_open_cb(busid, ep);
}

int usbd_ep_close(uint8_t busid, const uint8_t ep)
{
    return g_usbd_cb[busid].usbd_ep_close_cb(busid, ep);
}

int usbd_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    return g_usbd_cb[busid].usbd_ep_set_stall_cb(busid, ep);
}

int usbd_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    return g_usbd_cb[busid].usbd_ep_clear_stall_cb(busid, ep);
}

int usbd_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    return g_usbd_cb[busid].usbd_ep_is_stalled_cb(busid, ep, stalled);
}

int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    return g_usbd_cb[busid].usbd_ep_start_write_cb(busid, ep, data, data_len);
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    return g_usbd_cb[busid].usbd_ep_start_read_cb(busid, ep, data, data_len);
}

