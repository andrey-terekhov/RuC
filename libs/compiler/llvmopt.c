/*
 *	Copyright 2021 Andrey Terekhov, Ivan S. Arkhipov
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

#include "llvmopt.h"
#include <string.h>
#include "errors.h"
#include "operations.h"
#include "parser.h"
#include "stack.h"
#include "tree.h"
#include "uniprinter.h"


typedef struct information
{
	syntax *sx;										/**< Структура syntax с таблицами */

	item_t string_num;								/**< Номер строки */
	item_t init_num;								/**< Счётчик для инициализации */
	item_t was_printf;								/**< Флаг наличия printf в исходном коде */

	// TODO: может стоит и тут сделать функцию печать типа?
	item_t arr_init_type;							/**< Тип массива при инициализации */
} information;


static int node_recursive(information *const info, node *const nd)
{
	if (node_get_type(nd) == OP_ARRAY_INIT)
	{
		uni_printf(info->sx->io, "@arr_init.%" PRIitem " = private unnamed_addr constant ", info->init_num);
		info->init_num++;
		// TODO: а для многомерных как?
		uni_printf(info->sx->io, "[%" PRIitem " x %s] [", node_get_arg(nd, 0)
			, type_is_integer(info->arr_init_type) ? "i32" : "double");
	}

	for (size_t i = 0; i < node_get_amount(nd); i++)
	{
		node child = node_get_child(nd, i);

		switch (node_get_type(&child))
		{
			case OP_STRING:
			{
				const size_t N = node_get_argc(&child) - 4;
				uni_printf(info->sx->io, "@.str%" PRIitem " = private unnamed_addr constant [%zi x i8] c\""
					, info->string_num++, N + 1);

				for (size_t j = 0; j < N; j++)
				{
					const char ch = (char)node_get_arg(&child, j + 2);
					if (ch == '\n')
					{
						uni_printf(info->sx->io, "\\0A");
					}
					else
					{
						uni_printf(info->sx->io, "%c", ch);
					}
				}
				uni_printf(info->sx->io, "\\00\", align 1\n");
			}
			break;
			case OP_PRINTF:
				info->was_printf = 1;
				break;
			case OP_DECL_ID:
				info->arr_init_type = node_get_arg(&child, 1);
				break;
			default:
				break;
		}

		node_recursive(info, &child);
	}

	return 0;
}

static int optimize_pass(information *const info)
{
	node nd = node_get_root(&info->sx->tree);
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		if (node_recursive(info, &child))
		{
			return -1;
		}
	}

	uni_printf(info->sx->io, "\n");
	if (info->was_printf)
	{
		uni_printf(info->sx->io, "declare i32 @printf(i8*, ...)\n");
	}
	uni_printf(info->sx->io, "\n");

	return 0;
}

static void architecture(const workspace *const ws, syntax *const sx)
{
	for (size_t i = 0; ; i++)
	{
		const char *flag = ws_get_flag(ws, i);

		if (flag == NULL || strcmp(flag, "--x86_64") == 0)
		{
			uni_printf(sx->io, "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n");
			uni_printf(sx->io, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
			return;
		}
		else if (strcmp(flag, "--mipsel") == 0)
		{
			uni_printf(sx->io, "target datalayout = \"e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64\"\n");
			uni_printf(sx->io, "target triple = \"mipsel\"\n\n");
			return;
		}
	}
}


/*
 *	 __	 __   __	 ______   ______	 ______	 ______   ______	 ______	 ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	\ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	 \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int optimize_for_llvm(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	information info;
	info.sx = sx;
	info.string_num = 1;
	info.init_num = 1;
	info.was_printf = 0;

	architecture(ws, sx);
	const int ret = optimize_pass(&info);

	return ret;
}
