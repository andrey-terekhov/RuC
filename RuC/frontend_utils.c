#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global_vars.h"

/* Занесение ключевых слов в reprtab */
void
read_keywords(ruc_context *context, const char *path)
{
    ruc_context_attach_io(context, path, IO_TYPE_INPUT);

    context->keywordsnum = 1;
    getnext(context);
    nextch(context);
    while (scan(context) != LEOF) // чтение ключевых слов
        ;

    ruc_context_detach_io(context, IO_TYPE_INPUT);
}

/* Вывод таблиц и дерева */
void
output_tables_and_tree(ruc_context *context, const char *path)
{
    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT);

    getnext(context);
    nextch(context);
    context->next = scan(context);

    ext_decl(context); //   генерация дерева

    context->lines[context->line + 1] = context->charnum;
    tablesandtree(context);
    ruc_context_detach_io(context, IO_TYPE_OUTPUT);
}

/* Генерация кодов */
void
output_codes(ruc_context *context, const char *path)
{
    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT);
    codegen(context);
    tablesandcode(context);
    ruc_context_detach_io(context, IO_TYPE_OUTPUT);
}

/* Вывод таблиц в файл */
void
output_export(ruc_context *context, const char *path)
{
    int i;

    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT);
    fprintf(context->output, "%i %i %i %i %i %i %i\n", context->pc,
            context->funcnum, context->id, context->rp, context->md,
            context->maxdisplg, context->wasmain);

    for (i = 0; i < context->pc; i++)
        fprintf(context->output, "%i ", context->mem[i]);
    fprintf(context->output, "\n");

    for (i = 0; i < context->funcnum; i++)
        fprintf(context->output, "%i ", context->functions[i]);
    fprintf(context->output, "\n");

    for (i = 0; i < context->id; i++)
        fprintf(context->output, "%i ", context->identab[i]);
    fprintf(context->output, "\n");

    for (i = 0; i < context->rp; i++)
        fprintf(context->output, "%i ", context->reprtab[i]);

    for (i = 0; i < context->md; i++)
        fprintf(context->output, "%i ", context->modetab[i]);
    fprintf(context->output, "\n");

    ruc_context_detach_io(context, IO_TYPE_OUTPUT);
}
