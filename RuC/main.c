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

#include "frontend_utils.h"
#include "tables.h"

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
main(int argc, const char *argv[])
{
    ruc_context context;

    ruc_context_init(&context);

    read_keywords(&context, "keywords.txt");

    // Открытие исходного текста
    ruc_context_attach_io(&context, argc > 1 ? argv[1] : name, IO_TYPE_INPUT);

    // Препроцессинг в файл macro.txt
    ruc_context_attach_io(&context, "macro.txt", IO_TYPE_OUTPUT);
    init_modetab(&context);

    printf("\nИсходный текст:\n \n");

    preprocess_file(&context); //   макрогенерация

    ruc_context_detach_io(&context, IO_TYPE_OUTPUT);
    ruc_context_detach_io(&context, IO_TYPE_INPUT);

    ruc_context_attach_io(&context, "macro.txt", IO_TYPE_INPUT);
    output_tables_and_tree(&context, "tree.txt");
    output_codes(&context, "codes.txt");
    output_export(&context, "export.txt");
    ruc_context_detach_io(&context, IO_TYPE_INPUT);

    return 0;
}
