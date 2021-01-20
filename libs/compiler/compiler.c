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
#include "analyzer.h"
#include "codegen.h"
#include "codes.h"
#include "errors.h"
#include "preprocessor.h"
#include "syntax.h"
#include "tree.h"
#include "uniio.h"
#include <stdlib.h>

#ifndef _MSC_VER
	#include <sys/stat.h>
	#include <sys/types.h>
#endif


#define GENERATE_MACRO
#define GENERATE_TABLES
#define GENERATE_TREE


const char *const DEFAULT_MACRO = "macro.txt";
const char *const DEFAULT_TREE = "tree.txt";
const char *const DEFAULT_NEW = "new.txt";
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

int compile_from_io_to_vm(universal_io *const io)
{
	if (!in_is_correct(io) || !out_is_correct(io))
	{
		system_error("некорректные параметры ввода/вывода");
		io_erase(io);
		return -1;
	}

	syntax sx;
	int ret = sx_init(&sx);

	if (!ret)
	{
		ret = analyze(io, &sx);
#ifdef GENERATE_TABLES
		tables_and_tree(&sx, DEFAULT_TREE);
#endif
	}
	
	if (!ret)
	{
		ret = !sx_is_correct(&sx, io);
	}

	if (!ret)
	{
#ifdef GENERATE_TREE
		ret = tree_test(&sx)
			|| tree_test_next(&sx)
			|| tree_test_recursive(&sx)
			|| tree_test_copy(&sx);
		if (ret)
		{
			io_erase(io);
			return ret;
		}
		tree_print(&sx, DEFAULT_NEW);
#endif

		ret = encode_to_vm(io, &sx);
#ifdef GENERATE_TABLES
		tables_and_codes(&sx, DEFAULT_CODES);
#endif
	}

	io_erase(io);
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
		return -1;
	}

	universal_io io = io_create();

#ifndef GENERATE_MACRO
	// Препроцессинг в массив
	char *const preprocessing = macro(ws); // макрогенерация
	if (preprocessing == NULL)
	{
		return -1;
	}

	in_set_buffer(&io, preprocessing);
#else
	int ret_macro = macro_to_file(ws, DEFAULT_MACRO);
	if (ret_macro)
	{
		return ret_macro;
	}

	in_set_file(&io, DEFAULT_MACRO);
#endif

	out_set_file(&io, ws_get_output(ws));

	int ret = compile_from_io_to_vm(&io);
	if (!ret)
	{
		make_executable(ws_get_output(ws));
	}

#ifndef GENERATE_MACRO
	free(preprocessing);
#endif
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
