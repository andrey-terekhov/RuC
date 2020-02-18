/*
 *	Copyright 2019 Andrey Terekhov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */
#define _CRT_SECURE_NO_WARNINGS
#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern scanner_desc scanner_mem;
extern scanner_desc scanner_file;

scanner_desc *scanners[] = { &scanner_file, &scanner_mem, NULL };

/* See description in uniscanner.h */
void
scanner_init(universal_scanner_options *opts)
{
    memset(opts, 0, sizeof(universal_scanner_options));
    opts->source = IO_SOURCE_FILE;
}

/* See description in uniscanner.h */
void
scanner_deinit(universal_scanner_options *opts)
{
    scanner_close(opts);
}

/* See description in uniscanner.h */
void
scanner_close(universal_scanner_options *opts)
{
    if (opts->input != NULL)
    {
        fclose(opts->input);
        opts->input = NULL;
    }

    if (opts->ptr != NULL)
    {
        free(opts->ptr);
        opts->ptr = NULL;
    }

    opts->pos = 0;
    opts->opaque = NULL;
}

/* See description in uniscanner.h */
static void
scanner_prepare(universal_scanner_options *opts)
{
    if (opts->opaque == NULL)
    {
        scanner_desc **tmp = scanners;

        while (*tmp != NULL && (*tmp)->source != opts->source)
            tmp++;

        if (*tmp == NULL)
        {
            fprintf(stderr, " failed to find a suitable scanner\n");
            exit(1);
        }

        opts->opaque = *tmp;
    }
}

/* See description in uniscanner.h */
int
scanner_getnext(universal_scanner_options *opts)
{
    scanner_prepare(opts);
    return ((scanner_desc *)opts->opaque)->getnext(opts);
}

/* See description in uniscanner.h */
int
scanner_scanf(universal_scanner_options *opts, const char *fmt, ...)
{
    int     ret;
    va_list args;

    va_start(args, fmt);

    scanner_prepare(opts);
    ret = ((scanner_desc *)opts->opaque)->scanf(opts, fmt, args);
    va_end(args);

    return ret;
}

/* See description in uniscanner.h */
bool
scanner_attach_file(universal_scanner_options *opts, FILE *f)
{
    opts->source = IO_SOURCE_FILE;
    opts->input = f;
    opts->ptr = NULL;
    opts->pos = 0;
    opts->opaque = NULL;
    return true;
}

/* See description in uniscanner.h */
bool
scanner_attach_buffer(universal_scanner_options *opts, const char *ptr)
{
    opts->source = IO_SOURCE_MEM;
    opts->input = NULL;
    opts->ptr = (char *)ptr;
    opts->pos = 0;
    opts->opaque = NULL;
    return opts->ptr != NULL;
}
