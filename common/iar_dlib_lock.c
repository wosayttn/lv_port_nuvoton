/* IAR DLIB locks for FreeRTOS-backed malloc and standard I/O. */
#include "NuMicro.h"
#include "FreeRTOS.h"
#include "task.h"
#include <DLib_Threads.h>

static int scheduler_can_lock(void)
{
    return (__get_IPSR() == 0U) &&
           (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED);
}

void __iar_system_Mtxinit(__iar_Rmtx *mutex)
{
    *mutex = (void *)1;
}

void __iar_system_Mtxdst(__iar_Rmtx *mutex)
{
    *mutex = 0;
}

void __iar_system_Mtxlock(__iar_Rmtx *mutex)
{
    (void)mutex;
    if (scheduler_can_lock())
        vTaskSuspendAll();
}

void __iar_system_Mtxunlock(__iar_Rmtx *mutex)
{
    (void)mutex;
    if (scheduler_can_lock())
        (void)xTaskResumeAll();
}

void __iar_file_Mtxinit(__iar_Rmtx *mutex)
{
    __iar_system_Mtxinit(mutex);
}

void __iar_file_Mtxdst(__iar_Rmtx *mutex)
{
    __iar_system_Mtxdst(mutex);
}

void __iar_file_Mtxlock(__iar_Rmtx *mutex)
{
    __iar_system_Mtxlock(mutex);
}

void __iar_file_Mtxunlock(__iar_Rmtx *mutex)
{
    __iar_system_Mtxunlock(mutex);
}
