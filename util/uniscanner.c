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

void
scanner_init(universal_scanner_options *opts)
{
    memset(opts, 0, sizeof(universal_scanner_options));
    opts->source = IO_SOURCE_FILE;
}

int
scanner_getnext(universal_scanner_options *opts)
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

    return ((scanner_desc *)opts->opaque)->getnext(opts);
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
