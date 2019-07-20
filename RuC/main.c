//  RuC
//
//  Created by Andrey Terekhov on 24/Apr/16.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
// http://www.lysator.liu.se/c/ANSI-C-grammar-y.html

#define _CRT_SECURE_NO_WARNINGS

const char *name =
    //"tests/Egor/string/strcat.c";
    "tests/Fadeev/draw.c";
//"tests/arrstruct1.c";
//"../../../tests/mips/0test.c";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "Defs.h"
#include "context.h"

extern void
preprocess_file(ruc_context *context);

extern void
tablesandcode(ruc_context *context);
extern void
tablesandtree(ruc_context *context);
extern void
import(ruc_context *context);
extern int
getnext(ruc_context *context);
extern int
nextch(ruc_context *context);
extern int
scan(ruc_context *context);
extern void
error(ruc_context *context, int ernum);
extern void
codegen(ruc_context *context);
extern void
mipsopt(ruc_context *context);
extern void
mipsgen(ruc_context *context);
extern void
ext_decl(ruc_context *context);

int
toreprtab(ruc_context *context, char str[])
{
    int i, oldrepr = context->rp;
    context->hash = 0;
    context->rp += 2;
    for (i = 0; str[i] != 0; i++)
    {
        context->hash += str[i];
        context->reprtab[context->rp++] = str[i];
    }
    context->hash &= 255;
    context->reprtab[context->rp++] = 0;
    context->reprtab[oldrepr] = context->hashtab[context->hash];
    context->reprtab[oldrepr + 1] = 1;
    return context->hashtab[context->hash] = oldrepr;
}

/* Первичная инициализация глобальных объектов */
void
init(ruc_context *context)
{
    memset(context->hashtab, 0, sizeof(context->hashtab));
}

/* Инициализация modetab */
void
init_modetab(ruc_context *context)
{
    // занесение в modetab описателя struct {int numTh; int inf; }
    context->modetab[1] = 0;
    context->modetab[2] = MSTRUCT;
    context->modetab[3] = 2;
    context->modetab[4] = 4;
    context->modetab[5] = context->modetab[7] = LINT;
    context->modetab[6] = toreprtab(context, "numTh");
    context->modetab[8] = toreprtab(context, "data");

    // занесение в modetab описателя функции void t_msg_send(struct msg_info m)
    context->modetab[9] = 1;
    context->modetab[10] = MFUNCTION;
    context->modetab[11] = LVOID;
    context->modetab[12] = 1;
    context->modetab[13] = 2;

    // занесение в modetab описателя функции void* interpreter(void* n)
    context->modetab[14] = 9;
    context->modetab[15] = MFUNCTION;
    context->modetab[16] = LVOIDASTER;
    context->modetab[17] = 1;
    context->modetab[18] = LVOIDASTER;
    context->modetab[19] = context->startmode = 14;
    context->md = 19;
    context->keywordsnum = 0;
    context->lines[context->line = 1] = 1;
    context->charnum = 1;
    context->kw = 1;
    context->tc = 0;
}

typedef enum ruc_io_type
{
    IO_TYPE_INPUT,
    IO_TYPE_OUTPUT,
} ruc_io_type;

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

void
attach_io(ruc_context *context, const char *path, ruc_io_type type)
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

void
detach_io(ruc_context *context, ruc_io_type type)
{
    FILE **f = io_type2file(context, type);

    if (*f != NULL)
        fclose(*f);
    *f = NULL;
}


/* Занесение ключевых слов в reprtab */
void
read_keywords(ruc_context *context, const char *path)
{
    attach_io(context, path, IO_TYPE_INPUT);

    context->keywordsnum = 1;
    getnext(context);
    nextch(context);
    while (scan(context) != LEOF) // чтение ключевых слов
        ;

    detach_io(context, IO_TYPE_INPUT);
}

/* Вывод таблиц и дерева */
void
output_tables_and_tree(ruc_context *context, const char *path)
{
    attach_io(context, path, IO_TYPE_OUTPUT);

    getnext(context);
    nextch(context);
    context->next = scan(context);

    ext_decl(context); //   генерация дерева

    context->lines[context->line + 1] = context->charnum;
    tablesandtree(context);
    detach_io(context, IO_TYPE_OUTPUT);
}

/* Генерация кодов */
void
output_codes(ruc_context *context, const char *path)
{
    attach_io(context, path, IO_TYPE_OUTPUT);
    codegen(context);
    tablesandcode(context);
    detach_io(context, IO_TYPE_OUTPUT);
}

/* Вывод таблиц в файл */
void
output_export(ruc_context *context, const char *path)
{
    int i;

    attach_io(context, path, IO_TYPE_OUTPUT);
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

    detach_io(context, IO_TYPE_OUTPUT);
}

int
main(int argc, const char *argv[])
{
    ruc_context context;

    ruc_context_init(&context);

    init(&context);
    read_keywords(&context, "keywords.txt");

    // Открытие исходного текста
    attach_io(&context, argc > 1 ? argv[1] : name, IO_TYPE_INPUT);

    // Препроцессинг в файл macro.txt
    attach_io(&context, "macro.txt", IO_TYPE_OUTPUT);
    init_modetab(&context);

    printf("\nИсходный текст:\n \n");

    preprocess_file(&context); //   макрогенерация

    detach_io(&context, IO_TYPE_OUTPUT);
    detach_io(&context, IO_TYPE_INPUT);

    attach_io(&context, "macro.txt", IO_TYPE_INPUT);
    output_tables_and_tree(&context, "tree.txt");
    output_codes(&context, "codes.txt");
    output_export(&context, "export.txt");
    detach_io(&context, IO_TYPE_INPUT);

    return 0;
}
