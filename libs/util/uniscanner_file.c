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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "uniscanner.h"

int
io_file_getnext(universal_scanner_options *opts)
{
    // reads UTF-8
    unsigned char firstchar, secondchar;
    int           ret = EOF;
    int           retval;

    retval = fscanf(opts->input, "%c", &firstchar);
    if (retval == EOF)
    {
        return EOF;
    }
    else if (retval == 1)
    {
        if ((firstchar & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0)
        {
            if (fscanf(opts->input, "%c", &secondchar) != 1)
            {
                return EOF;
            }

            ret = ((int)(firstchar & /*0b11111*/ 0x1F)) << 6 |
                (secondchar & /*0b111111*/ 0x3F);
        }
        else
        {
            ret = firstchar;
        }
        if (ret == '\r')
            ret = io_file_getnext(opts);
    }

    return ret;
}

int
io_file_scanf(universal_scanner_options *opts, const char *fmt, va_list args)
{
    return vfscanf(opts->input, fmt, args);
}

scanner_desc scanner_file = { IO_SOURCE_FILE, io_file_getnext, io_file_scanf };
