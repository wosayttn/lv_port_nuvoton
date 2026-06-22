/**************************************************************************//**
 * @file     lv_glue.h
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

//#include "numaker_disp.h"
//#include "numaker_touch.h"

#endif /* __LV_GLUE_H__ */
