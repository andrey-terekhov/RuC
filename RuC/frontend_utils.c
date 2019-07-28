#if defined(__APPLE__) || defined(__linux__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"
#include "keywords.h"
#include "scanner.h"

/* Make executable actually executable on best-effort basis (if possible) */
static void
make_executable(const char *path)
{
    struct stat stat_buf;

#if defined(__APPLE__) || defined(__linux__)
    if (stat(path, &stat_buf) != 0)
        return;

    chmod(path, stat_buf.st_mode | S_IXUSR);
#endif
}

/* Занесение ключевых слов в reprtab */
void
read_keywords(compiler_context *context)
{
    char *keywords = malloc(keywords_txt_len + 1);
    if (keywords == NULL)
        exit(-1);

    /* Add null symbol to keywords */
    memcpy(keywords, keywords_txt, keywords_txt_len);
    keywords[keywords_txt_len] = '\0';

    compiler_context_attach_io(context, keywords, IO_TYPE_INPUT, IO_SOURCE_MEM);

    context->keywordsnum = 1;
    getnext(context);
    nextch(context);
    while (scan(context) != LEOF) // чтение ключевых слов
    {
        ;
    }

    compiler_context_detach_io(context, IO_TYPE_INPUT);
}

/* Вывод таблиц и дерева */
void
output_tables_and_tree(compiler_context *context, const char *path)
{
    compiler_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);

    getnext(context);
    nextch(context);
    context->next = scan(context);

    ext_decl(context); //   генерация дерева

    context->lines[context->line + 1] = context->charnum;
    tablesandtree(context);
    compiler_context_detach_io(context, IO_TYPE_OUTPUT);
}

/* Генерация кодов */
void
output_codes(compiler_context *context, const char *path)
{
    compiler_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);
    codegen(context);
    tablesandcode(context);
    compiler_context_detach_io(context, IO_TYPE_OUTPUT);
}

/* Вывод таблиц в файл */
void
output_export(compiler_context *context, const char *path)
{
    int i;

    compiler_context_attach_io(context, path, IO_TYPE_OUTPUT, IO_SOURCE_FILE);
    printer_printf(&context->output_options, "#!/usr/bin/ruc-vm\n");

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

    compiler_context_detach_io(context, IO_TYPE_OUTPUT);

    make_executable(path);
}
