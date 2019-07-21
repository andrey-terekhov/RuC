//
//  util.c
//
//  Created by Mikhail Terekhov on 03/09/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//

#include "util.h"

void _obsolete_printf_char(int wchar)
{
    if (wchar<128)
        printf("%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

        printf("%c%c", first, second);
    }
}

void _obsolete_fprintf_char(FILE *f, int wchar)
{    if (wchar<128)
    fprintf(f, "%c", wchar);
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

        fprintf(f, "%c%c", first, second);
    }
}

int _obsolete_getf_char()
{
    // reads UTF-8
    
    unsigned char firstchar, secondchar;
    
    if (scanf(" %c", &firstchar) == EOF)
        return EOF;
    else
        if ((firstchar & /*0b11100000*/0xE0) == /*0b11000000*/0xC0)
        {
            scanf("%c", &secondchar);
            return ((int)(firstchar & /*0b11111*/0x1F)) << 6 | (secondchar & /*0b111111*/0x3F);
        }
        else
            return firstchar;
}

