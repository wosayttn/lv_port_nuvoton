/**************************************************************************//**
 * @file     lv_demo.c
 * @brief    lvgl demo
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"

void lv_user_gui_init(void)
{

#if (LV_USE_DEMO_WIDGETS)
    extern void lv_demo_widgets(void);
    lv_demo_widgets();

#elif (LV_USE_DEMO_MUSIC)
    extern void lv_demo_music(void);
    lv_demo_music();

#endif

}
