/**************************************************************************//**
 * @file     plat_touch.c
 * @brief    AD touch driver
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include "plat_touch.h"

#if defined (__GNUC__)
    #define WEAK_DEF   __attribute__((weak))
#else
    #define WEAK_DEF   __weak
#endif

/**
 * @brief Initialize I2C interface for touch controller (weak definition).
 *
 * Weak function that should be implemented in platform-specific glue code
 * (e.g., touch_glue.c) to initialize the I2C peripheral for communicating
 * with the touch controller IC.
 *
 * @param psIfCtx[in,out]  Pointer to I2C interface context structure
 *
 * @return 0 on success, -1 if not implemented
 */
WEAK_DEF int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your init routine in lv_glue.c\n");
    return -1;
}

/**
 * @brief Read from I2C touch controller (weak definition).
 *
 * Weak function that should be implemented in platform-specific glue code
 * to perform I2C read operations from the touch controller device.
 *
 * @param psIfCtx[in,out]  Pointer to I2C interface context structure
 *
 * @return 0 on success, -1 if not implemented
 */
WEAK_DEF int32_t touch_plat_i2c_read(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your i2c_read routine in lv_glue.c\n");
    return -1;
}

/**
 * @brief Write to I2C touch controller (weak definition).
 *
 * Weak function that should be implemented in platform-specific glue code
 * to perform I2C write operations to the touch controller device.
 *
 * @param psIfCtx[in,out]  Pointer to I2C interface context structure
 *
 * @return 0 on success, -1 if not implemented
 */
WEAK_DEF int32_t touch_plat_i2c_write(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your ad i2c_write routine in lv_glue.c\n");
    return -1;
}

/**
 * @brief Finalize/cleanup I2C interface for touch controller (weak definition).
 *
 * Weak function that should be implemented in platform-specific glue code
 * to clean up and release the I2C interface when touch controller is no longer needed.
 *
 * @param psIfCtx[in]  Pointer to I2C interface context structure
 */
WEAK_DEF void touch_plat_i2c_fini(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your fini routine in lv_glue.c\n");
}


