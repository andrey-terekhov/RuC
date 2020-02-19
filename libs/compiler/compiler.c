/*
 *	Copyright 2015 Andrey Terekhov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include "codegen.h"
#include "codes.h"
#include "context.h"
#include "defs.h"
#include "errors.h"
#include "frontend_utils.h"
#include "tables.h"

#ifdef ANALYSIS_ENABLED
#include "asp/asp_simple.h"
#define ASP_HOST "localhost"
#define ASP_PORT (5500)
#endif

extern void preprocess_file(compiler_context *context);

//#define FILE_DEBUG

#ifdef ANALYSIS_ENABLED
void report_cb(asp_report *report)
{
	fprintf(stderr, "%s:%d:%d: %s: %s\n", report->file, report->line, report->column, report->rule_id,
			report->explanation);
}
#endif

static void process_user_requests(compiler_context *context, int argc, const char *argv[])
{
	int i;
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

#if !defined(FILE_DEBUG) && !defined(_MSC_VER)
			/* Regular file */
			char macro_path[] = "/tmp/macroXXXXXX";
			char tree_path[] = "/tmp/treeXXXXXX";
			char codes_path[] = "/tmp/codesXXXXXX";

			mkstemp(macro_path);
			mkstemp(tree_path);
			mkstemp(codes_path);
#else
			char macro_path[] = "macro.txt";
			char tree_path[] = "tree.txt";
			char codes_path[] = "codes.txt";
#endif
			if (strlen(macro_path) == 0 || strlen(tree_path) == 0 || strlen(codes_path) == 0)
			{
				fprintf(stderr, " ошибка при создании временного файла\n");
				exit(1);
			}

			// Открытие исходного текста
			compiler_context_attach_io(context, argv[i], IO_TYPE_INPUT, IO_SOURCE_FILE);

			// Препроцессинг в массив
			compiler_context_attach_io(context, "", IO_TYPE_OUTPUT, IO_SOURCE_MEM);

			printf("\nИсходный текст:\n \n");

			preprocess_file(context); //   макрогенерация
			macro_processed = strdup(context->output_options.ptr);
			if (macro_processed == NULL)
			{
				fprintf(stderr, " ошибка выделения памяти для "
								"макрогенератора\n");
				exit(1);
			}

			compiler_context_detach_io(context, IO_TYPE_OUTPUT);
			compiler_context_detach_io(context, IO_TYPE_INPUT);

			compiler_context_attach_io(context, macro_processed, IO_TYPE_INPUT, IO_SOURCE_MEM);
			output_tables_and_tree(context, tree_path);
			output_codes(context, codes_path);
			compiler_context_detach_io(context, IO_TYPE_INPUT);

			/* Will be left for debugging in case of failure */
#if !defined(FILE_DEBUG) && !defined(_MSC_VER)
			unlink(tree_path);
			unlink(codes_path);
			unlink(macro_path);
#endif
			enough_files = true;
#ifdef ANALYSIS_ENABLED
			asp_simple_invoke_singlefile(ASP_HOST, ASP_PORT, argv[i], ASP_LANGUAGE_RUC, report_cb);
#endif
		}
		else
		{
			fprintf(stderr, " лимит исходных файлов\n");
			break;
		}
	}

	output_export(context, context->output_file != NULL ? context->output_file : "export.txt");
}

#ifdef _MSC_VER
__declspec(dllexport)
#endif
	int compile(int argc, const char *argv[])
{
	compiler_context *context = malloc(sizeof(compiler_context));

	if (context == NULL)
	{
		fprintf(stderr, " ошибка выделения памяти под контекст\n");
		return 1;
	}

	compiler_context_init(context);
	compiler_context_attach_io(context, ":stderr", IO_TYPE_ERROR, IO_SOURCE_FILE);
	compiler_context_attach_io(context, ":stdout", IO_TYPE_MISC, IO_SOURCE_FILE);

	read_keywords(context);

	init_modetab(context);

	process_user_requests(context, argc, argv);

	compiler_context_deinit(context);
	free(context);
	return 0;
}
