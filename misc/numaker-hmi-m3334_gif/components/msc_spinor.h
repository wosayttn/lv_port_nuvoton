/**************************************************************************//**
 * @file     msc_spinor.h
 * @brief    CherryUSB Mass Storage Device (MSC) export for SPI NOR Flash
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __MSC_SPINOR_H__
#define __MSC_SPINOR_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start CherryUSB Mass Storage Device (MSC)
 *        exporting on-board SPI NOR Flash to USB Host (PC).
 * @return 0 on success, negative error code on failure.
 */
int msc_spinor_init(void);

/**
 * @brief Check if USB MSC is connected to a host.
 * @return true if connected.
 */
bool msc_spinor_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __MSC_SPINOR_H__ */
