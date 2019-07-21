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

scanner_desc scanner_file = { IO_SOURCE_FILE, io_file_getnext };
