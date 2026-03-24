/**************************************************************************//**
 * @file     plat_touch.h
 * @brief    for plat touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __PLAT_TOUCH_H__
#define __PLAT_TOUCH_H__

#include <stdint.h>

typedef struct
{
    void    *m_pvI2C;
    uint8_t *m_pu8Reg;
    uint32_t m_u32RegLen;
    uint8_t *m_pu8Data;
    uint32_t m_u32DataLen;
    void    *m_pvPrivate;
    uint8_t  m_u8DevAddr;

} S_TOUCH_IF_I2C;

int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx);
int32_t touch_plat_i2c_read(S_TOUCH_IF_I2C *psIfCtx);
int32_t touch_plat_i2c_write(S_TOUCH_IF_I2C *psIfCtx);
void touch_plat_i2c_fini(S_TOUCH_IF_I2C *psIfCtx);

#endif /* __PLAT_TOUCH_H__*/
