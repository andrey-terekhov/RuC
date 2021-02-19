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
#include "errors.h"
#include "tree.h"
#include "uniprinter.h"


typedef struct information
{
	item_t string_num;
	item_t was_printf;
} information;


static size_t node_recursive(node *const nd, size_t i, universal_io *const io, information *const info)
{
	for (size_t j = 0; j < node_get_amount(nd); j++)
	{
		node child = node_get_child(nd, j);

		switch (node_get_type(&child))
		{
			case TString:
			{
				const size_t N = (size_t)node_get_arg(&child, 0);

				uni_printf(io, "@.str%li = private unnamed_addr constant [%li x i8] c\"", info->string_num++, N + 1);
				for (size_t k = 0; k < N; k++) 
				{
					const char ch = (char)node_get_arg(&child, k + 1);
					if (ch == '\n')
					{
						uni_printf(io, "\\0A");
					}
					else
					{
						uni_printf(io, "%c", ch);
					}
				}
				uni_printf(io, "\\00\", align 1\n");
			}
			break;
			case TPrintf:
			{
				const size_t N = (size_t)node_get_arg(&child, 0);
				// перестановка TPrintf
				// TODO: подумать, как для всех типов работать будет
				for (size_t k = 0; k < N + 1; k++)
				{
					node_swap(nd, j-k, nd, j-k-1);
				}
				// перестановка TString
				// TODO: подумать, как для всех типов работать будет
				for (size_t k = 0; k < N; k++)
				{
					node_swap(nd, j-k, nd, j-k-1);
				}

				info->was_printf = 1;
			}
			break;
		}

		i = node_recursive(&child, i, io, info);
	}
	return i;
}

static int optimize_pass(universal_io *const io, syntax *const sx, int architecture)
{
	// архитектурно-зависимая часть
	// здесь будет обработка флагов
	switch (architecture)
	{
		case 0:
		{
			uni_printf(io, "target datalayout = \"e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64\"\n");
			uni_printf(io, "target triple = \"mipsel\"\n\n");
		}
		break;
		case 1:
		{
			uni_printf(io, "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n");
			uni_printf(io, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
		}
		break;
	}

	information info;
	info.string_num = 1;
	info.was_printf = 0;

	if (!vector_is_correct(&sx->tree))
	{
		return -1;
	}

	node nd = node_get_root(&sx->tree);
	
	size_t index = 0;
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		index = node_recursive(&child, index, io, &info);
	}

	uni_printf(io, "\n");
	if (info.was_printf)
	{
		uni_printf(io, "declare i32 @printf(i8*, ...)\n");
	}
	uni_printf(io, "\n");

	return index != SIZE_MAX && index == vector_size(nd.tree) ? 0 : -1;
}


/*
 *	 __	 __   __	 ______   ______	 ______	 ______   ______	 ______	 ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	\ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	 \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int optimize_for_llvm(const workspace *const ws, universal_io *const io, syntax *const sx, int architecture)
{
	if (!out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	return optimize_pass(io, sx, architecture);
}
