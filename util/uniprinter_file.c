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

/** File-based printer */
int
printer_file_fprintf(universal_printer_options *opts,
                     const char *               fmt,
                     va_list                    args)
{
    int ret;

    ret = vfprintf(opts->output, fmt, args);

    return ret;
}

printer_desc printer_file = { IO_SOURCE_FILE, printer_file_fprintf };
