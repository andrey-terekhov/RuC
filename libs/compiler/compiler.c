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
#include "uniio.h"
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


#define FILE_DEBUG


static void process_user_requests(compiler_context *context, workspace *const ws)
{	
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
		system_error("не удалось создать временные файлы");
		exit(1);
	}

	// Препроцессинг в массив

	char *macro_processed = macro(ws); // макрогенерация
	if (macro_processed == NULL)
	{
		system_error("не удалось выделить память для макрогенератора");
		exit(1);
	}
	
	in_set_buffer(&context->io, macro_processed);
	output_tables_and_tree(context, tree_path);
	if (!context->error_flag)
	{
		output_codes(context, codes_path);
	}

	/* Will be left for debugging in case of failure */
#if !defined(FILE_DEBUG) && !defined(_MSC_VER)
	unlink(tree_path);
	unlink(codes_path);
	unlink(macro_path);
#endif

	output_export(context, ws_get_output(ws));
	io_erase(&context->io);
	free(macro_processed);
}

int compile_to_vm(workspace *const ws)
{
	if (!ws_is_correct(ws))
	{
		system_error("некорректные входные данные");
		return 1;
	}

	compiler_context *context = malloc(sizeof(compiler_context));
	if (context == NULL)
	{
		system_error("не удалось выделить память под контекст");
		return 1;
	}

	compiler_context_init(context);

	read_keywords(context);

	init_modetab(context);

	process_user_requests(context, ws);

	int ret = context->error_flag;
	free(context);
	return ret;
}

int auto_compile_to_vm(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	return compile_to_vm(&ws);
}
