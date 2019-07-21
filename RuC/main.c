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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include "Defs.h"
#include "context.h"

#include "frontend_utils.h"
#include "tables.h"

extern void preprocess_file(ruc_context *context);

extern void tablesandcode(ruc_context *context);
extern void tablesandtree(ruc_context *context);
extern void import(ruc_context *context);
extern int  getnext(ruc_context *context);
extern int  nextch(ruc_context *context);
extern int  scan(ruc_context *context);
extern void error(ruc_context *context, int ernum);
extern void codegen(ruc_context *context);
extern void mipsopt(ruc_context *context);
extern void mipsgen(ruc_context *context);
extern void ext_decl(ruc_context *context);

//#define FILE_DEBUG

static void
process_user_requests(ruc_context *context, int argc, const char *argv[])
{
    int  i;
    bool enough_files = false;

    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            if ((i + 1) >= argc)
                fprintf(stderr, " не указан выходной файл\n");

            /* Output file */
            context->output_file = argv[i + 1];
            i++;
        }
        else if (!enough_files)
        {
            char *macro_processed;

#ifndef FILE_DEBUG
            /* Regular file */
            char macro_path[] = "/tmp/macroXXXXXX";
            char tree_path[] = "/tmp/treeXXXXXX";
            char codes_path[] = "/tmp/codesXXXXXX";

            mktemp(macro_path);
            mktemp(tree_path);
            mktemp(codes_path);
#else
            char macro_path[] = "macro.txt";
            char tree_path[] = "tree.txt";
            char codes_path[] = "codes.txt";
#endif
            if (strlen(macro_path) == 0 || strlen(tree_path) == 0 ||
                strlen(codes_path) == 0)
            {
                fprintf(stderr, " ошибка при создании временного файла\n");
                exit(1);
            }

            // Открытие исходного текста
            ruc_context_attach_io(context, argv[i], IO_TYPE_INPUT,
                                  IO_SOURCE_FILE);

            // Препроцессинг в массив
            ruc_context_attach_io(context, "", IO_TYPE_OUTPUT,
                                  IO_SOURCE_MEM);

            printf("\nИсходный текст:\n \n");

            preprocess_file(context); //   макрогенерация
            macro_processed = context->output_options.ptr;

            ruc_context_detach_io(context, IO_TYPE_OUTPUT);
            ruc_context_detach_io(context, IO_TYPE_INPUT);

            ruc_context_attach_io(context, macro_processed, IO_TYPE_INPUT,
                                  IO_SOURCE_MEM);
            output_tables_and_tree(context, tree_path);
            output_codes(context, codes_path);
            ruc_context_detach_io(context, IO_TYPE_INPUT);

            free(macro_processed);
            /* Will be left for debugging in case of failure */
#ifndef FILE_DEBUG
            unlink(tree_path);
            unlink(codes_path);
            unlink(macro_path);
#endif
            enough_files = true;
        }
        else
        {
            fprintf(stderr, " лимит исходных файлов\n");
            break;
        }
    }

    output_export(context,
                  context->output_file != NULL ? context->output_file
                                               : "export.txt");
}

int
main(int argc, const char *argv[])
{
    ruc_context context;

    ruc_context_init(&context);
    ruc_context_attach_io(&context, ":stderr", IO_TYPE_ERROR, IO_SOURCE_FILE);
    ruc_context_attach_io(&context, ":stdout", IO_TYPE_MISC, IO_SOURCE_FILE);

    read_keywords(&context, "keywords.txt");

    init_modetab(&context);

    process_user_requests(&context, argc, argv);
    return 0;
}
