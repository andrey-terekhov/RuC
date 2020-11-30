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

#include "syntax.h"

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

/** Вывод таблиц в файл */
void output_export(universal_io *const io, const syntax *const sx)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%i %i %i %i %i %i %i\n", sx->pc, sx->funcnum, sx->id,
				   sx->reprtab.len, sx->md, sx->maxdisplg, sx->wasmain);

	for (int i = 0; i < sx->pc; i++)
	{
		uni_printf(io, "%i ", sx->mem[i]);
	}
	uni_printf(io, "\n");

	for (int i = 0; i < sx->funcnum; i++)
	{
		uni_printf(io, "%i ", sx->functions[i]);
	}
	uni_printf(io, "\n");

	for (int i = 0; i < sx->id; i++)
	{
		uni_printf(io, "%i ", sx->identab[i]);
	}
	uni_printf(io, "\n");

	for (int i = 0; i < sx->reprtab.len; i++)
	{
		uni_printf(io, "%i ", sx->reprtab.table[i]);
	}

	for (int i = 0; i < sx->md; i++)
	{
		uni_printf(io, "%i ", sx->modetab[i]);
	}
	uni_printf(io, "\n");
}

int compile_from_io_to_vm(universal_io *const io)
{
	if (!in_is_correct(io) || !out_is_correct(io))
	{
		system_error("некорректные параметры ввода/вывода");
		io_erase(io);
		return 1;
	}

	syntax sx;
	if (syntax_init(&sx))
	{
		system_error("не удалось выделить память для таблиц");
		io_erase(io);
		return 1;
	}

	universal_io temp = io_create();
	compiler_context context = compiler_context_create(&temp, &sx);

	read_keywords(&context);
	init_modetab(&context);

	io_erase(&temp);

	context.io = io;
	ext_decl(&context);
	tables_and_tree(&sx, DEFAULT_TREE);

	if (!context.error_flag)
	{
		codegen(&context);
		tables_and_codes(&sx, DEFAULT_CODES);
	}

	output_export(io, &sx);
	io_erase(io);

	syntax_deinit(&sx);
	return context.error_flag;
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
	if (!ret)
	{
		make_executable(ws_get_output(ws));
	}

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

	int ret = compile_from_io_to_vm(&io);
	if (!ret)
	{
		make_executable(DEFAULT_OUTPUT);
	}

	return ret;
}
