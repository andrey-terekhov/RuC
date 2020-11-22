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
#include "io.h"
#include "logger.h"
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

#define FILE_DEBUG


#ifdef ANALYSIS_ENABLED
void report_cb(asp_report *report)
{
	char msg[4 * MAXSTRINGL];
	sprintf(msg, "%s:%d:%d: %s: %s", report->file, report->line, report->column, report->rule_id,
			report->explanation);
	log_system_note("report_cb", msg)
}
#endif

char *preprocess_ruc_file(compiler_context *context, const workspace *const ws)
{

	char **argv = malloc(MAX_PATHS * sizeof(char *));

	int argc = 0;
	const char *temp = ws_get_file(ws, argc);
	while (temp != NULL)
	{
		argv[argc] = malloc((1 + strlen(temp)) * sizeof(char));
		sprintf(argv[argc++], "%s", temp);
		temp = ws_get_file(ws, argc);
	}

	const int files_num = argc;
	temp = ws_get_dir(ws, argc - files_num);
	while (temp != NULL)
	{
		argv[argc] = malloc((3 + strlen(temp)) * sizeof(char));
		sprintf(argv[argc++], "-I%s", temp);
		temp = ws_get_dir(ws, argc - files_num);
	}
	
	char *result = preprocess_file(argc, (const char **)argv);
	for (int i = 0; i < argc; i++)
	{
		free(argv[i]);
	}
	free(argv);
	return result;
}

static void process_user_requests(compiler_context *context, const workspace *const ws)
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
		log_system_error("ruc", "не удалось создать временные файлы");
		exit(1);
	}

	// Препроцессинг в массив

	macro_processed = preprocess_ruc_file(context, ws); // макрогенерация
	if (macro_processed == NULL)
	{
		log_system_error("ruc", "не удалось выделить память для макрогенератора");
		exit(1);
	}
	//compiler_context_detach_io(context, IO_TYPE_OUTPUT);
	//compiler_context_detach_io(context, IO_TYPE_INPUT);
	//compiler_context_attach_io(context, macro_processed, IO_TYPE_INPUT, IO_SOURCE_MEM);
	in_set_buffer(&context->io, macro_processed);
	output_tables_and_tree(context, tree_path);
	if (!context->error_flag)
	{
		output_codes(context, codes_path);
	}
	//compiler_context_detach_io(context, IO_TYPE_INPUT);

	/* Will be left for debugging in case of failure */
#if !defined(FILE_DEBUG) && !defined(_MSC_VER)
	unlink(tree_path);
	unlink(codes_path);
	unlink(macro_path);
#endif
#ifdef ANALYSIS_ENABLED
	asp_simple_invoke_singlefile(ASP_HOST, ASP_PORT, argv[i], ASP_LANGUAGE_RUC, report_cb);
#endif

	output_export(context, ws_get_output(ws));
	io_erase(&context->io);
	free(macro_processed);
}

int compile_to_vm(const workspace *const ws)
{
	if (!ws_is_correct(ws))
	{
		log_system_error("ruc", "некорректные входные данные");
		return 1;
	}

	compiler_context *context = malloc(sizeof(compiler_context));
	if (context == NULL)
	{
		log_system_error("ruc", "не удалось выделить память под контекст");
		return 1;
	}

	compiler_context_init(context);
	//compiler_context_attach_io(context, ":stderr", IO_TYPE_ERROR, IO_SOURCE_FILE);
	//compiler_context_attach_io(context, ":stdout", IO_TYPE_MISC, IO_SOURCE_FILE);

	read_keywords(context);

	init_modetab(context);

	process_user_requests(context, ws);

	int ret = get_exit_code(context);
	//compiler_context_deinit(context);
	free(context);
	return ret;
}

int auto_compile_to_vm(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	return compile_to_vm(&ws);
}
