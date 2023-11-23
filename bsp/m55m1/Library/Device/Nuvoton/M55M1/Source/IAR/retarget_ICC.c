/**************************************************************************//**
 * @file     retarget_ICC.c
 * @version  V1.00
 * @brief    Debug Port and Semihost Setting Source File for ICC
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <LowLevelIOInterface.h>


size_t __write(int handle, const unsigned char *buffer, size_t size)
{
    /* Remove the #if #endif pair to enable the implementation */

    size_t nChars = 0;

    if (buffer == 0)
    {
        /*
         * This means that we should flush internal buffers.  Since we
         * don't we just return.  (Remember, "handle" == -1 means that all
         * handles should be flushed.)
         */
        return 0;
    }

    /* This template only writes to "standard out" and "standard err",
     * for all other file handles it returns failure. */
    if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR)
    {
        return _LLIO_ERROR;
    }

    for (/* Empty */; size != 0; --size)
    {
        SendChar(*buffer++);
        ++nChars;
    }

    return nChars;
}

size_t __read(int handle, unsigned char *buffer, size_t size)
{
    /* Remove the #if #endif pair to enable the implementation */
    int nChars = 0;

    /* This template only reads from "standard in", for all other file
     * handles it returns failure. */
    if (handle != _LLIO_STDIN)
    {
        return _LLIO_ERROR;
    }

    for (/* Empty */; size > 0; --size)
    {
        int c = GetChar();

        if (c < 0)
            break;

#if (STDIN_ECHO != 0)
        SendChar(c);
#endif

        *buffer++ = c;
        ++nChars;
    }

    return nChars;
}

long __lseek(int handle, long offset, int whence)
{
    return -1;
}
