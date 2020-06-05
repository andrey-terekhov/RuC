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
#include "codegen.h"
#include "codes.h"
#include "context.h"
#include "defs.h"
#include "errors.h"
#include "frontend_utils.h"
#include "preprocessor.h"
#include "tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef __linux__
	#include <unistd.h>
#endif

#ifdef ANALYSIS_ENABLED
	#include "asp/asp_simple.h"
	#define ASP_HOST "localhost"
	#define ASP_PORT (5500)
#endif

//#define FILE_DEBUG


#ifdef ANALYSIS_ENABLED
void report_cb(asp_report *report)
{
	fprintf(stderr, "%s:%d:%d: %s: %s\n", report->file, report->line, report->column, report->rule_id,
			report->explanation);
}
#endif

static void process_user_requests(compiler_context *context, compiler_workspace *workspace)
{
	compiler_workspace_file *file;

	file = workspace->files;
	while (file != NULL)
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
		//compiler_context_attach_io(context, file->path, IO_TYPE_INPUT, IO_SOURCE_FILE);

		// Препроцессинг в массив
		//compiler_context_attach_io(context, "", IO_TYPE_OUTPUT, IO_SOURCE_MEM);

		printf("\nИсходный текст:\n \n");

		macro_processed = preprocess_file(context, file->path); // макрогенерация
		//macro_processed = strdup(context->output_options.ptr);
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
#ifdef ANALYSIS_ENABLED
		asp_simple_invoke_singlefile(ASP_HOST, ASP_PORT, argv[i], ASP_LANGUAGE_RUC, report_cb);
#endif

		/* FIXME: support more than one file */
		break;
	}

	output_export(context, workspace->output_file != NULL ? workspace->output_file : "export.txt");
}

compiler_workspace *compiler_workspace_create()
{
	return calloc(1, sizeof(compiler_workspace));
}

void compiler_workspace_free(compiler_workspace *workspace)
{
	compiler_workspace_file *file;

	if (workspace == NULL)
	{
		return;
	}

	/* Free up files */
	file = workspace->files;
	while (file != NULL)
	{
		compiler_workspace_file *next = file->next;

		free(file->path);
		free(file);
		file = next;
	}

	/* Free up the rest of workspace */
	free(workspace->output_file);
	free(workspace);
}

compiler_workspace_file *compiler_workspace_add_file(compiler_workspace *workspace, const char *path)
{
	compiler_workspace_file *file;
	compiler_workspace_file *tmp;

	file = calloc(1, sizeof(compiler_workspace_file));
	if (file == NULL)
	{
		return NULL;
	}

	file->path = strdup(path);

	/* Find the tail file */
	tmp = workspace->files;
	while (tmp != NULL && tmp->next != NULL)
	{
		tmp = tmp->next;
	}

	/* Actually put it to the tail */
	if (tmp != NULL)
	{
		tmp->next = file;
	}
	else
	{
		workspace->files = file;
	}

	return file;
}

char *compiler_workspace_error2str(compiler_workspace_error *error)
{
	char *str = NULL;

	if (error == NULL)
	{
		return NULL;
	}

	switch (error->code)
	{
		case COMPILER_WS_ENOOUTPUT:
		{
			/*
			 * See, even though the string is static, we leave this API
			 * open for improvements, hence the strdup()/free()
			 */
			str = strdup("Output file is not set");
			break;
		}
		case COMPILER_WS_EFILEADD:
		{
			str = strdup("Error adding input file");
			break;
		}
		case COMPILER_WS_ENOINPUT:
		{
			str = strdup("No input files");
			break;
		}
		default:
		{
			break;
		}
	}

	return str;
}

compiler_workspace *compiler_get_workspace(int argc, const char *argv[])
{
	compiler_workspace *ws;
	int i;

	ws = compiler_workspace_create();
	if (ws == NULL)
	{
		return NULL;
	}

	for (i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-o") == 0)
		{
			if ((i + 1) >= argc)
			{
				ws->error.code = COMPILER_WS_ENOOUTPUT;
				break;
			}

			/* Output file */
			ws->output_file = strdup(argv[i + 1]);
			i++;
		}
		else
		{
			if (compiler_workspace_add_file(ws, argv[i]) == NULL)
			{
				ws->error.code = COMPILER_WS_EFILEADD;
				break;
			}
		}
	}

	if (ws->files == NULL)
	{
		ws->error.code = COMPILER_WS_ENOINPUT;
	}

	return ws;
}

COMPILER_EXPORTED int compiler_workspace_compile(compiler_workspace *workspace)
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

	process_user_requests(context, workspace);

	compiler_context_deinit(context);
	free(context);
	return 0;
}

COMPILER_EXPORTED int compiler_compile(const char *path)
{
	int ret;
	compiler_workspace *ws;

	ws = compiler_workspace_create();
	if (ws == NULL)
	{
		/* Failed to create workspace */
		return 1;
	}

	if (compiler_workspace_add_file(ws, path) == NULL)
	{
		/* Failed to add file to workspace */
		compiler_workspace_free(ws);
		return 1;
	}

	ret = compiler_workspace_compile(ws);
	compiler_workspace_free(ws);

	return ret;
}
