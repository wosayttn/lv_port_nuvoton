/**************************************************************************//**
 * @file     task_lv.c
 * @brief    Initialize LVGL task.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

#if LV_USE_OS == LV_OS_FREERTOS

#include "../core/lv_global.h"

#define globals LV_GLOBAL_DEFAULT()

#define CONFIG_LV_TASK_STACKSIZE     8192
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

/* Calculate CPU Usage */
void lv_freertos_task_switch_in_arm9(const char *name)
{
    if (lv_strcmp(name, "IDLE"))
        globals->freertos_idle_task_running = false;
    else
        globals->freertos_idle_task_running = true;

    globals->freertos_task_switch_timestamp = xTaskGetTickCountFromISR();
}

void lv_freertos_task_switch_out_arm9(void)
{
    uint32_t elaps = xTaskGetTickCountFromISR() - globals->freertos_task_switch_timestamp;

    if (globals->freertos_idle_task_running)
        globals->freertos_idle_time_sum += elaps;
    else
        globals->freertos_non_idle_time_sum += elaps;
}
#endif

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
        vTaskDelay(pdMS_TO_TICKS(1));
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
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


int task_lv_init(void)
{
    xTaskCreate(lv_tick_task, "lv_tick", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY - 1, NULL);
    xTaskCreate(lv_nuvoton_task, "lv_hdler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    return 0;
}
