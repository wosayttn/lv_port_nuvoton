/* newlib-nano system heap support for GCC/FreeRTOS projects. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <reent.h>

#include "FreeRTOS.h"
#include "task.h"

extern char __end__;
extern char __HeapLimit;

/* newlib calls _sbrk() with the malloc lock held. Keep its break in .bss. */
void *_sbrk(ptrdiff_t increment)
{
    static char *heap_end;
    char *previous;

    if (heap_end == NULL)
        heap_end = &__end__;

    previous = heap_end;
    if (increment >= 0)
    {
        if ((size_t)increment > (size_t)(&__HeapLimit - heap_end))
            goto no_memory;
    }
    else if ((size_t)(-(increment + 1)) + 1 > (size_t)(heap_end - &__end__))
    {
        goto no_memory;
    }

    heap_end += increment;
    return previous;

no_memory:
    errno = ENOMEM;
    return (void *)-1;
}

/* These are newlib hooks, not the Arm C library's _mutex_* hooks. FreeRTOS
 * scheduler suspension nests, including when heap_3.c calls malloc().
 * Allocation from an ISR is unsupported by newlib and FreeRTOS heap_3.c. */
static uint32_t is_thread_mode(void)
{
    uint32_t ipsr;
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return ipsr == 0U;
}

void __malloc_lock(struct _reent *reent)
{
    (void)reent;
    if (is_thread_mode() && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        vTaskSuspendAll();
}

void __malloc_unlock(struct _reent *reent)
{
    (void)reent;
    if (is_thread_mode() && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        (void)xTaskResumeAll();
}