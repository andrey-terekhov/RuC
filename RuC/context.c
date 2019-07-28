#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "global_vars.h"

#define DEFAULT_OUTBUF_SIZE (1024)

/* See description in context.h */
void
compiler_context_init(compiler_context *context)
{
    memset(context, 0, sizeof(compiler_context));
    scanner_init(&context->input_options);
    printer_init(&context->output_options);
    printer_init(&context->err_options);
    printer_init(&context->miscout_options);
    context->charnum = 1;
    context->m_charnum = 1;
    context->rp = 1;
    context->id = 2;
    context->md = 1;
    context->startmode = 1;
    context->sopnd = -1;
    context->curid = 2;
    context->lg = -1;
    context->displ = -3;
    context->maxdispl = 3;
    context->maxdisplg = 3;
    context->procd = 1;
    context->pc = 4;
    context->funcnum = 2;
    context->blockflag = 1;
    context->notrobot = 1;
    context->prdf = -1;
    context->leftansttype = -1;

    context->fip = 1;
    context->mfp = 1;
    context->mfirstrp = -1;
    context->mlastrp = -1;
    context->mp = 3;
}

/* See description in context.h */
void
compiler_context_deinit(compiler_context *context)
{
    scanner_deinit(&context->input_options);
    printer_deinit(&context->output_options);
    printer_deinit(&context->err_options);
    printer_deinit(&context->miscout_options);
}

/** Is I/O an actual output? */
static bool
io_type_is_output(ruc_io_type type)
{
    switch (type)
    {
        case IO_TYPE_INPUT:
            return false;
        default:
            return true;
    }
}

/** Get type-specific options */
static void *
io_type2opts(compiler_context *context, ruc_io_type type)
{
    switch (type)
    {
        case IO_TYPE_INPUT:
            return &context->input_options;
        case IO_TYPE_OUTPUT:
            return &context->output_options;
        case IO_TYPE_ERROR:
            return &context->err_options;
        case IO_TYPE_MISC:
            return &context->miscout_options;
        default:
            return NULL;
    }
}

/**
 * Get access mask for a specific IO type
 */
static const char *
io_type2access_mask(ruc_io_type type)
{
    return io_type_is_output(type) ? "wt" : "r";
}

/**
 * Open file with specific mask taking standard files into account
 */
static FILE *
io_get_file(const char *ptr, const char *mask)
{
    if (strcmp(ptr, ":stderr") == 0)
        return stderr;
    else if (strcmp(ptr, ":stdout") == 0)
        return stdout;
    else if (strcmp(ptr, ":stdin") == 0)
        return stdin;
    return fopen(ptr, mask);
}

/* See description in context.h */
void
compiler_context_attach_io(compiler_context *context,
                           const char *      ptr,
                           ruc_io_type       type,
                           ruc_io_source     source)
{
    void *opts = io_type2opts(context, type);

    if (source == IO_SOURCE_FILE)
    {
        FILE *f = io_get_file(ptr, io_type2access_mask(type));

        if (f == NULL)
        {
            if (io_type_is_output(type))
            {
                printf(" ошибка открытия файла %s: %s\n", ptr, strerror(errno));
            }
            else
            {
                printf(" не найден файл %s\n", ptr);
            }
            exit(1);
        }

        if (io_type_is_output(type))
            printer_attach_file(opts, f);
        else
            scanner_attach_file(opts, f);
    }
    else
    {
        if (io_type_is_output(type))
            printer_attach_buffer(opts, DEFAULT_OUTBUF_SIZE);
        else
            scanner_attach_buffer(opts, ptr);
    }
}

/* See description in context.h */
void
compiler_context_detach_io(compiler_context *context, ruc_io_type type)
{
    void *opts = io_type2opts(context, type);

    if (io_type_is_output(type))
    {
        printer_close(opts);
    }
    else
    {
        scanner_close(opts);
    }
}
