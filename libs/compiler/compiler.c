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

#include "extdecl.h"
#include "scanner.h"
#include "uniprinter.h"
#include <limits.h>
#include <math.h>


#ifdef __linux__
	#include <unistd.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
	#include <sys/stat.h>
	#include <sys/types.h>
#endif


//#define GENERATE_FILES


const char *const DEFAULT_MACRO = "macro.txt";
const char *const DEFAULT_KEYWORDS = "keywords.txt";
const char *const DEFAULT_TREE = "tree.txt";
const char *const DEFAULT_CODES = "codes.txt";
const char *const DEFAULT_OUTPUT = "export.txt";


/** Make executable actually executable on best-effort basis (if possible) */
static void make_executable(const char *const path)
{
#ifndef _MSC_VER
	struct stat stat_buf;

	if (stat(path, &stat_buf))
	{
		return;
	}

	chmod(path, stat_buf.st_mode | S_IXUSR);
#else
	(void)path;
#endif
}

/** Вывод таблиц и дерева */
void output_tables_and_tree(compiler_context *context, const char *path)
{
	out_set_file(context->io, path);

	getnext(context);
	nextch(context);
	context->next = scan(context);

	ext_decl(context); // генерация дерева

	tables_and_tree(context);
	out_clear(context->io);
}

/** Генерация кодов */
void output_codes(compiler_context *context, const char *path)
{
	out_set_file(context->io, path);
	codegen(context);
	tables_and_code(context);
	out_clear(context->io);
}

/** Вывод таблиц в файл */
void output_export(compiler_context *context, const char *path)
{
	int i;

	out_set_file(context->io, path);
	uni_printf(context->io, "#!/usr/bin/ruc-vm\n");

	uni_printf(context->io, "%i %i %i %i %i %i %i\n", context->pc, context->funcnum, context->id,
				   REPRTAB_LEN, context->md, context->maxdisplg, context->wasmain);

	for (i = 0; i < context->pc; i++)
	{
		uni_printf(context->io, "%i ", context->mem[i]);
	}
	uni_printf(context->io, "\n");

	for (i = 0; i < context->funcnum; i++)
	{
		uni_printf(context->io, "%i ", context->functions[i]);
	}
	uni_printf(context->io, "\n");

	for (i = 0; i < context->id; i++)
	{
		uni_printf(context->io, "%i ", context->identab[i]);
	}
	uni_printf(context->io, "\n");

	for (i = 0; i < REPRTAB_LEN; i++)
	{
		uni_printf(context->io, "%i ", REPRTAB[i]);
	}

	for (i = 0; i < context->md; i++)
	{
		uni_printf(context->io, "%i ", context->modetab[i]);
	}
	uni_printf(context->io, "\n");

	out_clear(context->io);

	make_executable(path);
}

int compile_from_io_to_vm(universal_io *const io)
{
	if (!in_is_correct(io) || !out_is_correct(io))
	{
		io_erase(io);
		system_error("некорректные параметры ввода/вывода");
		return 1;
	}

	compiler_context *context = malloc(sizeof(compiler_context));
	if (context == NULL)
	{
		io_erase(io);
		system_error("не удалось выделить память под контекст");
		return 1;
	}

	universal_io temp = io_create();
	compiler_context_init(context, &temp);

	read_keywords(context);

	init_modetab(context);

	io_erase(&temp);
	char output[MAX_ARG_SIZE];
	out_get_path(io, output);

	context->io = io;
	output_tables_and_tree(context, DEFAULT_TREE);
	if (!context->error_flag)
	{
		output_codes(context, DEFAULT_CODES);
	}

	output_export(context, output);
	io_erase(io);

	int ret = context->error_flag;
	free(context);
	return ret;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int compile_to_vm(workspace *const ws)
{
	if (!ws_is_correct(ws))
	{
		system_error("некорректные входные данные");
		return 1;
	}

	// Препроцессинг в массив
	char *const preprocessing = macro(ws); // макрогенерация
	if (preprocessing == NULL)
	{
		return 1;
	}

	universal_io io = io_create();
	in_set_buffer(&io, preprocessing);
	out_set_file(&io, ws_get_output(ws));
	int ret = compile_from_io_to_vm(&io);

	free(preprocessing);
	return ret;
}

int auto_compile_to_vm(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	return compile_to_vm(&ws);
}

int no_macro_compile_to_vm(const char *const path)
{
	universal_io io = io_create();
	in_set_file(&io, path);
	out_set_file(&io, DEFAULT_OUTPUT);
	return compile_from_io_to_vm(&io);
}
