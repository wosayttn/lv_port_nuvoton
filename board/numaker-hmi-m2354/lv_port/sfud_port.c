/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>
#include "drv_spi.h"
#include "sfud_cfg.h"

static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);

static void spi_lock(const sfud_spi *spi)
{
    __disable_irq();
}

static void spi_unlock(const sfud_spi *spi)
{
    __enable_irq();
}

void sfud_demo(uint32_t addr, size_t size, uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;
    extern sfud_flash *sfud_dev;
    const sfud_flash *flash = sfud_get_device(SFUD_W25_DEVICE_INDEX);
    size_t i;
    /* prepare write data */
    for (i = 0; i < size; i++)
    {
        data[i] = i;
    }
    /* erase test */
    result = sfud_erase(flash, addr, size);
    if (result == SFUD_SUCCESS)
    {
        printf("Erase the %s flash data finish. Start from 0x%08X, size is %zu.\r\n", flash->name, addr, size);
    }
    else
    {
        printf("Erase the %s flash data failed.\r\n", flash->name);
        return;
    }
    /* write test */
    result = sfud_write(flash, addr, size, data);
    if (result == SFUD_SUCCESS)
    {
        printf("Write the %s flash data finish. Start from 0x%08X, size is %zu.\r\n", flash->name, addr, size);
    }
    else
    {
        printf("Write the %s flash data failed.\r\n", flash->name);
        return;
    }
    /* read test */
    result = sfud_read(flash, addr, size, data);
    if (result == SFUD_SUCCESS)
    {
        printf("Read the %s flash data success. Start from 0x%08X, size is %zu. The data is:\r\n", flash->name, addr, size);
        printf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (i = 0; i < size; i++)
        {
            if (i % 16 == 0)
            {
                printf("[%08X] ", addr + i);
            }
            printf("%02X ", data[i]);
            if (((i + 1) % 16 == 0) || i == size - 1)
            {
                printf("\r\n");
            }
        }
        printf("\r\n");
    }
    else
    {
        printf("Read the %s flash data failed.\r\n", flash->name);
    }
    /* data check */
    for (i = 0; i < size; i++)
    {
        if (data[i] != i % 256)
        {
            printf("Read and check write data has an error. Write the %s flash data failed.\r\n", flash->name);
            break;
        }
    }
    if (i == size)
    {
        printf("The %s flash test is success.\r\n", flash->name);
    }
}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf, size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;
    SPI_T *nu_spi = (SPI_T *)spi->user_data;

    //printf("[%s] %x tx:%x,%d rx:%x,%d \n", __func__, nu_spi, write_buf, write_size, read_buf, read_size);
    if (nu_spi_send_then_recv(nu_spi, (const void *)write_buf, write_size, read_buf, read_size, 1) != 0)
        result = SFUD_ERR_WRITE;

    return result;
}

/* about 100 microsecond delay */
static void retry_delay_100us(void)
{
    volatile uint32_t delay = 2400;
    while (delay--);
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf, size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;
    SPI_T *spi = (SPI_T *)flash->spi.user_data;

    /**
     * add your port spi bus and device object initialize code like this:
     * 1. rcc initialize
     * 2. gpio initialize
     * 3. spi device initialize
     * 4. flash->spi and flash->retry item initialize
     *    flash->spi.wr = spi_write_read; //Required
     *    flash->spi.qspi_read = qspi_read; //Required when QSPI mode enable
     *    flash->spi.lock = spi_lock;
     *    flash->spi.unlock = spi_unlock;
     *    flash->spi.user_data = &spix;
     *    flash->retry.delay = null;
     *    flash->retry.times = 10000; //Required
     */
    spi = (SPI_T *)flash->spi.user_data;
    if ((spi != (SPI_T *)QSPI0) &&
            (spi != SPI0) &&
            (spi != SPI1) &&
            (spi != SPI2) &&
            (spi != SPI3))
    {
        result = SFUD_ERR_NOT_FOUND;
        goto exit_sfud_spi_port_init;
    }

    /* Open spi_dev */
    SPI_Open(spi, SPI_MASTER, SPI_MODE_0, 8, 48000000);
    /* Disable auto SS function, control SS signal manually. */
    SPI_DisableAutoSS(spi);
    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(spi);
    /* Set CS pin to HIGH */
    SPI_SET_SS_HIGH(spi);
    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(spi);

    /* set the interfaces and data */
    flash->spi.wr = spi_write_read;
#ifdef SFUD_USING_QSPI
    flash->spi.qspi_read = qspi_read;
#endif
    //flash->spi.lock = spi_lock;
    //flash->spi.unlock = spi_unlock;
    /* about 100 microsecond delay */
    flash->retry.delay = retry_delay_100us;
    /* adout 60 seconds timeout */
    flash->retry.times = 60 * 10000;

exit_sfud_spi_port_init:

    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD1]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}
