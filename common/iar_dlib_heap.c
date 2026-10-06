/* Give IAR DLIB the SRAM remaining after the linker-placed data/BSS. */
#include <stddef.h>
#include <stdint.h>

/* The zero-sized HEAP block is placed last in its RAM region by the ICF.
 * DLIB requires the chunk's first word to contain its byte size. */
#pragma section="HEAP"
extern char const __iar_heap_region_end;

#define HEAP_ALIGNMENT 16U

void *__data_GetMemChunk(void)
{
    static unsigned char issued;
    uintptr_t start;
    uintptr_t limit;

    if (issued)
        return NULL;
    issued = 1;

        start = ((uintptr_t)__section_begin("HEAP") + (HEAP_ALIGNMENT - 1U)) &
            ~(uintptr_t)(HEAP_ALIGNMENT - 1U);
        limit = (uintptr_t)&__iar_heap_region_end & ~(uintptr_t)(HEAP_ALIGNMENT - 1U);
        if (start >= limit || limit - start < HEAP_ALIGNMENT)
        return NULL;

    *(size_t *)start = (size_t)(limit - start);
    return (void *)start;
}
