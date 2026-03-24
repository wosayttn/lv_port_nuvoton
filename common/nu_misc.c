#include <stdint.h>
#include "nu_misc.h"
#include <math.h>

#if defined(__FREERTOS__)
    #include "FreeRTOS.h"
#else
    #include <stdlib.h>
#endif

void *nvt_malloc_align(uint32_t size, uint32_t align)
{
    void *ptr;
    uint32_t align_size;

    align = NVT_ALIGN(align, sizeof(void *));

    align_size = NVT_ALIGN(size, sizeof(void *)) + align;
#if defined(__FREERTOS__)
    if ((ptr = pvPortMalloc(align_size)) != NULL)
#else
    if ((ptr = malloc(align_size)) != NULL)
#endif
    {
        void *align_ptr;

        if (((uintptr_t)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((uintptr_t)ptr + align);
        }
        else
        {
            align_ptr = (void *)NVT_ALIGN((uintptr_t)ptr, align);
        }

        *((uint32_t *)((uintptr_t)align_ptr - sizeof(void *))) = (uintptr_t)ptr;

        ptr = align_ptr;
    }


    return ptr;
}

void nvt_free_align(void *ptr)
{
    if (ptr == NULL) return;

#if defined(__FREERTOS__)
    vPortFree((void *) * ((uintptr_t *)((uintptr_t)ptr - sizeof(void *))));
#else
    free((void *) * ((uintptr_t *)((uintptr_t)ptr - sizeof(void *))));
#endif

}

uint32_t ns_to_cycles_ceil(double ns, double clk_hz)
{
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    /* ceil(ns * clk_hz / 1e9) */
    double cycles = (ns * clk_hz) / 1e9;
    return (uint32_t)ceil(cycles);
}

uint32_t hz_to_ns_ceil(double hz)
{
    if (hz <= 0.0)
        return 0;

    /* ceil(1e9 / hz) */
    double ns = 1e9 / hz;
    return (uint32_t)ceil(ns);
}

double ns_to_cycles(double ns, double clk_hz)
{
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    return (ns * clk_hz) / 1e9;
}

double hz_to_ns(double hz)
{
    if (hz <= 0.0)
        return 0;

    return 1e9 / hz;;
}

