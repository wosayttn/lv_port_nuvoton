/**************************************************************************//**
 * @file     task_lv.c
 * @brief    Initialize LVGL task.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lv_glue.h"

#define CONFIG_LV_TASK_STACKSIZE     16384
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
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void lv_nuvoton_task(void *pdata)
{
    lv_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_nuvoton_log);
#endif /* LV_USE_LOG */

#if LV_USE_DRAW_GDMA
    void lv_draw_gdma_init(void);
    lv_draw_gdma_init();
#endif

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

#if (configSUPPORT_STATIC_ALLOCATION==1)

__attribute__((section("DTCM.Init"), aligned(8))) static StaticTask_t idle_tcb;
__attribute__((section("DTCM.Init"), aligned(8))) static StackType_t idle_stack[256];
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize = 256;
}


__attribute__((section("DTCM.Init"), aligned(8))) static StaticTask_t timer_tcb;
__attribute__((section("DTCM.Init"), aligned(8))) static StackType_t timer_stack[256];
void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer,
    StackType_t **ppxTimerTaskStackBuffer,
    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &timer_tcb;
    *ppxTimerTaskStackBuffer = timer_stack;
    *pulTimerTaskStackSize = 256;
}

__attribute__((section("DTCM.Init"), aligned(8))) static StaticTask_t lvgl_tcb;
__attribute__((section("DTCM.Init"), aligned(8))) static StackType_t lvgl_stack[CONFIG_LV_TASK_STACKSIZE];

int task_lv_init(void)
{	
  	printf("stack addr = %p\n", lvgl_stack);
    xTaskCreate(lv_tick_task, "lv_tick", configMINIMAL_STACK_SIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    xTaskCreateStatic(lv_nuvoton_task, "lv_hdler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, lvgl_stack, &lvgl_tcb);
    return 0;
}

#else

int task_lv_init(void)
{	
    xTaskCreate(lv_tick_task, "lv_tick", configMINIMAL_STACK_SIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    xTaskCreate(lv_nuvoton_task, "lv_hdler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    return 0;
}

#endif