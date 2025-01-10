#ifndef __TC_H__
#define __TC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_BASE_ADDRESS               (SPIM_DMM0_SADDR)   //0x20110000   //SPIM_DMM0_SADDR
#define SPIM_HYPERRAM_SIZE                (8*1024*1024)

#if 1
#define CONFIG_BATCH_SIZE_START       (1)
#define CONFIG_BATCH_SIZE_STOP        (4*1024) //(8)
#define CONFIG_BATCH_SIZE_STEP        (1)
#else
#define CONFIG_BATCH_SIZE_START       (2048)
#define CONFIG_BATCH_SIZE_STOP        (8192) //(8)
#define CONFIG_BATCH_SIZE_STEP        (2048)
#endif

#if (_DEBUG==1)
#define TC_PRINTF     printf
#else
#define TC_PRINTF(...)    do {   } while(0);
#endif

/**
 * tc_export
 *
 * @brief testcase data structure.
 *        Will export the data to `UtestTcTab` section in flash.
 *
 * @member name        Testcase name.
 * @member run_timeout Testcase maximum test time (Time unit: seconds).
 * @member init        Necessary initialization before executing the test case function.
 * @member tc          Total number of tests failed.
 * @member cleanup     Total number of tests failed.
 *
*/
struct tc_export
{
    const char  *name;
    int (*tc_init)(void);
    void (*tc_exec)(void);
    int (*tc_cleanup)(void);
};
typedef struct tc_export *tc_export_t;

/**
 * TC_EXPORT
 *
 * @brief Export testcase function to `TcTab` section in flash.
 *        Used in application layer.
 *
 * @param testcase The testcase function.
 * @param name     The testcase name.
 * @param init     The initialization function of the test case.
 * @param cleanup  The cleanup function of the test case.
 * @param timeout  Testcase maximum test time (Time unit: seconds).
 *
 * @return None
 *
*/
#define TC_EXPORT(testcase, name, init, cleanup)                \
    __attribute__((used)) static const struct tc_export _tc_testcase           \
    __attribute__((section("TcTab"))) =                                        \
    {                                                                          \
        name,                                                                  \
        init,                                                                  \
        testcase,                                                              \
        cleanup                                                                \
    }

void tc_list(void);
int tc_run(void);
void tc_prepare(uint32_t u32BaseAddr, int i32BatchSize);
int tc_compare(uint32_t u32BaseAddr, int i32BatchSize);

#ifdef __cplusplus
}
#endif

#endif /* __TC_H__*/
