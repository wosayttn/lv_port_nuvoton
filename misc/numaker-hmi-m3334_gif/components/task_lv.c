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

#include "core/lv_global.h"
#include "display/lv_display_private.h"
#include "debugging/sysmon/lv_sysmon_private.h"

#define CONFIG_LV_TASK_STACKSIZE     2048
#define CONFIG_LV_TASK_PRIORITY      (configMAX_PRIORITIES-1)

extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Base[];
extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Limit[];
extern uint32_t Load$$LR$$LR_ROM$$Length[];
extern uint32_t Image$$ER_ROM$$Length[];
extern uint32_t Image$$RW_RAM$$Length[];
extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Length[];

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

//    printf("[Heap Monitor] Total: %u KB | Used: %u KB (%u.%u%%) | Free: %u KB | Peak: %u KB (%u.%u%%)\n",
//           (unsigned int)(s_heap_mon.total_bytes / 1024),
//           (unsigned int)(cur_used_bytes / 1024),
//           cur_pct_x10 / 10, cur_pct_x10 % 10,
//           (unsigned int)(s_heap_mon.current_free_bytes / 1024),
//           (unsigned int)(s_heap_mon.peak_used_bytes / 1024),
//           peak_pct_x10 / 10, peak_pct_x10 % 10);
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

void $Sub$$lv_mem_monitor_core(lv_mem_monitor_t * mon_p)
{
    query_heap_high_water_mark();

    mon_p->total_size = s_heap_mon.total_bytes;
    mon_p->free_size = s_heap_mon.current_free_bytes;
    mon_p->max_used = s_heap_mon.peak_used_bytes;
    if(mon_p->total_size > 0) {
        size_t used = (mon_p->total_size >= mon_p->free_size) ? (mon_p->total_size - mon_p->free_size) : 0;
        mon_p->used_pct = (uint8_t)(((uint64_t)used * 100) / mon_p->total_size);
    }
}

#if LV_USE_MEM_MONITOR
static void custom_rom_ram_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(subject);
    lv_obj_t * label = lv_observer_get_target(observer);
    if(label == NULL) return;

    /* 1. ROM usage */
    uint32_t rom_used_bytes = (uint32_t)Load$$LR$$LR_ROM$$Length;
    uint32_t rom_total_bytes = 512 * 1024;
    uint32_t rom_used_kb = (rom_used_bytes + 1023) / 1024;
    uint32_t rom_pct = (rom_used_bytes * 100) / rom_total_bytes;

    /* 2. RAM usage (Static Data + Stack + Heap Used) */
    uint32_t ram_base = 0x20000000;
    uint32_t ram_limit = (uint32_t)Image$$ARM_LIB_HEAP$$ZI$$Limit;
    uint32_t ram_total_bytes = (ram_limit > ram_base) ? (ram_limit - ram_base) : (320 * 1024);
    uint32_t static_ram_bytes = (uint32_t)Image$$ARM_LIB_HEAP$$ZI$$Base - ram_base;

    size_t heap_used_bytes = (s_heap_mon.total_bytes >= s_heap_mon.current_free_bytes) ?
                             (s_heap_mon.total_bytes - s_heap_mon.current_free_bytes) : 0;
    uint32_t ram_used_bytes = static_ram_bytes + (uint32_t)heap_used_bytes;
    uint32_t ram_used_kb = (ram_used_bytes + 1023) / 1024;
    uint32_t ram_pct = (ram_total_bytes > 0) ? ((ram_used_bytes * 100) / ram_total_bytes) : 0;

    lv_label_set_text_fmt(label,
                          "ROM: %u KB (%u%%)\n"
                          "RAM: %u KB (%u%%)",
                          (unsigned int)rom_used_kb, (unsigned int)rom_pct,
                          (unsigned int)ram_used_kb, (unsigned int)ram_pct);
}
#endif

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

#if LV_USE_MEM_MONITOR
    lv_display_t * disp = lv_display_get_default();
    if(disp && disp->mem_label) {
        lv_obj_remove_from_subject(disp->mem_label, NULL);
        lv_subject_add_observer_obj(&LV_GLOBAL_DEFAULT()->sysmon_mem.subject,
                                    custom_rom_ram_observer_cb, disp->mem_label, NULL);
        lv_obj_set_style_text_font(disp->mem_label, &lv_font_montserrat_12, 0);
        lv_obj_align(disp->mem_label, LV_USE_MEM_MONITOR_POS, 0, 0);
    }
#endif

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
