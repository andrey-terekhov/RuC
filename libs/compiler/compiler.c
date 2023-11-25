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
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "errors.h"
#include "mipsgen.h"
#include "riscvgen.h"
#include "llvmgen.h"
#include "parser.h"
#include "macro.h"
#include "syntax.h"
#include "uniio.h"

#ifndef _WIN32
	#include <sys/stat.h>
	#include <sys/types.h>
#endif


static const char *const DEFAULT_MACRO = "macro.txt";

static const char *const DEFAULT_VM = "out.ruc";
static const char *const DEFAULT_LLVM = "out.ll";
static const char *const DEFAULT_MIPS = "out.s";
static const char *const DEFAULT_RISCV = "out-riscv.s";


typedef int (*encoder)(const workspace *const ws, syntax *const sx);


/** Make executable actually executable on best-effort basis (if possible) */
static inline void make_executable(const char *const path)
{
#ifndef _WIN32
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


static status_t compile_from_io(const workspace *const ws, universal_io *const io, const encoder enc)
{
	if (!in_is_correct(io) || !out_is_correct(io))
	{
		error_msg("некорректные параметры ввода/вывода");
		io_erase(io);
		return sts_system_error;
	}

	syntax sx = sx_create(ws, io);
	int ret = parse(&sx);
	status_t sts = sts_parse_error;

	if (!ret && !ws_has_flag(ws, "-c")) // Skip linker stage
	{
		ret = !sx_is_correct(&sx);
		sts = sts_link_error;
	}

	if (!ret)
	{
		ret = enc(ws, &sx);
		sts = sts_codegen_error;
	}

	sx_clear(&sx);
	io_erase(io);
	return ret ? sts : sts_success;
}

static status_t compile_from_ws(workspace *const ws, const encoder enc)
{
	if (!ws_is_correct(ws) || ws_get_files_num(ws) == 0)
	{
		error_msg("некорректные входные данные");
		return sts_system_error;
	}

	if (ws_has_flag(ws, "-E"))
	{
		return macro_to_file(ws, ws_get_output(ws)) ? sts_macro_error : sts_success;
	}

	universal_io io = io_create();

#ifndef GENERATE_MACRO
	// Препроцессинг в массив
	char *const preprocessing = macro(ws); // макрогенерация
	if (preprocessing == NULL)
	{
		return sts_macro_error;
	}

	in_set_buffer(&io, preprocessing);
#else
	int ret_macro = macro_to_file(ws, DEFAULT_MACRO);
	if (ret_macro)
	{
		return sts_macro_error;
	}

	in_set_file(&io, DEFAULT_MACRO);
#endif

	out_set_file(&io, ws_get_output(ws));
	const status_t sts = compile_from_io(ws, &io, enc);

#ifndef GENERATE_MACRO
	free(preprocessing);
#endif
	return sts;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


status_t compile(workspace *const ws)
{
	if (ws_has_flag(ws, "-LLVM"))
	{
		return compile_to_llvm(ws);
	}
	else if (ws_has_flag(ws, "-MIPS"))
	{
		return compile_to_mips(ws);
	}
	else if (ws_has_flag(ws, "-RISCV"))
	{
		return compile_to_riscv(ws);
	}
	else // if (ws_has_flag(ws, "-VM"))
	{
		return compile_to_vm(ws);
	}
}

status_t compile_to_vm(workspace *const ws)
{
	if (ws_get_output(ws) == NULL)
	{
		ws_set_output(ws, DEFAULT_VM);
	}

	const status_t sts = compile_from_ws(ws, &encode_to_vm);
	if (sts == sts_success)
	{
		make_executable(ws_get_output(ws));
	}

	return sts == sts_codegen_error ? sts_virtul_error : sts;
}

status_t compile_to_llvm(workspace *const ws)
{
	if (ws_get_output(ws) == NULL)
	{
		ws_set_output(ws, DEFAULT_LLVM);
	}

	const status_t sts = compile_from_ws(ws, &encode_to_llvm);
	return sts == sts_codegen_error ? sts_llvm_error : sts;
}

int compile_to_mips(workspace *const ws)
{
	if (ws_get_output(ws) == NULL)
	{
		ws_set_output(ws, DEFAULT_MIPS);
	}

	return compile_from_ws(ws, &encode_to_mips);
}

int compile_to_riscv(workspace *const ws)
{
	if (ws_get_output(ws) == NULL)
	{
		ws_set_output(ws, DEFAULT_RISCV);
	}

	return compile_from_ws(ws, &encode_to_riscv);
}



int auto_compile(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	const int ret = compile(&ws);
	ws_clear(&ws);
	return ret;
}

int auto_compile_to_vm(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	const int ret = compile_to_vm(&ws);
	ws_clear(&ws);
	return ret;
}

int auto_compile_to_llvm(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	const status_t ret = compile_to_llvm(&ws);
	ws_clear(&ws);
	return ret;
}

int auto_compile_to_mips(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	const int ret = compile_to_mips(&ws);
	ws_clear(&ws);
	return ret;
}

int auto_compile_to_riscv(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	const int ret = compile_to_riscv(&ws);
	ws_clear(&ws);
	return ret;
}




int no_macro_compile_to_vm(const char *const path)
{
	universal_io io = io_create();
	in_set_file(&io, path);

	workspace ws = ws_create();
	ws_add_file(&ws, path);
	ws_set_output(&ws, DEFAULT_VM);
	out_set_file(&io, ws_get_output(&ws));

	const int ret = compile_from_io(&ws, &io, &encode_to_vm);
	if (!ret)
	{
		make_executable(ws_get_output(&ws));
	}

	ws_clear(&ws);
	return ret;
}

int no_macro_compile_to_llvm(const char *const path)
{
	universal_io io = io_create();
	in_set_file(&io, path);

	workspace ws = ws_create();
	ws_add_file(&ws, path);
	ws_set_output(&ws, DEFAULT_LLVM);
	out_set_file(&io, ws_get_output(&ws));

	const int ret = compile_from_io(&ws, &io, &encode_to_llvm);
	ws_clear(&ws);
	return ret;
}

int no_macro_compile_to_mips(const char *const path)
{
	universal_io io = io_create();
	in_set_file(&io, path);

	workspace ws = ws_create();
	ws_add_file(&ws, path);
	ws_set_output(&ws, DEFAULT_MIPS);
	out_set_file(&io, ws_get_output(&ws));

	const int ret = compile_from_io(&ws, &io, &encode_to_mips);
	ws_clear(&ws);
	return ret;
}

int no_macro_compile_to_risv(const char *const path)
{
	universal_io io = io_create();
	in_set_file(&io, path);

	workspace ws = ws_create();
	ws_add_file(&ws, path);
	ws_set_output(&ws, DEFAULT_MIPS);
	out_set_file(&io, ws_get_output(&ws));

	const int ret = compile_from_io(&ws, &io, &encode_to_mips);
	ws_clear(&ws);
	return ret;
}
