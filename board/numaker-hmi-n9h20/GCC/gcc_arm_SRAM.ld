#if defined(N9H20K5_32MB)
#define MEMSIZE 0x02000000
#elif defined(N9H20K3_8MB)
#define MEMSIZE 0x00800000
#elif defined(N9H20K1_2MB)
#define MEMSIZE 0x00200000
#else
#error Unsupported N9H20 RAM configuration
#endif

#define STACK_RESERVE 0x0000A400
#include "../../../common/arm9_heap.ld"
