/**************************************************************************//**
 * @file     task_lv.c
 * @brief    Initialize LVGL task.
 *
 * @note
 * Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "lvgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#define CONFIG_LV_TASK_STACKSIZE     2048
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Base[];
extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Limit[];

typedef struct {
    size_t total_bytes;
    size_t current_free_bytes;
    size_t min_ever_free_bytes;
    size_t peak_used_bytes;
} heap_monitor_t;

static heap_monitor_t s_heap_mon = {0};

static int heap_stats_parser(void *param, char const *format, ...)
{
    heap_monitor_t *mon = (heap_monitor_t *)param;
    va_list ap;
    va_start(ap, format);

    /* Arm C Library outputs: "%d bytes in %d free blocks..." or "%u bytes in %d free blocks..." */
    if (format != NULL && strstr(format, "free blocks") != NULL)
    {
        unsigned int free_bytes = va_arg(ap, unsigned int);
        mon->current_free_bytes = free_bytes;

        if (mon->min_ever_free_bytes == 0 || free_bytes < mon->min_ever_free_bytes)
        {
            mon->min_ever_free_bytes = free_bytes;
        }
        if (mon->total_bytes >= mon->min_ever_free_bytes)
        {
            mon->peak_used_bytes = mon->total_bytes - mon->min_ever_free_bytes;
        }
    }

    va_end(ap);
    return 0;
}

static void query_heap_high_water_mark(void)
{
    if (s_heap_mon.total_bytes == 0)
    {
        s_heap_mon.total_bytes = (size_t)Image$$ARM_LIB_HEAP$$ZI$$Limit - (size_t)Image$$ARM_LIB_HEAP$$ZI$$Base;
        s_heap_mon.current_free_bytes = s_heap_mon.total_bytes;
        s_heap_mon.min_ever_free_bytes = s_heap_mon.total_bytes;
    }

    __heapstats(heap_stats_parser, &s_heap_mon);

    size_t cur_used_bytes = (s_heap_mon.total_bytes >= s_heap_mon.current_free_bytes) ?
                            (s_heap_mon.total_bytes - s_heap_mon.current_free_bytes) : 0;

    unsigned int cur_pct_x10 = (s_heap_mon.total_bytes > 0) ?
                               (unsigned int)(((uint64_t)cur_used_bytes * 1000ULL) / s_heap_mon.total_bytes) : 0;
    unsigned int peak_pct_x10 = (s_heap_mon.total_bytes > 0) ?
                                (unsigned int)(((uint64_t)s_heap_mon.peak_used_bytes * 1000ULL) / s_heap_mon.total_bytes) : 0;

    printf("[Heap Monitor] Total: %u KB | Used: %u KB (%u.%u%%) | Free: %u KB | Peak: %u KB (%u.%u%%)\n",
           (unsigned int)(s_heap_mon.total_bytes / 1024),
           (unsigned int)(cur_used_bytes / 1024),
           cur_pct_x10 / 10, cur_pct_x10 % 10,
           (unsigned int)(s_heap_mon.current_free_bytes / 1024),
           (unsigned int)(s_heap_mon.peak_used_bytes / 1024),
           peak_pct_x10 / 10, peak_pct_x10 % 10);
}

#if defined(__FREERTOS__)
static const char * task_state_to_str(eTaskState state)
{
    switch (state)
    {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        default:         return "Unknown";
    }
}

static void query_task_stack_high_water_mark(void)
{
#define MAX_TRACK_TASKS 16
    TaskStatus_t asTaskStatus[MAX_TRACK_TASKS];
    UBaseType_t uxArraySize = MAX_TRACK_TASKS;

    UBaseType_t uxTaskCount = uxTaskGetSystemState(asTaskStatus, uxArraySize, NULL);

    printf("\n[Task Stack Monitor - 10s] Total Tasks: %u\n", (unsigned int)uxTaskCount);
    printf("  %-16s %-10s %-6s %-16s %-16s\n", "Task Name", "State", "Prio", "Min Free(Words)", "Min Free(Bytes)");
    printf("  --------------------------------------------------------------------\n");

    for (UBaseType_t i = 0; i < uxTaskCount; i++)
    {
        uint32_t u32FreeWords = (uint32_t)asTaskStatus[i].usStackHighWaterMark;
        uint32_t u32FreeBytes = u32FreeWords * (uint32_t)sizeof(StackType_t);

        printf("  %-16s %-10s %-6u %-16u %-16u\n",
               asTaskStatus[i].pcTaskName,
               task_state_to_str(asTaskStatus[i].eCurrentState),
               (unsigned int)asTaskStatus[i].uxCurrentPriority,
               (unsigned int)u32FreeWords,
               (unsigned int)u32FreeBytes);
    }
    printf("  --------------------------------------------------------------------\n\n");
}
#endif /* defined(__FREERTOS__) */

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

#if defined(__FREERTOS__)
    TickType_t xLastHeapMonitorTime = xTaskGetTickCount();
    TickType_t xLastStackMonitorTime = xTaskGetTickCount();
#endif

    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(1));

#if defined(__FREERTOS__)
        if ((xTaskGetTickCount() - xLastHeapMonitorTime) >= pdMS_TO_TICKS(5000))
        {
            xLastHeapMonitorTime = xTaskGetTickCount();
            query_heap_high_water_mark();
        }

        if ((xTaskGetTickCount() - xLastStackMonitorTime) >= pdMS_TO_TICKS(10000))
        {
            xLastStackMonitorTime = xTaskGetTickCount();
            query_task_stack_high_water_mark();
        }
#endif
    }
}


int task_lv_init(void)
{
    xTaskCreate(lv_tick_task, "lv_tick", 256, NULL, CONFIG_LV_TASK_PRIORITY - 1, NULL);
    xTaskCreate(lv_nuvoton_task, "lv_hdler", CONFIG_LV_TASK_STACKSIZE, NULL, CONFIG_LV_TASK_PRIORITY, NULL);
    return 0;
}
