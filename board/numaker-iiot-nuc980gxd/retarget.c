#include <stdio.h>

#include "nuc980.h"
#include "sys.h"

int kbhit(void)
{
    return !((inpw(REG_UART0_FSR) & (1 << 14)) == 0);
}

#if defined(__CC_ARM)

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)
#pragma import(__use_two_region_memory)

int sendchar(int ch)
{
    while (inpw(REG_UART0_FSR) & (1 << 23));
    outpw(REG_UART0_THR, ch);
    if (ch == '\n')
    {
        while (inpw(REG_UART0_FSR) & (1 << 23));
        outpw(REG_UART0_THR, '\r');
    }
    return ch;
}

int recvchar(void)
{
    while (inpw(REG_UART0_FSR) & (1 << 14));
    return inpw(REG_UART0_RBR);
}

struct __FILE
{
    int handle;
};

FILE __stdout;
FILE __stdin;
FILE __stderr;

int fflush(FILE *stream)
{
    (void)stream;
    while (inpw(REG_UART0_FSR) & (1 << 23));
    return 0;
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    return sendchar(ch);
}

int fgetc(FILE *stream)
{
    (void)stream;
    return recvchar();
}

int fclose(FILE *stream)
{
    (void)stream;
    return 0;
}

int fseek(FILE *stream, long offset, int origin)
{
    (void)stream;
    (void)offset;
    (void)origin;
    return 0;
}

int ferror(FILE *stream)
{
    (void)stream;
    return EOF;
}

void _ttywrch(int ch)
{
    (void)sendchar(ch);
}

void _sys_exit(int return_code)
{
    (void)return_code;
    for (;;);
}

#elif defined(__GNUC__)

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

int _fstat(int descriptor, struct stat *status)
{
    if (descriptor < 0 || descriptor > 2)
    {
        errno = EBADF;
        return -1;
    }
    memset(status, 0, sizeof(*status));
    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int descriptor)
{
    if (descriptor < 0 || descriptor > 2)
    {
        errno = EBADF;
        return 0;
    }
    return 1;
}

int _write(int fd, char *buffer, int length)
{
    int remaining = length;

    (void)fd;
    while (remaining--)
    {
        while (inpw(REG_UART0_FSR) & (1 << 23));
        outpw(REG_UART0_THR, *buffer);
        if (*buffer == '\n')
        {
            while (inpw(REG_UART0_FSR) & (1 << 23));
            outpw(REG_UART0_THR, '\r');
        }
        buffer++;
    }
    return length;
}

int _read(int fd, char *buffer, int length)
{
    (void)fd;
    (void)length;
    while (inpw(REG_UART0_FSR) & (1 << 14));
    *buffer = inpw(REG_UART0_RBR);
    return 1;
}

#endif
