#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"
#include "keywords.h"
#include "scanner.h"

/* Занесение ключевых слов в reprtab */
void
read_keywords(ruc_context *context, const char *path)
{
    char *keywords = malloc(keywords_txt_len + 1);
    if (keywords == NULL)
        exit(-1);

    /* Add null symbol to keywords */
    memcpy(keywords, keywords_txt, keywords_txt_len);
    keywords[keywords_txt_len] = '\0';

    ruc_context_attach_io(context, keywords, IO_TYPE_INPUT, IO_SOURCE_MEM);

    context->keywordsnum = 1;
    getnext(context);
    nextch(context);
    while (scan(context) != LEOF) // чтение ключевых слов
    {
        ;
    }

    ruc_context_detach_io(context, IO_TYPE_INPUT);
    free(keywords);
}

/* Вывод таблиц и дерева */
void
output_tables_and_tree(ruc_context *context, const char *path)
{
    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);

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
    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);
    codegen(context);
    tablesandcode(context);
    ruc_context_detach_io(context, IO_TYPE_OUTPUT);
}

/* Вывод таблиц в файл */
void
output_export(ruc_context *context, const char *path)
{
    int i;

    ruc_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);
    printer_printf(&context->output_options, "%i %i %i %i %i %i %i\n",
                   context->pc, context->funcnum, context->id, context->rp,
                   context->md, context->maxdisplg, context->wasmain);

    for (i = 0; i < context->pc; i++)
        printer_printf(&context->output_options, "%i ", context->mem[i]);
    printer_printf(&context->output_options, "\n");

    for (i = 0; i < context->funcnum; i++)
        printer_printf(&context->output_options, "%i ", context->functions[i]);
    printer_printf(&context->output_options, "\n");

    for (i = 0; i < context->id; i++)
        printer_printf(&context->output_options, "%i ", context->identab[i]);
    printer_printf(&context->output_options, "\n");

    for (i = 0; i < context->rp; i++)
        printer_printf(&context->output_options, "%i ", context->reprtab[i]);

    for (i = 0; i < context->md; i++)
        printer_printf(&context->output_options, "%i ", context->modetab[i]);
    printer_printf(&context->output_options, "\n");

    ruc_context_detach_io(context, IO_TYPE_OUTPUT);
}
