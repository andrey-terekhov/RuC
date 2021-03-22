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
#include "tree.h"
#include "uniprinter.h"


#define MAX_STACK_SIZE	512


typedef enum EXPRESSION
{
	OPERAND,
	UNARY_OPERATION,
	BINARY_OPERATION,
	NOT_EXPRESSION,
} expression_t;

typedef struct node_info
{
	node *parent;										/**< Родитель узла */ 
	size_t child;										/**< Номер ребёнка, которым является узел */
	size_t depth;										/**< Количество узлов после данного узла при перестановке */
} node_info;

typedef struct information
{
	universal_io *io;									/**< Вывод */

	item_t string_num;									/**< Номер строки */
	item_t was_printf;									/**< Флаг наличия printf в исходном коде */

	node_info stack[MAX_STACK_SIZE];					/**< Стек для преобразования выражений */
	size_t stack_size;									/**< Размер стека */
} information;


static inline void stack_push(information *const info, node_info nd)
{
	info->stack[info->stack_size++] = nd;
}

static inline node_info stack_pop(information *const info)
{
	return info->stack[--info->stack_size];
}

static inline void stack_clear(information *const info)
{
	info->stack_size = 0;
}


static expression_t expression_type(node *const nd)
{
	switch (node_get_type(nd))
	{
		case TIdent:
		case TIdenttoval:
		case TConst:
		case TIdenttoaddr:
		case TConstd:
		case TIdenttovald:
			return OPERAND;


		case POSTINC:
		case POSTDEC:
		case INC:
		case DEC:
		case POSTINCAT:
		case POSTDECAT:
		case INCAT:
		case DECAT:
		case POSTINCV:
		case POSTDECV:
		case INCV:
		case DECV:
		case POSTINCATV:
		case POSTDECATV:
		case INCATV:
		case DECATV:

		case UNMINUS:

		case LNOT:
		case LOGNOT:

		case POSTINCR:
		case POSTDECR:
		case INCR:
		case DECR:
		case POSTINCATR:
		case POSTDECATR:
		case INCATR:
		case DECATR:
		case POSTINCRV:
		case POSTDECRV:
		case INCRV:
		case DECRV:
		case POSTINCATRV:
		case POSTDECATRV:
		case INCATRV:
		case DECATRV:

		case UNMINUSR:
			return UNARY_OPERATION;


		case REMASS:
		case SHLASS:
		case SHRASS:
		case ANDASS:
		case EXORASS:
		case ORASS:

		case ASS:
		case PLUSASS:
		case MINUSASS:
		case MULTASS:
		case DIVASS:

		case REMASSAT:
		case SHLASSAT:
		case SHRASSAT:
		case ANDASSAT:
		case EXORASSAT:
		case ORASSAT:

		case ASSAT:
		case PLUSASSAT:
		case MINUSASSAT:
		case MULTASSAT:
		case DIVASSAT:

		case REMASSV:
		case SHLASSV:
		case SHRASSV:
		case ANDASSV:
		case EXORASSV:
		case ORASSV:

		case ASSV:
		case PLUSASSV:
		case MINUSASSV:
		case MULTASSV:
		case DIVASSV:

		case REMASSATV:
		case SHLASSATV:
		case SHRASSATV:
		case ANDASSATV:
		case EXORASSATV:
		case ORASSATV:

		case ASSATV:
		case PLUSASSATV:
		case MINUSASSATV:
		case MULTASSATV:
		case DIVASSATV:

		case LREM:
		case LSHL:
		case LSHR:
		case LAND:
		case LEXOR:
		case LOR:
		case LOGAND:
		case LOGOR:

		case EQEQ:
		case NOTEQ:
		case LLT:
		case LGT:
		case LLE:
		case LGE:
		case LPLUS:
		case LMINUS:
		case LMULT:
		case LDIV:

		case ASSR:
		case PLUSASSR:
		case MINUSASSR:
		case MULTASSR:
		case DIVASSR:

		case ASSATR:
		case PLUSASSATR:
		case MINUSASSATR:
		case MULTASSATR:
		case DIVASSATR:

		case ASSRV:
		case PLUSASSRV:
		case MINUSASSRV:
		case MULTASSRV:
		case DIVASSRV:

		case ASSATRV:
		case PLUSASSATRV:
		case MINUSASSATRV:
		case MULTASSATRV:
		case DIVASSATRV:

		case EQEQR:
		case NOTEQR:
		case LLTR:
		case LGTR:
		case LLER:
		case LGER:
		case LPLUSR:
		case LMINUSR:
		case LMULTR:
		case LDIVR:
			return BINARY_OPERATION;


		default:
			return NOT_EXPRESSION;
	}
}

static void node_recursive(information *const info, node *const nd)
{
	for (size_t i = 0; i < node_get_amount(nd); i++)
	{
		node child = node_get_child(nd, i);

		// перестановка узлов выражений
		switch (expression_type(&child))
		{
			case OPERAND:
			{
				node_info node_info;
				node_info.parent = nd;
				node_info.child = i;
				node_info.depth = 1;
				stack_push(info, node_info);
			}
			break;
			case UNARY_OPERATION:
			{
				node_info operand = stack_pop(info);

				// перестановка с операндом
				node_order(nd, i, operand.parent, operand.child);
				node child_to_order = node_get_child(operand.parent, operand.child);
				for (size_t j = 0; j < operand.depth - 1; j++)
				{
					node_order(nd, i, &child_to_order, 0);
					child_to_order = node_get_child(&child_to_order, 0);
				}

				// добавляем в стек переставленное выражение
				operand.depth++;
				stack_push(info, operand);
			}
			break;
			case BINARY_OPERATION:
			{
				node_info second_operand = stack_pop(info);
				node_info first_operand = stack_pop(info);

				// перестановка со вторым операндом
				node_order(nd, i, second_operand.parent, second_operand.child);
				node child_to_order_second = node_get_child(second_operand.parent, second_operand.child);
				for (size_t j = 0; j < second_operand.depth - 1; j++)
				{
					node_order(nd, i, &child_to_order_second, 0);
					child_to_order_second = node_get_child(&child_to_order_second, 0);
				}

				// перестановка с первым операндом
				node_order(second_operand.parent, second_operand.child, first_operand.parent, first_operand.child);
				node child_to_order_first = node_get_child(first_operand.parent, first_operand.child);
				for (size_t j = 0; j < first_operand.depth - 1; j++)
				{
					node_order(second_operand.parent, 0, &child_to_order_first, 0);
					child_to_order_first = node_get_child(&child_to_order_first, 0);
				}

				// добавляем в стек переставленное выражение
				first_operand.depth = first_operand.depth + second_operand.depth + 1;
				stack_push(info, first_operand);
			}
			break;
			case NOT_EXPRESSION:
			{
				switch (node_get_type(&child))
				{
					case TString:
					{
						const size_t N = (size_t)node_get_arg(&child, 0);
						uni_printf(info->io, "@.str%" PRIitem " = private unnamed_addr constant [%zi x i8] c\""
							, info->string_num++, N + 1);
						
						for (size_t j = 0; j < N; j++) 
						{
							const char ch = (char)node_get_arg(&child, j + 1);
							if (ch == '\n')
							{
								uni_printf(info->io, "\\0A");
							}
							else
							{
								uni_printf(info->io, "%c", ch);
							}
						}
						uni_printf(info->io, "\\00\", align 1\n");
					}
					break;
					case TPrintf:
					{
						const size_t N = (size_t)node_get_arg(&child, 0);
						// перестановка TPrintf
						// TODO: подумать, как для всех типов работать будет
						for (size_t j = 0; j < N + 1; j++)
						{
							node_swap(nd, i - j, nd, i - j - 1);
						}

						// перестановка TString
						// TODO: подумать, как для всех типов работать будет
						for (size_t j = 0; j < N; j++)
						{
							node_swap(nd, i - j, nd, i - j - 1);
						}

						info->was_printf = 1;
					}
					break;
					// если конец выражения, то очищаем стек
					case TExprend:
						stack_clear(info);
					break;
				}
			}
			break;
		}

		node_recursive(info, &child);
	}
}

static int optimize_pass(universal_io *const io, syntax *const sx)
{
	information info;
	info.io = io;
	info.string_num = 1;
	info.was_printf = 0;
	info.stack_size = 0;

	node nd = node_get_root(&sx->tree);
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		node_recursive(&info, &child);
	}

	uni_printf(io, "\n");
	if (info.was_printf)
	{
		uni_printf(io, "declare i32 @printf(i8*, ...)\n");
	}
	uni_printf(io, "\n");

	return 0;
}

static void architecture(const workspace *const ws, universal_io *const io)
{
	for (size_t i = 0; ; i++)
	{
		const char *flag = ws_get_flag(ws, i);

		if (flag == NULL || strcmp(flag, "--x86_64") == 0)
		{
			uni_printf(io, "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n");
			uni_printf(io, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
			return;
		}
		else if (strcmp(flag, "--mipsel") == 0)
		{
			uni_printf(io, "target datalayout = \"e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64\"\n");
			uni_printf(io, "target triple = \"mipsel\"\n\n");
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


int optimize_for_llvm(const workspace *const ws, universal_io *const io, syntax *const sx)
{
	if (!ws_is_correct(ws) || !out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	architecture(ws, io);
	return optimize_pass(io, sx);
}
