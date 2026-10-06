/*
 * Copyright (c) 2025, Kenny Tseng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "NuMicro.h"
#include "usbd_core.h"
#include "glue_nuvoton.h"

// Compile-time configuration (that can be externally overridden if necessary)
// Maximum number of endpoints
#ifndef USBD_MAX_ENDPOINT_NUM
    #define USBD_MAX_ENDPOINT_NUM           (HSUSBD_MAX_EP)
#endif

// Maximum packet size for Endpoint 0
#ifndef USBD_EP0_MAX_PACKET_SIZE
    #define USBD_EP0_MAX_PACKET_SIZE        (64)
#endif

// Maximum RAM for endpoint buffers
#ifndef USBD_BUF_SIZE
    #define USBD_BUF_SIZE                   (8192)
#endif

#ifndef NVT_ITCM
    #define NVT_ITCM
#endif

// Macros
// Macro for porting compatibility
#define USBD_t          HSUSBD_T
#define USBD_EP_t       HSUSBD_EP_T

#define USBD_HW(port) ((USBD_t *)g_usbdev_bus[port].reg_base)

typedef enum
{
    PERIPH_EPA = 0,
    PERIPH_EPB = 1,
    PERIPH_EPC = 2,
    PERIPH_EPD = 3,
    PERIPH_EPE = 4,
    PERIPH_EPF = 5,
    PERIPH_EPG = 6,
    PERIPH_EPH = 7,
    PERIPH_EPI = 8,
    PERIPH_EPJ = 9,
    PERIPH_EPK = 10,
    PERIPH_EPL = 11,
    PERIPH_EPM = 12,
    PERIPH_EPN = 13,
    PERIPH_EPO = 14,
    PERIPH_EPP = 15,
    PERIPH_EPQ = 16,
    PERIPH_EPR = 17,
    PERIPH_EPS = 18,
    PERIPH_EPT = 19,
    PERIPH_EPU = 20,
    PERIPH_EPV = 21,
    PERIPH_EPW = 22,
    PERIPH_EPX = 23,
    PERIPH_EPY = 24,
    PERIPH_EPZ = 25,
    PERIPH_MAX_EP = USBD_MAX_ENDPOINT_NUM,
} EP_Num_t;

static const uint8_t epcfg_eptype_table[] =
{
    [USB_ENDPOINT_TYPE_ISOCHRONOUS] = 3 << HSUSBD_EPCFG_EPTYPE_Pos,
                                        [USB_ENDPOINT_TYPE_BULK]        = 1 << HSUSBD_EPCFG_EPTYPE_Pos,
                                        [USB_ENDPOINT_TYPE_INTERRUPT]   = 2 << HSUSBD_EPCFG_EPTYPE_Pos,
};

static const uint8_t eprspctl_eptype_table[] =
{
    [USB_ENDPOINT_TYPE_ISOCHRONOUS] = 2 << HSUSBD_EPRSPCTL_MODE_Pos, /* Fly Mode */
                                        [USB_ENDPOINT_TYPE_BULK]        = 0 << HSUSBD_EPRSPCTL_MODE_Pos, /* Auto-Validate Mode */
                                        [USB_ENDPOINT_TYPE_INTERRUPT]   = 1 << HSUSBD_EPRSPCTL_MODE_Pos, /* Manual-Validate Mode */
};


// Endpoint information
typedef struct
{
    uint16_t ep_mps;                        // Endpoint max packet size
    uint8_t ep_type;                        // Endpoint type
    uint8_t ep_stalled;                     // Endpoint stall flag
    uint8_t *volatile xfer_buf;             // Pointer to buf
    volatile uint32_t xfer_len;             // Number of bytes to transfer
    volatile uint32_t actual_xfer_len;      // Number of totally transferred bytes
    volatile uint32_t xfer_transferring;    // Number of transferred bytes in last transfer
    uint8_t ep_addr;                        // Endpoint address
} usb_dc_ep_state;

/* Driver state */
typedef struct
{
    uint8_t dev_addr;
    volatile uint8_t usbd_set_address_flag;
    uint32_t bufseg_addr;                     // Allocated USB RAM
    volatile uint8_t setup_packet[8];         // Setup Packet data
    usb_dc_ep_state ep_state[PERIPH_MAX_EP];  // Endpoint parameters
    usb_dc_ep_state cep_state[2];             // Control endpoint parameters
} usb_dc_config_priv;

static usb_dc_config_priv g_nuvoton_udc[1];
// Auxiliary functions
static USBD_EP_t *USBD_EndpointEntry(uint8_t busid, uint8_t ep_addr, bool add)
{
    EP_Num_t ep_index;
    USBD_t *husbd = USBD_HW(busid);
    USBD_EP_t *periph_ep;
    usb_dc_ep_state *ep_state;

    for (ep_index = PERIPH_EPA, ep_state = &g_nuvoton_udc->ep_state[PERIPH_EPA], periph_ep = husbd->EP;
         ep_index < PERIPH_MAX_EP;
         ep_index++, ep_state++, periph_ep++)
    {
        if (add)
        {
            if (0 == (periph_ep->EPCFG & HSUSBD_EPCFG_EPEN_Msk)) return periph_ep;
        }
        else
        {
            if (ep_state->ep_addr == ep_addr) return periph_ep;
        }
    }

    return NULL;
}

static void USBD_EndpointConfigureBuffer(uint8_t busid)
{
    // USB RAM beyond what we've allocated above is available to the user(Control endpoint and setup package used.)
    g_nuvoton_udc->bufseg_addr = USBD_EP0_MAX_PACKET_SIZE;

    // Reconfigures the buffer segmentation for all enabled endpoints.
    EP_Num_t ep_index;
    USBD_EP_t *periph_ep;

    for (ep_index = PERIPH_EPA, periph_ep = &USBD_HW(busid)->EP[PERIPH_EPA]; ep_index < PERIPH_MAX_EP; ep_index++, periph_ep++)
    {
        if (0 == (periph_ep->EPCFG & HSUSBD_EPCFG_EPEN_Msk)) continue;

        // Update the Endpoint Buffer Segmentation
        periph_ep->EPBUFSTART = g_nuvoton_udc->bufseg_addr;
        periph_ep->EPBUFEND = g_nuvoton_udc->bufseg_addr + g_nuvoton_udc->ep_state[ep_index].ep_mps - 1U;
        g_nuvoton_udc->bufseg_addr += g_nuvoton_udc->ep_state[ep_index].ep_mps;
        // Set endpoint payload
        periph_ep->EPMPS = g_nuvoton_udc->ep_state[ep_index].ep_mps;
    }
}

// Write data into the USB endpoint buffer (IN transaction)
void USBD_WriteEpBuffer(uint32_t u32EpDat[], uint8_t u8Src[], uint32_t num)
{
    if (num == 0) return;

    uint32_t i = 0;

    // 1. Align src to 4-byte boundary (Head)
    for (; ((uint32_t)(uintptr_t)u8Src & 3u) && (i < num); i++)
    {
        outpb(u32EpDat, *u8Src++);
    }

    // 2. Word write (Body): DMA if > 128 bytes, else CPU-feed
    uint32_t u32Words = (num - i) / 4u;
    if (u32Words > 0)
    {
        uint32_t u32Bytes = u32Words * 4u;
        bool bDmaDone = false;

        if (u32Bytes > 128)
        {
            USBD_t *husbd = USBD_HW(DEF_DC_USBID_HS);
            if (!husbd)
            {
                husbd = (USBD_t *)((uintptr_t)u32EpDat & ~0x0FFFu);
            }

            if (husbd != NULL)
            {
                uintptr_t ptr = (uintptr_t)u32EpDat;
                uintptr_t ep_start = (uintptr_t)&husbd->EP[0];
                uintptr_t ep_end   = (uintptr_t)&husbd->EP[PERIPH_MAX_EP - 1];

                if (ptr >= ep_start && ptr <= ep_end && (((ptr - ep_start) % sizeof(USBD_EP_t)) == 0))
                {
                    USBD_EP_t *periph_ep = (USBD_EP_t *)ptr;
                    if (periph_ep->EPCFG & HSUSBD_EPCFG_EPEN_Msk)
                    {
                        uint32_t ep_num = (periph_ep->EPCFG & HSUSBD_EPCFG_EPNUM_Msk) >> HSUSBD_EPCFG_EPNUM_Pos;
                        if (ep_num >= 1 && ep_num <= 15)
                        {
#ifdef CONFIG_USB_DCACHE_ENABLE
                            usb_dcache_clean((uintptr_t)u8Src, u32Bytes);
#endif
                            /* Clear any pending DMADONE flag */
                            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;

                            /* Bulk IN: DMA read from memory into USB buffer */
                            husbd->DMACTL = (husbd->DMACTL & ~HSUSBD_DMACTL_EPNUM_Msk) | HSUSBD_DMACTL_DMARD_Msk | ep_num | HSUSBD_DMACTL_SVINEP_Msk;
                            husbd->DMAADDR = (uint32_t)u8Src;
                            husbd->DMACNT  = u32Bytes;
                            husbd->DMACTL |= HSUSBD_DMACTL_DMAEN_Msk;

                            uint32_t u32Timeout = SystemCoreClock >> 4;
                            while (!(husbd->BUSINTSTS & HSUSBD_BUSINTSTS_DMADONEIF_Msk))
                            {
                                if (!(husbd->PHYCTL & HSUSBD_PHYCTL_VBUSDET_Msk))
                                    break;
                                if (--u32Timeout == 0)
                                    break;
                            }
                            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;
                            bDmaDone = true;
                        }
                    }
                }
            }
        }

        if (bDmaDone)
        {
            i     += u32Bytes;
            u8Src += u32Bytes;
        }
        else
        {
            uint32_t *pu32Src = (uint32_t *)u8Src;
            for (uint32_t j = 0; j < u32Words; j++)
            {
                outpw(u32EpDat, pu32Src[j]);
            }
            i     += u32Bytes;
            u8Src += u32Bytes;
        }
    }

    // 3. Tail
    for (; i < num; i++)
    {
        outpb(u32EpDat, *u8Src++);
    }
}

// Read data from the USB endpoint buffer (OUT transaction)
void USBD_ReadEpBuffer(uint8_t u8Dst[], uint32_t u32EpDat[], uint32_t num)
{
    if (num == 0) return;

    uint32_t i = 0;

    // 1. Align dst to 4-byte boundary (Head)
    for (; ((uint32_t)(uintptr_t)u8Dst & 3u) && (i < num); i++)
    {
        *u8Dst++ = inpb(u32EpDat);
    }

    // 2. Word read (Body): DMA if > 128 bytes, else CPU-feed
    uint32_t u32Words = (num - i) / 4u;
    if (u32Words > 0)
    {
        uint32_t u32Bytes = u32Words * 4u;
        bool bDmaDone = false;

        if (u32Bytes > 128)
        {
            USBD_t *husbd = USBD_HW(DEF_DC_USBID_HS);
            if (!husbd)
            {
                husbd = (USBD_t *)((uintptr_t)u32EpDat & ~0x0FFFu);
            }

            if (husbd != NULL)
            {
                uintptr_t ptr = (uintptr_t)u32EpDat;
                uintptr_t ep_start = (uintptr_t)&husbd->EP[0];
                uintptr_t ep_end   = (uintptr_t)&husbd->EP[PERIPH_MAX_EP - 1];

                if (ptr >= ep_start && ptr <= ep_end && (((ptr - ep_start) % sizeof(USBD_EP_t)) == 0))
                {
                    USBD_EP_t *periph_ep = (USBD_EP_t *)ptr;
                    if (periph_ep->EPCFG & HSUSBD_EPCFG_EPEN_Msk)
                    {
                        uint32_t ep_num = (periph_ep->EPCFG & HSUSBD_EPCFG_EPNUM_Msk) >> HSUSBD_EPCFG_EPNUM_Pos;
                        if (ep_num >= 1 && ep_num <= 15)
                        {
                            /* Clear any pending DMADONE flag */
                            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;

                            /* Bulk OUT: DMA write from USB buffer into memory */
                            husbd->DMACTL = (husbd->DMACTL & ~(HSUSBD_DMACTL_EPNUM_Msk | HSUSBD_DMACTL_DMARD_Msk | HSUSBD_DMACTL_SVINEP_Msk)) | ep_num;
                            husbd->DMAADDR = (uint32_t)u8Dst;
                            husbd->DMACNT  = u32Bytes;
                            husbd->DMACTL |= HSUSBD_DMACTL_DMAEN_Msk;

  													uint32_t u32Timeout = SystemCoreClock >> 4;
                            while (!(husbd->BUSINTSTS & HSUSBD_BUSINTSTS_DMADONEIF_Msk))
                            {
                                if (!(husbd->PHYCTL & HSUSBD_PHYCTL_VBUSDET_Msk))
                                    break;
                                if (--u32Timeout == 0)
                                    break;
                            }
                            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;

#ifdef CONFIG_USB_DCACHE_ENABLE
                            usb_dcache_invalidate((uintptr_t)u8Dst, u32Bytes);
#endif
                            bDmaDone = true;
                        }
                    }
                }
            }
        }

        if (bDmaDone)
        {
            i     += u32Bytes;
            u8Dst += u32Bytes;
        }
        else
        {
            uint32_t *pu32Dst = (uint32_t *)u8Dst;
            for (uint32_t j = 0; j < u32Words; j++)
            {
                pu32Dst[j] = inpw(u32EpDat);
            }
            i     += u32Bytes;
            u8Dst += u32Bytes;
        }
    }

    // 3. Tail
    for (; i < num; i++)
    {
        *u8Dst++ = inpb(u32EpDat);
    }
}


int usb_hs_dc_init(uint8_t busid)
{
    USBD_t *husbd = USBD_HW(busid);
    uint32_t u32TimeOutCnt;

    // Initial USB engine
    husbd->PHYCTL |= HSUSBD_PHYCTL_PHYEN_Msk;

    // wait PHY clock ready
    #if defined(HSUSBD_PHYCTL_PHYCLKSTB_Msk)
    u32TimeOutCnt = SystemCoreClock >> 5;;
    while (!(husbd->PHYCTL & HSUSBD_PHYCTL_PHYCLKSTB_Msk))
    {
        if (--u32TimeOutCnt == 0) break;
    }
    #else
    //Wayne: For Legacy USBD IP, is without PHYCLKSTB bit, so we need to wait a while for PHY clock ready.
    while (1)
    {
        husbd->EP[EPA].EPMPS = 0x20ul;
        if (husbd->EP[EPA].EPMPS == 0x20ul)
        {
            break;
        }
    }
    #endif

    // Enable USB BUS, CEP global interrupt
    husbd->GINTEN = (HSUSBD_GINTEN_USBIEN_Msk | HSUSBD_GINTEN_CEPIEN_Msk);
    // Enable BUS interrupt
    husbd->BUSINTEN = (HSUSBD_BUSINTEN_DMADONEIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
    NVIC_EnableIRQ(HSUSBD_IRQn);
    // USB High-speed initiate a chirp-sequence
    husbd->OPER = HSUSBD_OPER_HISPDEN_Msk;
    husbd->PHYCTL |= HSUSBD_PHYCTL_DPPUEN_Msk;

    if (husbd->PHYCTL & HSUSBD_PHYCTL_VBUSDET_Msk)
    {
        usbd_event_connect_handler(busid);
    }

    return 0;
}

int usb_hs_dc_deinit(uint8_t busid)
{
    USBD_t *husbd = USBD_HW(busid);

    for (EP_Num_t ep_index = PERIPH_EPA; ep_index < PERIPH_MAX_EP; ep_index++)
    {
        husbd->EP[ep_index].EPCFG = 0;
    }

    NVIC_DisableIRQ(HSUSBD_IRQn);
    memset((void *)g_nuvoton_udc, 0, sizeof(g_nuvoton_udc));
    husbd->PHYCTL &= ~HSUSBD_PHYCTL_DPPUEN_Msk;
    return 0;
}

int usbd_hs_set_address(uint8_t busid, const uint8_t addr)
{
    USBD_t *husbd = USBD_HW(busid);
    uint8_t usbd_addr = husbd->FADDR;

    if ((usbd_addr == 0) && (usbd_addr != addr))
    {
        g_nuvoton_udc->dev_addr = addr;
        g_nuvoton_udc->usbd_set_address_flag = 1;
    }

    return 0;
}

int usbd_hs_set_remote_wakeup(uint8_t busid)
{
    (void) busid;
    return -1;
}

uint8_t usbd_hs_get_port_speed(uint8_t busid)
{
    USBD_t *husbd = USBD_HW(busid);
    return (husbd->OPER & HSUSBD_OPER_CURSPD_Msk) ? USB_SPEED_HIGH : USB_SPEED_FULL;
}

int usbd_hs_ep_close(uint8_t busid, const uint8_t ep)
{
    USBD_t *husbd = USBD_HW(busid);
    USBD_EP_t *periph_ep = USBD_EndpointEntry(busid, ep, false);
    usb_dc_ep_state *ep_state;

    if (USB_EP_GET_IDX(ep) != 0)
    {
        if (periph_ep != NULL)
        {
            uint8_t periph_epnum = periph_ep - husbd->EP;
            usb_dc_ep_state *ep_state = &g_nuvoton_udc->ep_state[periph_epnum];
            // Clear Endpoint information
            memset((void *)ep_state, 0, sizeof(usb_dc_ep_state));
            // Clear Endpoint configure
            husbd->GINTEN &= ~(HSUSBD_GINTEN_EPAIEN_Msk << periph_epnum);
            periph_ep->EPCFG = 0;
            periph_ep->EPRSPCTL = HSUSBD_EP_RSPCTL_TOGGLE;
        }
    }
    else
    {
        ep_state = &g_nuvoton_udc->cep_state[USB_EP_DIR_IS_IN(ep)];
        // Clear Endpoint information
        memset((void *)ep_state, 0, sizeof(usb_dc_ep_state));
    }

    return 0;
}

int usbd_hs_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep)
{
    uint32_t ep_dir_mask, ep_type_mask;
    uint8_t ep_num = USB_EP_GET_IDX(ep->bEndpointAddress);
    uint8_t ep_dir = USB_EP_DIR_IS_IN(ep->bEndpointAddress);
    USBD_t *husbd = USBD_HW(busid);
    usb_dc_ep_state *ep_state;

    // Unconfigure Endpoint the endpoint if it has been used
    usbd_hs_ep_close(busid, ep->bEndpointAddress);

    if (ep_num != 0)
    {
        // Open periph endpoint
        USBD_EP_t *periph_ep = USBD_EndpointEntry(busid, ep->bEndpointAddress, true);

        // Error if all periph endpoints used
        if (periph_ep == NULL)
        {
            return -1;
        }

        uint8_t periph_epnum = periph_ep - husbd->EP;
        ep_state = &g_nuvoton_udc->ep_state[periph_epnum];

        // Error if USB buffer is insufficient
        if ((g_nuvoton_udc->bufseg_addr + USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize)) > USBD_BUF_SIZE)
        {
            USB_LOG_ERR("USB buffer is insufficient\n");
            return -2;
        }

        periph_ep->EPRSPCTL = (HSUSBD_EP_RSPCTL_FLUSH | eprspctl_eptype_table[USB_GET_ENDPOINT_TYPE(ep->bmAttributes)]);

        // Configured endpoint
        ep_dir_mask = ep_dir ? HSUSBD_EP_CFG_DIR_IN : HSUSBD_EP_CFG_DIR_OUT;
        ep_type_mask = epcfg_eptype_table[USB_GET_ENDPOINT_TYPE(ep->bmAttributes)];

        periph_ep->EPCFG = ((ep_num << HSUSBD_EPCFG_EPNUM_Pos) | ep_dir_mask | ep_type_mask | HSUSBD_EPCFG_EPEN_Msk);

        husbd->GINTEN |= (HSUSBD_GINTEN_EPAIEN_Msk << periph_epnum);

    }
    else
    {
        ep_state = &g_nuvoton_udc->cep_state[ep_dir];
    }

    // Store ep address information
    ep_state->ep_addr = ep->bEndpointAddress;

    // Store max packet size information
    ep_state->ep_mps = USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize);

    // Reconfigures the buffer segmentation for all enabled endpoints(must configure EP before configure buffer segmentation)
    USBD_EndpointConfigureBuffer(busid);

    return 0;
}

int usbd_hs_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    if (USB_EP_GET_IDX(ep) != 0)
    {
        USBD_EP_t *periph_ep = USBD_EndpointEntry(busid, ep, false);
        periph_ep->EPRSPCTL = (periph_ep->EPRSPCTL & 0xf7) | HSUSBD_EPRSPCTL_HALT_Msk;
    }
    else
    {
        USBD_HW(busid)->CEPCTL = HSUSBD_CEPCTL_STALLEN_Msk;
    }

    return 0;
}

int usbd_hs_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    if (USB_EP_GET_IDX(ep) != 0)
    {
        USBD_EP_t *periph_ep = USBD_EndpointEntry(busid, ep, false);
        periph_ep->EPRSPCTL = HSUSBD_EPRSPCTL_TOGGLE_Msk;
    }

    return 0;
}

int usbd_hs_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    if (USB_EP_GET_IDX(ep) != 0)
    {
        USBD_EP_t *periph_ep = USBD_EndpointEntry(busid, ep, false);
        *stalled = (periph_ep->EPRSPCTL & HSUSBD_EPRSPCTL_HALT_Msk) > 0 ? 1 : 0;
    }
    else
        *stalled = 0;

    return 0;
}

int usbd_hs_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    if (!data && data_len)
    {
        return -1;
    }

    usb_dc_ep_state *ep_state;
    USBD_t *husbd = USBD_HW(busid);
    USBD_EP_t *periph_ep;

    if (USB_EP_GET_IDX(ep) != 0)
    {
        periph_ep = USBD_EndpointEntry(busid, ep, false);

        if (periph_ep == NULL)
        {
            return -2;
        }

        ep_state = &g_nuvoton_udc->ep_state[periph_ep - husbd->EP];
    }
    else
    {
        ep_state = &g_nuvoton_udc->cep_state[USB_EP_DIR_IS_IN(ep)];
    }

    ep_state->xfer_buf = (uint8_t *)data;
    ep_state->xfer_len = data_len;
    ep_state->actual_xfer_len = 0;

    if (USB_EP_GET_IDX(ep) != 0)
    {
        periph_ep->EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;
        periph_ep->EPINTEN = HSUSBD_EPINTEN_BUFEMPTYIEN_Msk;
    }
    else
    {
        if (ep_state->xfer_len)
        {
            husbd->CEPCTL = HSUSBD_CEPCTL_FLUSH_Msk;
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_INTKIF_Msk;
            husbd->CEPINTEN = HSUSBD_CEPINTEN_INTKIEN_Msk;
        }
        else//Zero Length Packet
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_STSDONEIF_Msk;
            husbd->CEPCTL = HSUSBD_CEPCTL_NAKCLR;
            husbd->CEPINTEN = HSUSBD_CEPINTEN_STSDONEIEN_Msk;
        }
    }

    return 0;
}

int usbd_hs_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    if (!data)
    {
        return -1;
    }

    usb_dc_ep_state *ep_state;
    USBD_t *husbd = USBD_HW(busid);
    USBD_EP_t *periph_ep;

    if (USB_EP_GET_IDX(ep) != 0)
    {
        periph_ep = USBD_EndpointEntry(busid, ep, false);

        if (periph_ep == NULL)
        {
            return -2;
        }

        ep_state = &g_nuvoton_udc->ep_state[periph_ep - husbd->EP];
    }
    else
    {
        ep_state = &g_nuvoton_udc->cep_state[USB_EP_DIR_IS_IN(ep)];
    }

    ep_state->xfer_buf = (uint8_t *)data;
    ep_state->xfer_len = data_len;
    ep_state->actual_xfer_len = 0;

    if (USB_EP_GET_IDX(ep) != 0)
    {
        periph_ep->EPINTEN = HSUSBD_EPINTEN_RXPKIEN_Msk;
    }
    else
    {
        if (ep_state->xfer_len)
        {
            uint32_t u32TimeOutCnt = SystemCoreClock >> 5;;

            while (ep_state->xfer_len < (husbd->CEPRXCNT & HSUSBD_CEPRXCNT_RXCNT_Msk))
                if (--u32TimeOutCnt == 0) return -1;

            USBD_ReadEpBuffer(ep_state->xfer_buf, (uint32_t *)&husbd->CEPDAT, ep_state->xfer_len);
            ep_state->actual_xfer_len += ep_state->xfer_len;

        }

        // alert CherryUSB that we've finished
        usbd_event_ep_out_complete_handler(busid, 0x00U, ep_state->actual_xfer_len);
    }

    return 0;
}

// Event functions *****************************************************************
static void USBD_BusReset(uint8_t busid)
{
    USBD_t *husbd = USBD_HW(busid);
    // Clear Endpoints information
    memset((void *)g_nuvoton_udc->ep_state, 0U, PERIPH_MAX_EP * sizeof(usb_dc_ep_state));
    memset((void *)g_nuvoton_udc->cep_state, 0U, 2 * sizeof(usb_dc_ep_state));

    for (EP_Num_t ep_index = PERIPH_EPA; ep_index < PERIPH_MAX_EP; ep_index++)
    {
        husbd->EP[ep_index].EPCFG = 0U;
    }

    husbd->FADDR = 0U;
    g_nuvoton_udc->dev_addr = 0;

    // Buffer for control endpoint.
    husbd->CEPBUFSTART = 0U;
    husbd->CEPBUFEND = USBD_EP0_MAX_PACKET_SIZE - 1U;

    // USB RAM beyond what we've allocated above is available to the user(Control endpoint used.)
    g_nuvoton_udc->bufseg_addr = USBD_EP0_MAX_PACKET_SIZE;
}

NVT_ITCM void HSUSBDn_IRQHandler(uint8_t busid)
{
    volatile uint32_t u32IntSts, u32BusSts, u32EpIntSts;
    uint32_t u32TimeOutCnt;
    usb_dc_ep_state *ep_state;
    USBD_t *husbd = USBD_HW(busid);

    u32IntSts = husbd->GINTSTS & husbd->GINTEN;

    if (!u32IntSts)    return;

    //------------------------------------------------------------------
    if (u32IntSts & HSUSBD_GINTSTS_USBIF_Msk)
    {
        // Bus event
        u32BusSts = husbd->BUSINTSTS & husbd->BUSINTEN;

        if (u32BusSts & HSUSBD_BUSINTSTS_SOFIF_Msk)
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_SOFIF_Msk;

        if (u32BusSts & HSUSBD_BUSINTSTS_RSTIF_Msk)
        {
            USBD_BusReset(busid);

            husbd->CEPINTEN = (HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            husbd->BUSINTEN = (HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_RSTIF_Msk;
            husbd->CEPINTSTS = 0x1ffc;
            usbd_event_reset_handler(busid);
        }

        if (u32BusSts & HSUSBD_BUSINTSTS_RESUMEIF_Msk)
        {
            husbd->PHYCTL |= (HSUSBD_PHYCTL_PHYEN_Msk | HSUSBD_PHYCTL_DPPUEN_Msk);

            #if defined(HSUSBD_PHYCTL_PHYCLKSTB_Msk)
            u32TimeOutCnt = SystemCoreClock >> 5;;
            while (!(husbd->PHYCTL & HSUSBD_PHYCTL_PHYCLKSTB_Msk))
                if (--u32TimeOutCnt == 0) break;
            #else
            //Wayne: For Legacy USBD IP, is without PHYCLKSTB bit, so we need to wait a while for PHY clock ready.
            while (1)
            {
                husbd->EP[EPA].EPMPS = 0x20ul;
                if (husbd->EP[EPA].EPMPS == 0x20ul)
                {
                    break;
                }
            }
            #endif

            husbd->BUSINTEN = (HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_RESUMEIF_Msk;
            usbd_event_resume_handler(busid);
        }

        if (u32BusSts & HSUSBD_BUSINTSTS_SUSPENDIF_Msk)
        {
            husbd->BUSINTEN = (HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_SUSPENDIF_Msk;
            usbd_event_suspend_handler(busid);
        }

        if (u32BusSts & HSUSBD_BUSINTSTS_HISPDIF_Msk)
        {
            husbd->CEPINTEN = HSUSBD_CEPINTEN_SETUPPKIEN_Msk;
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_HISPDIF_Msk;
        }

        if (u32BusSts & HSUSBD_BUSINTSTS_DMADONEIF_Msk)
        {
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;
        }

        if (u32BusSts & HSUSBD_BUSINTSTS_PHYCLKVLDIF_Msk)
            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_PHYCLKVLDIF_Msk;

        if (u32BusSts & HSUSBD_BUSINTSTS_VBUSDETIF_Msk)
        {
            if (husbd->PHYCTL & HSUSBD_PHYCTL_VBUSDET_Msk)
            {
                // USB Plug In
                husbd->PHYCTL |= (HSUSBD_PHYCTL_PHYEN_Msk | HSUSBD_PHYCTL_DPPUEN_Msk);

                #if defined(HSUSBD_PHYCTL_PHYCLKSTB_Msk)
                u32TimeOutCnt = SystemCoreClock >> 5;;
                while (!(husbd->PHYCTL & HSUSBD_PHYCTL_PHYCLKSTB_Msk))
                    if (--u32TimeOutCnt == 0) break;
                #else
                //Wayne: For Legacy USBD IP, is without PHYCLKSTB bit, so we need to wait a while for PHY clock ready.
                while (1)
                {
                    husbd->EP[EPA].EPMPS = 0x20ul;
                    if (husbd->EP[EPA].EPMPS == 0x20ul)
                    {
                        break;
                    }
                }
                #endif

                husbd->OPER = HSUSBD_OPER_HISPDEN_Msk;
#if defined(HSUSBD_OPER_HISHSEN_Msk) //Wayne
                husbd->OPER |= HSUSBD_OPER_HISHSEN_Msk;
#endif
                usbd_event_connect_handler(busid);
            }
            else
            {
                // USB Un-plug
                husbd->PHYCTL &= ~HSUSBD_PHYCTL_DPPUEN_Msk;
#if defined(HSUSBD_OPER_HISHSEN_Msk) //Wayne
                husbd->OPER &= ~HSUSBD_OPER_HISHSEN_Msk;
#endif
                usbd_event_disconnect_handler(busid);
            }

            husbd->BUSINTSTS = HSUSBD_BUSINTSTS_VBUSDETIF_Msk;
        }

        return;
    }

    //------------------------------------------------------------------
    // Control endpoint event
    if (u32IntSts & HSUSBD_GINTSTS_CEPIF_Msk)
    {
        u32EpIntSts = husbd->CEPINTSTS & husbd->CEPINTEN;

        if (u32EpIntSts & HSUSBD_CEPINTSTS_SETUPTKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_SETUPTKIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_SETUPPKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_SETUPPKIF_Msk;

            g_nuvoton_udc->setup_packet[0] = (uint8_t)((husbd->SETUP1_0 >> 0) & 0xFF);
            g_nuvoton_udc->setup_packet[1] = (uint8_t)((husbd->SETUP1_0 >> 8) & 0xFF);
            g_nuvoton_udc->setup_packet[2] = (uint8_t)((husbd->SETUP3_2 >> 0) & 0xFF);
            g_nuvoton_udc->setup_packet[3] = (uint8_t)((husbd->SETUP3_2 >> 8) & 0xFF);
            g_nuvoton_udc->setup_packet[4] = (uint8_t)((husbd->SETUP5_4 >> 0) & 0xFF);
            g_nuvoton_udc->setup_packet[5] = (uint8_t)((husbd->SETUP5_4 >> 8) & 0xFF);
            g_nuvoton_udc->setup_packet[6] = (uint8_t)((husbd->SETUP7_6 >> 0) & 0xFF);
            g_nuvoton_udc->setup_packet[7] = (uint8_t)((husbd->SETUP7_6 >> 8) & 0xFF);
            usbd_event_ep0_setup_complete_handler(busid, (uint8_t *)g_nuvoton_udc->setup_packet);
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_OUTTKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_OUTTKIF_Msk;
            husbd->CEPINTEN = HSUSBD_CEPINTEN_STSDONEIEN_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_INTKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_INTKIF_Msk;

            if (!(u32EpIntSts & HSUSBD_CEPINTSTS_STSDONEIF_Msk))
            {
                husbd->CEPINTSTS = HSUSBD_CEPINTSTS_TXPKIF_Msk;

                ep_state = &g_nuvoton_udc->cep_state[1];

                uint8_t *data_to_transfer = ep_state->xfer_buf + ep_state->actual_xfer_len;
                uint32_t num_to_transfer  = MIN(ep_state->xfer_len - ep_state->actual_xfer_len, ep_state->ep_mps);

                USBD_WriteEpBuffer((uint32_t *)&husbd->CEPDAT, data_to_transfer, num_to_transfer);

                ep_state->actual_xfer_len += num_to_transfer;

                husbd->CEPINTEN = HSUSBD_CEPINTEN_TXPKIEN_Msk;
                husbd->CEPTXCNT = num_to_transfer;
            }
            else
            {
                husbd->CEPINTSTS = HSUSBD_CEPINTSTS_TXPKIF_Msk;
                husbd->CEPINTEN = (HSUSBD_CEPINTEN_TXPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }

            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_PINGIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_PINGIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_TXPKIF_Msk)
        {
            ep_state = &g_nuvoton_udc->cep_state[1];
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_STSDONEIF_Msk;
            husbd->CEPCTL = HSUSBD_CEPCTL_NAKCLR;

            if (ep_state->actual_xfer_len != ep_state->xfer_len)
            {
                husbd->CEPINTSTS = HSUSBD_CEPINTSTS_INTKIF_Msk;
                husbd->CEPINTEN = HSUSBD_CEPINTEN_INTKIEN_Msk;
            }
            else
            {
                // alert CherryUSB that we've finished
                usbd_event_ep_in_complete_handler(busid, 0x80U, ep_state->actual_xfer_len);
                husbd->CEPINTSTS = HSUSBD_CEPINTSTS_STSDONEIF_Msk;
                husbd->CEPINTEN = (HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }

            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_TXPKIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_RXPKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_RXPKIF_Msk;
            husbd->CEPCTL = HSUSBD_CEPCTL_NAKCLR;
            husbd->CEPINTEN = (HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);

            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_NAKIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_NAKIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_STALLIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_STALLIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_ERRIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_ERRIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_STSDONEIF_Msk)
        {

            if (husbd->CEPINTSTS & HSUSBD_CEPINTSTS_INTKIF_Msk)
            {

                if (g_nuvoton_udc->usbd_set_address_flag == 1)
                    husbd->FADDR = g_nuvoton_udc->dev_addr;

                ep_state = &g_nuvoton_udc->cep_state[1];

                // alert CherryUSB that we've finished
                if (ep_state->actual_xfer_len == ep_state->xfer_len)
                    usbd_event_ep_in_complete_handler(busid, 0x80U, ep_state->actual_xfer_len);
            }

            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_STSDONEIF_Msk;
            husbd->CEPINTEN = HSUSBD_CEPINTEN_SETUPPKIEN_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_BUFFULLIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_BUFFULLIF_Msk;
            return;
        }

        if (u32EpIntSts & HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk)
        {
            husbd->CEPINTSTS = HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk;
            return;
        }

        return;

    }

    //------------------------------------------------------------------
    // Endpoint event
#if 0
    if (u32IntSts & \
            (HSUSBD_GINTSTS_EPAIF_Msk | HSUSBD_GINTSTS_EPBIF_Msk | HSUSBD_GINTSTS_EPCIF_Msk | HSUSBD_GINTSTS_EPDIF_Msk | HSUSBD_GINTSTS_EPEIF_Msk | HSUSBD_GINTSTS_EPFIF_Msk | \
             HSUSBD_GINTSTS_EPGIF_Msk | HSUSBD_GINTSTS_EPHIF_Msk | HSUSBD_GINTSTS_EPIIF_Msk | HSUSBD_GINTSTS_EPJIF_Msk | HSUSBD_GINTSTS_EPKIF_Msk | HSUSBD_GINTSTS_EPLIF_Msk | \
             HSUSBD_GINTSTS_EPMIF_Msk | HSUSBD_GINTSTS_EPNIF_Msk | HSUSBD_GINTSTS_EPOIF_Msk | HSUSBD_GINTSTS_EPPIF_Msk | HSUSBD_GINTSTS_EPQIF_Msk | HSUSBD_GINTSTS_EPRIF_Msk))
#else
    if (u32IntSts & ((( 1U << USBD_MAX_ENDPOINT_NUM) - 1U) << 2U))
#endif
    {
        EP_Num_t ep_index;
        uint32_t mask;
        USBD_EP_t *periph_ep;

        for (ep_index = PERIPH_EPA, mask = HSUSBD_GINTSTS_EPAIF_Msk, periph_ep = &husbd->EP[PERIPH_EPA], ep_state = &g_nuvoton_udc->ep_state[PERIPH_EPA];
             ep_index < PERIPH_MAX_EP;
             ep_index++, mask <<= 1U, periph_ep++, ep_state++)
        {
            if (u32IntSts & mask)
            {
                uint32_t u32EpIntSts = periph_ep->EPINTSTS & periph_ep->EPINTEN;
                // Clear endpoint event flag
                periph_ep->EPINTSTS = u32EpIntSts;
                uint8_t const ep_addr = ep_state->ep_addr;

                if (USB_EP_DIR_IS_IN(ep_addr))
                {
                    if (u32EpIntSts & HSUSBD_EPINTSTS_BUFEMPTYIF_Msk)
                    {
                        // If there is more data to transfer
                        uint8_t *data_to_transfer = ep_state->xfer_buf + ep_state->actual_xfer_len;
                        uint32_t num_to_transfer  = MIN(ep_state->xfer_len - ep_state->actual_xfer_len, ep_state->ep_mps);

                        // Update transferred number
                        ep_state->actual_xfer_len += num_to_transfer;

                        if (ep_state->actual_xfer_len == ep_state->xfer_len)
                        {
                            periph_ep->EPINTSTS = HSUSBD_EPINTSTS_TXPKIF_Msk;
                            periph_ep->EPINTEN = HSUSBD_EPINTEN_TXPKIEN_Msk;
                        }

                        USBD_WriteEpBuffer((uint32_t *)&periph_ep->EPDAT, (uint8_t *)data_to_transfer, num_to_transfer);

                        if (num_to_transfer != ep_state->ep_mps) periph_ep->EPRSPCTL = HSUSBD_EPRSPCTL_SHORTTXEN_Msk;

                    }
                    else if (u32EpIntSts & HSUSBD_EPINTSTS_TXPKIF_Msk)
                    {
                        periph_ep->EPINTEN = 0;
                        // alert CherryUSB that we've finished
                        usbd_event_ep_in_complete_handler(busid, ep_addr, ep_state->actual_xfer_len);
                    }
                }
                else
                {
                    uint32_t num_transferred = periph_ep->EPDATCNT & HSUSBD_EPDATCNT_DATCNT_Msk;

                    USBD_ReadEpBuffer((uint8_t *)ep_state->xfer_buf + ep_state->actual_xfer_len, (uint32_t *)&periph_ep->EPDAT, num_transferred);

                    ep_state->actual_xfer_len += num_transferred;

                    // when the transfer is finished, alert CherryUSB; otherwise, continue accepting more data
                    if ((num_transferred < ep_state->ep_mps) || (ep_state->actual_xfer_len == ep_state->xfer_len))
                    {
                        periph_ep->EPINTEN = 0;
                        usbd_event_ep_out_complete_handler(busid, ep_addr, ep_state->actual_xfer_len);
                    }
                }
            }
        }
    }

}

NVT_ITCM void HSUSBD_IRQHandler(void)
{
    HSUSBDn_IRQHandler(DEF_DC_USBID_HS);
}

#if defined(__M3331_H__)
void USBD20_IRQHandler(void)
{
    HSUSBD_IRQHandler();
}
#endif