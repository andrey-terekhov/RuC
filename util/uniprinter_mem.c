//
//  scaner.c
//  RuC
//
//  Created by Andrey Terekhov on 04/08/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//
#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "uniprinter.h"

/** Memory-based printer */
static int
printer_mem_fprintf(universal_printer_options *opts,
                    const char *fmt, va_list args)
{
    char    buf[20];
    char   *buf_to_use = buf;
    char   *allocated = NULL;
    int     ret;

    ret = vsnprintf(buf, sizeof(buf), fmt, args);
    if (ret < 0)
        return ret;

    if ((size_t)ret >= sizeof(buf))
    {
        allocated = malloc(ret);
        ret = vsnprintf(allocated, ret, fmt, args);
        if (ret < 0)
            return ret;
        buf_to_use = allocated;
    }

    if (opts->size >= (opts->pos + ret + 1))
    {
        int new_size = (opts->size * 2) > (opts->pos + ret + 1)
            ? (opts->size * 2)
            : (opts->pos + ret + 1);
        char *reallocated = realloc(opts->ptr, new_size);

        if (reallocated == NULL)
        {
            free(allocated);
            return -1;
        }

        opts->ptr = reallocated;
        opts->size = new_size;
    }

    ret = vsnprintf(buf_to_use, ret, fmt, args);
    if (ret >= 0)
    {
        memcpy(&opts->ptr[opts->pos], buf_to_use, ret);
        opts->pos += ret;
    }

    free(allocated);
    return ret;
}

printer_desc printer_mem = { IO_SOURCE_MEM, printer_mem_fprintf };
