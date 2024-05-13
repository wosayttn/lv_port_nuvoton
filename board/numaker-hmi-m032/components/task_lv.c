/**************************************************************************//**
 * @file     task_lv.c
 * @brief    Initialize LVGL task.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

#define CONFIG_LV_TASK_STACKSIZE     2048
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

#if LV_USE_LOG
static void lv_nuvoton_log(lv_log_level_t level, const char *buf)
{
    printf("%s", buf);
}
#endif /* LV_USE_LOG */

void lv_tick_task(void *pdata)
{
    while (1)
    {
        lv_tick_inc(1);
        vTaskDelay((TickType_t) 1 / portTICK_PERIOD_MS);
    }
}

void lv_nuvoton_task(void *pdata)
{
    lv_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_nuvoton_log);
#endif /* LV_USE_LOG */

    lv_tick_set_cb(xTaskGetTickCount);    /*Expression evaluating to current system time in ms*/
    lv_delay_set_cb(vTaskDelay);

    extern void lv_port_disp_init(void);
    lv_port_disp_init();

    extern void lv_port_indev_init(void);
    lv_port_indev_init();

    extern void ui_init(void);
    ui_init();

    while (1)
    {
        lv_task_handler();
        vTaskDelay((TickType_t) 1 / portTICK_PERIOD_MS);
    }
}


int task_lv_init(void)
{
    xTaskCreate(lv_tick_task, "lv_tick", configMINIMAL_STACK_SIZE, NULL, CONFIG_LV_TASK_PRIORITY - 1, NULL);
    xTaskCreate(lv_nuvoton_task, "lv_hdler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    return 0;
}
