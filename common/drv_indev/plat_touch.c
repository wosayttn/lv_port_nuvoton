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

WEAK_DEF int32_t touch_plat_i2c_init(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your init routine in lv_glue.c\n");

    return -1;
}

WEAK_DEF int32_t touch_plat_i2c_read(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your i2c_read routine in lv_glue.c\n");

    return -1;
}

WEAK_DEF int32_t touch_plat_i2c_write(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your ad i2c_write routine in lv_glue.c\n");

    return -1;
}

WEAK_DEF void touch_plat_i2c_fini(S_TOUCH_IF_I2C *psIfCtx)
{
    printf("Implement your fini routine in lv_glue.c\n");
}


