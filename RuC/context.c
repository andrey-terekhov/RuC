#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "global_vars.h"

void
ruc_context_init(ruc_context *context)
{
    memset(context, 0, sizeof(ruc_context));
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

/**
 * Get FILE for a specific IO type
 */
static FILE **
io_type2file(ruc_context *context, ruc_io_type type)
{
    switch (type)
    {
        case IO_TYPE_INPUT:
            return &context->input;
        case IO_TYPE_OUTPUT:
            return &context->output;
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
    switch (type)
    {
        case IO_TYPE_OUTPUT:
            return "wt";
        case IO_TYPE_INPUT:
        default:
            return "r";
    }
}

/* See description in context.h */
void
ruc_context_attach_io(ruc_context *context, const char *path, ruc_io_type type)
{
    FILE * f = fopen(path, io_type2access_mask(type));
    FILE **target = io_type2file(context, type);

    if (f == NULL)
    {
        if (type == IO_TYPE_OUTPUT)
            printf(" ошибка открытия файла %s: %s\n", path, strerror(errno));
        else
            printf(" не найден файл %s\n", path);
        exit(1);
    }

    *target = f;
}

/* See description in context.h */
void
ruc_context_detach_io(ruc_context *context, ruc_io_type type)
{
    FILE **f = io_type2file(context, type);

    if (*f != NULL)
        fclose(*f);
    *f = NULL;
}
