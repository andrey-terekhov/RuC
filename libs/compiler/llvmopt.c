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
#include "stack.h"
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
	node cur_node;									/**< Узел */
	size_t depth;									/**< Количество узлов после данного узла при перестановке */
} node_info;

typedef struct information
{
	universal_io *io;								/**< Вывод */
	syntax *sx;										/**< Структура syntax с таблицами */

	item_t string_num;								/**< Номер строки */
	item_t was_printf;								/**< Флаг наличия printf в исходном коде */

	stack nodes;									/**< Стек нод для преобразования выражений */
	stack depths;									/**< Стек глубин нод для преобразования выражений */
	size_t last_depth;								/**< Глубина стека после перестановки для пропуска переставленных нод */

	// TODO: а если в выражении вырезки есть вырезка, надо обдумать и этот случай
	size_t slice_depth;								/**< Количество узлов после OP_SLICE_IDENT */
	size_t slice_stack_size;						/**< Размер стека в начале вырезки */
} information;


static inline node_info stack_info_pop(information *const info)
{
	const item_t index = stack_pop(&info->nodes);
	const item_t operand_depth = stack_pop(&info->depths);

	if (index == ITEM_MAX || operand_depth == ITEM_MAX)
	{
		return (node_info){ .depth = SIZE_MAX };
	}

	return (node_info){ node_load(&info->sx->tree, (size_t)index), (size_t)operand_depth };
}

static inline void stack_resize(information *const info, const size_t size)
{
	vector_resize(&info->nodes, size);
	vector_resize(&info->depths, size);
}


static int transposition(node_info *const expr, node_info *const cur)
{
	if (expr == NULL || cur == NULL || !node_is_correct(&expr->cur_node) || !node_is_correct(&cur->cur_node))
	{
		system_error(transposition_not_possible);
		return -1;
	}

	node_order(&expr->cur_node, &cur->cur_node);

	node tmp;
	node_copy(&tmp, &expr->cur_node);
	node_copy(&expr->cur_node, &cur->cur_node);
	node_copy(&cur->cur_node, &tmp);

	node node_to_order;
	node_copy(&node_to_order, &expr->cur_node);
	for (size_t i = 1; i < expr->depth; i++)
	{
		node_to_order = node_get_next(&node_to_order);

		node_order(&cur->cur_node, &node_to_order);

		node_copy(&tmp, &node_to_order);
		node_copy(&node_to_order, &cur->cur_node);
		node_copy(&cur->cur_node, &tmp);
	}

	expr->depth += cur->depth;
	return 0;
}


static expression_t expression_type(node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_IDENT:
		case OP_IDENT_TO_VAL:
		case OP_CONST:
		case OP_IDENT_TO_ADDR:
		case OP_CONST_D:
		case OP_IDENT_TO_VAL_D:
		case OP_SLICE_IDENT:
			return OPERAND;


		case OP_POST_INC:
		case OP_POST_DEC:
		case OP_PRE_INC:
		case OP_PRE_DEC:
		case OP_POST_INC_AT:
		case OP_POST_DEC_AT:
		case OP_PRE_INC_AT:
		case OP_PRE_DEC_AT:
		case OP_POST_INC_V:
		case OP_POST_DEC_V:
		case OP_PRE_INC_V:
		case OP_PRE_DEC_V:
		case OP_POST_INC_AT_V:
		case OP_POST_DEC_AT_V:
		case OP_PRE_INC_AT_V:
		case OP_PRE_DEC_AT_V:

		case OP_UNMINUS:

		case OP_NOT:
		case OP_LOG_NOT:

		case OP_POST_INC_R:
		case OP_POST_DEC_R:
		case OP_PRE_INC_R:
		case OP_PRE_DEC_R:
		case OP_POST_INC_AT_R:
		case OP_POST_DEC_AT_R:
		case OP_PRE_INC_AT_R:
		case OP_PRE_DEC_AT_R:
		case OP_POST_INC_R_V:
		case OP_POST_DEC_R_V:
		case OP_PRE_INC_R_V:
		case OP_PRE_DEC_R_V:
		case OP_POST_INC_AT_R_V:
		case OP_POST_DEC_AT_R_V:
		case OP_PRE_INC_AT_R_V:
		case OP_PRE_DEC_AT_R_V:

		case OP_UNMINUS_R:

		case OP_CALL1:
			return UNARY_OPERATION;


		case OP_REM_ASSIGN:
		case OP_SHL_ASSIGN:
		case OP_SHR_ASSIGN:
		case OP_AND_ASSIGN:
		case OP_XOR_ASSIGN:
		case OP_OR_ASSIGN:

		case OP_ASSIGN:
		case OP_ADD_ASSIGN:
		case OP_SUB_ASSIGN:
		case OP_MUL_ASSIGN:
		case OP_DIV_ASSIGN:

		case OP_REM_ASSIGN_AT:
		case OP_SHL_ASSIGN_AT:
		case OP_SHR_ASSIGN_AT:
		case OP_AND_ASSIGN_AT:
		case OP_XOR_ASSIGN_AT:
		case OP_OR_ASSIGN_AT:

		case OP_ASSIGN_AT:
		case OP_ADD_ASSIGN_AT:
		case OP_SUB_ASSIGN_AT:
		case OP_MUL_ASSIGN_AT:
		case OP_DIV_ASSIGN_AT:

		case OP_REM_ASSIGN_V:
		case OP_SHL_ASSIGN_V:
		case OP_SHR_ASSIGN_V:
		case OP_AND_ASSIGN_V:
		case OP_XOR_ASSIGN_V:
		case OP_OR_ASSIGN_V:

		case OP_ASSIGN_V:
		case OP_ADD_ASSIGN_V:
		case OP_SUB_ASSIGN_V:
		case OP_MUL_ASSIGN_V:
		case OP_DIV_ASSIGN_V:

		case OP_REM_ASSIGN_AT_V:
		case OP_SHL_ASSIGN_AT_V:
		case OP_SHR_ASSIGN_AT_V:
		case OP_AND_ASSIGN_AT_V:
		case OP_XOR_ASSIGN_AT_V:
		case OP_OR_ASSIGN_AT_V:

		case OP_ASSIGN_AT_V:
		case OP_ADD_ASSIGN_AT_V:
		case OP_SUB_ASSIGN_AT_V:
		case OP_MUL_ASSIGN_AT_V:
		case OP_DIV_ASSIGN_AT_V:

		case OP_REM:
		case OP_SHL:
		case OP_SHR:
		case OP_AND:
		case OP_XOR:
		case OP_OR:
		case OP_LOG_AND:
		case OP_LOG_OR:

		case OP_EQ:
		case OP_NE:
		case OP_LT:
		case OP_GT:
		case OP_LE:
		case OP_GE:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:

		case OP_ASSIGN_R:
		case OP_ADD_ASSIGN_R:
		case OP_SUB_ASSIGN_R:
		case OP_MUL_ASSIGN_R:
		case OP_DIV_ASSIGN_R:

		case OP_ASSIGN_AT_R:
		case OP_ADD_ASSIGN_AT_R:
		case OP_SUB_ASSIGN_AT_R:
		case OP_MUL_ASSIGN_AT_R:
		case OP_DIV_ASSIGN_AT_R:

		case OP_ASSIGN_R_V:
		case OP_ADD_ASSIGN_R_V:
		case OP_SUB_ASSIGN_R_V:
		case OP_MUL_ASSIGN_R_V:
		case OP_DIV_ASSIGN_R_V:

		case OP_ASSIGN_AT_R_V:
		case OP_ADD_ASSIGN_AT_R_V:
		case OP_SUB_ASSIGN_AT_R_V:
		case OP_MUL_ASSIGN_AT_R_V:
		case OP_DIV_ASSIGN_AT_R_V:

		case OP_EQ_R:
		case OP_NE_R:
		case OP_LT_R:
		case OP_GT_R:
		case OP_LE_R:
		case OP_GE_R:
		case OP_ADD_R:
		case OP_SUB_R:
		case OP_MUL_R:
		case OP_DIV_R:
			return BINARY_OPERATION;


		default:
			return NOT_EXPRESSION;
	}
}

static int node_recursive(information *const info, node *const nd)
{
	int has_error = 0;

	if (info->slice_depth != 0)
	{
		info->slice_depth++;
	}

	for (size_t i = 0; i < node_get_amount(nd); i++)
	{
		node child = node_get_child(nd, i);

		// Очищаем полностью стек, если родитель -- блок
		if (node_get_type(nd) == OP_BLOCK)
		{
			stack_resize(info, 0);
		}

		switch (node_get_type(&child))
		{
			case OP_STRING:
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
			case OP_PRINTF:
			{
				size_t N = (size_t)node_get_arg(&child, 0);
				// перестановка OP_PRINTF
				for (size_t j = 0; j < N + 1; j++)
				{
					const node fst = node_get_child(nd, i - j);
					const node snd = node_get_child(nd, i - j - 1);
					node_swap(&fst, &snd);

					// TODO: пока только для двумерных вырезок, потом надо подумать
					if (node_get_type(&snd) == OP_IDENT_TO_VAL_D
						|| (node_get_type(&snd) == OP_SLICE_IDENT
						&& (node_get_arg(&snd, 1) == mode_float
						|| mode_get(info->sx, (size_t)node_get_arg(&snd, 1) + 1) == mode_float)))
					{
						N--;
					}
				}

				// перестановка OP_STRING
				for (size_t j = 0; j < N; j++)
				{
					const node fst = node_get_child(nd, i - j);
					const node snd = node_get_child(nd, i - j - 1);
					node_swap(&fst, &snd);
				}

				node nd_printf = node_get_child(nd, i - N - 1);
				node_set_arg(&nd_printf, 0, N);

				info->was_printf = 1;
			}
			break;
			case OP_EXPR_END:
			{
				if (info->slice_depth != 0)
				{
					// если вырезка не переставлена, то надо частично очистить стек и изменить глубину OP_SLICE_IDENT
					node nd_expr_end = node_get_child(&child, 0);
					if (node_get_type(&nd_expr_end) == OP_SLICE)
					{
						break;
					}

					stack_resize(info, info->slice_stack_size);

					node_info slice_info = stack_info_pop(info);
					has_error |= slice_info.depth == SIZE_MAX ? -1 : 0;
					if (has_error)
					{
						return has_error;
					}

					slice_info.depth = info->slice_depth;
					has_error |= stack_push(&info->nodes, node_save(&slice_info.cur_node));
					has_error |= stack_push(&info->depths, slice_info.depth);
					info->slice_depth = 0;
				}
				else if (info->slice_depth == 0 && info->last_depth > 1)
				{
					info->last_depth--;
				}
			}
			break;

			default:
			{
				// если уже в переставленных, то с ними ничего делать не надо
				if (info->last_depth > 1)
				{
					info->last_depth--;
					break;
				}

				node_info nd_info = { child, 1 };

				// перестановка узлов выражений
				switch (expression_type(&child))
				{
					case OPERAND:
					{
						has_error |= stack_push(&info->nodes, node_save(&child));
						has_error |= stack_push(&info->depths, 1);
					}
					break;
					case UNARY_OPERATION:
					{
						node_info operand = stack_info_pop(info);

						node parent = node_get_parent(&child);
						if (node_get_type(&parent) == OP_ADDR_TO_VAL)
						{
							operand.depth++;
						}

						// перестановка с операндом
						has_error |= transposition(&operand, &nd_info);

						if (node_get_type(&operand.cur_node) == OP_CALL1)
						{
							node tmp;
							node_copy(&tmp, &child);
							while (node_get_type(&tmp) != OP_CALL2)
							{
								node_set_next(&tmp);
								operand.depth++;
							}
						}
						info->last_depth = operand.depth;

						// добавляем в стек переставленное выражение
						has_error |= stack_push(&info->nodes, node_save(&operand.cur_node));
						has_error |= stack_push(&info->depths, operand.depth);
					}
					break;
					case BINARY_OPERATION:
					{
						node_info second = stack_info_pop(info);
						node_info first = stack_info_pop(info);

						node parent = node_get_parent(&child);
						if (node_get_type(&parent) == OP_ADDR_TO_VAL)
						{
							second.depth++;
						}

						// перестановка со вторым операндом
						has_error |= transposition(&second, &nd_info);

						parent = node_get_parent(&second.cur_node);
						if (node_get_type(&parent) == OP_AD_LOG_OR
							|| node_get_type(&parent) == OP_AD_LOG_AND
							|| node_get_type(&parent) == OP_ADDR_TO_VAL)
						{
							first.depth++;
						}

						// перестановка с первым операндом
						has_error |= transposition(&first, &second);
						info->last_depth = first.depth;

						// добавляем в стек переставленное выражение
						has_error |= stack_push(&info->nodes, node_save(&first.cur_node));
						has_error |= stack_push(&info->depths, first.depth);
					}
					break;
					case NOT_EXPRESSION:
						break;
				}
			}
			break;
		}

		// если встретили вырезку, надо запомнить состояние стека для дальнейшего восстановления
		// и установить начальную глубину вырезки
		if (node_get_type(&child) == OP_SLICE_IDENT && info->last_depth <= 1)
		{
			info->slice_depth = 1;
			info->slice_stack_size = stack_size(&info->nodes);
		}

		if (has_error || node_recursive(info, &child))
		{
			return -1;
		}
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

	uni_printf(info->io, "\n");
	if (info->was_printf)
	{
		uni_printf(info->io, "declare i32 @printf(i8*, ...)\n");
	}
	uni_printf(info->io, "\n");

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

	information info;
	info.io = io;
	info.sx = sx;
	info.string_num = 1;
	info.was_printf = 0;
	info.last_depth = 1;
	info.slice_depth = 0;
	info.slice_stack_size = 0;

	info.nodes = stack_create(MAX_STACK_SIZE);
	info.depths = stack_create(MAX_STACK_SIZE);

	architecture(ws, io);

	const int ret = optimize_pass(&info);

	stack_clear(&info.nodes);
	stack_clear(&info.depths);
	return ret;
}
