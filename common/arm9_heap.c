#include "FreeRTOS.h"
#include "task.h"

#if defined(__CC_ARM)

extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Base;
extern unsigned int Image$$ARM_LIB_HEAP$$ZI$$Limit;
extern void $Super$$__rt_lib_init(unsigned int heap_base, unsigned int heap_limit);

void $Sub$$__rt_lib_init(unsigned int heap_base, unsigned int heap_limit)
{
    (void)heap_base;
    (void)heap_limit;
    $Super$$__rt_lib_init((unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Base,
                         (unsigned int)&Image$$ARM_LIB_HEAP$$ZI$$Limit);
}

__attribute__((used)) int _mutex_initialize(void *mutex)
{
    (void)mutex;
    return 1;
}

__attribute__((used)) void _mutex_acquire(void *mutex)
{
    (void)mutex;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        vTaskSuspendAll();
}

__attribute__((used)) void _mutex_release(void *mutex)
{
    (void)mutex;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        (void)xTaskResumeAll();
}

__attribute__((used)) void _mutex_free(void *mutex)
{
    (void)mutex;
}

#elif defined(__GNUC__)

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

struct _reent;
extern char __HeapBase;
extern char __HeapLimit;

void *__wrap__sbrk(ptrdiff_t increment)
{
    static uintptr_t heap_end;
    const uintptr_t heap_base = (uintptr_t)&__HeapBase;
    const uintptr_t heap_limit = (uintptr_t)&__HeapLimit;
    uintptr_t previous;

    if (heap_end == 0)
        heap_end = heap_base;
    previous = heap_end;
    if (increment >= 0)
    {
        if ((uintptr_t)increment > heap_limit - heap_end)
            goto no_memory;
    }
    else if ((uintptr_t)(-(increment + 1)) + 1 > heap_end - heap_base)
    {
        goto no_memory;
    }
    heap_end += increment;
    return (void *)previous;

no_memory:
    errno = ENOMEM;
    return (void *)-1;
}

void __malloc_lock(struct _reent *reent)
{
    (void)reent;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        vTaskSuspendAll();
}

void __malloc_unlock(struct _reent *reent)
{
    (void)reent;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        (void)xTaskResumeAll();
}

#endif
