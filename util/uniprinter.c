#define _CRT_SECURE_NO_WARNINGS
#include "uniprinter.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern printer_desc printer_mem;
extern printer_desc printer_file;

printer_desc *printers[] = { &printer_file, &printer_mem, NULL };

/* See description in uniprinter.h */
void
printer_init(universal_printer_options *opts)
{
    memset(opts, 0, sizeof(universal_printer_options));
    opts->source = IO_SOURCE_FILE;
}

/* See description in uniprinter.h */
void
printer_deinit(universal_printer_options *opts)
{
    printer_close(opts);
}

/* See description in uniprinter.h */
void
printer_close(universal_printer_options *opts)
{
    if (opts->output != NULL)
    {
        fclose(opts->output);
        opts->output = NULL;
    }

    free(opts->ptr);
    opts->ptr = NULL;

    opts->pos = 0;
    opts->size = 0;
    opts->opaque = NULL;
}

/* See description in uniprinter.h */
int
printer_printf(universal_printer_options *opts, const char *fmt, ...)
{
    int     ret;
    va_list args;
    va_start(args, fmt);

    if (opts->opaque == NULL)
    {
        printer_desc **tmp = printers;

        while (*tmp != NULL && (*tmp)->source != opts->source)
            tmp++;

        if (*tmp == NULL)
        {
            fprintf(stderr, " failed to find a suitable printer\n");
            exit(1);
        }

        opts->opaque = *tmp;
    }

    ret = ((printer_desc *)opts->opaque)->printf(opts, fmt, args);
    va_end(args);

    return ret;
}

/* See description in uniprinter.h */
int
printer_printchar(universal_printer_options *opts, int wchar)
{
    if (wchar < 128)
    {
        return printer_printf(opts, "%c", wchar);
    }
    else
    {
        unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
        unsigned char second =
            (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

        return printer_printf(opts, "%c%c", first, second);
    }
}

/* See description in uniprinter.h */
bool
printer_attach_file(universal_printer_options *opts, FILE *f)
{
    opts->source = IO_SOURCE_FILE;
    opts->output = f;
    opts->ptr = NULL;
    opts->pos = 0;
    opts->size = 0;
    opts->opaque = NULL;
    return true;
}

/* See description in uniprinter.h */
bool
printer_attach_buffer(universal_printer_options *opts, size_t size)
{
    opts->source = IO_SOURCE_MEM;
    opts->output = NULL;
    opts->ptr = malloc(size);
    memset(opts->ptr, 0, size);
    opts->pos = 0;
    opts->size = size;
    opts->opaque = NULL;
    return opts->ptr != NULL;
}
