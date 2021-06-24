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
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"
#include <string.h>


#define MAX_STACK_SIZE	512
int counter = 0, counter2 = 0;


typedef enum EXPRESSION
{
	OPERAND,
	UNARY_OPERATION,
	BINARY_OPERATION,
	NOT_EXPRESSION,
} expression_t;

typedef struct node_info
{
	node *ref_node;										/**< Ссылка на узел */
	size_t depth;										/**< Количество узлов после данного узла при перестановке */
} node_info;

typedef struct information
{
	universal_io *io;									/**< Вывод */
	syntax *sx;											/**< Структура syntax с таблицами */

	item_t string_num;									/**< Номер строки */
	item_t was_printf;									/**< Флаг наличия printf в исходном коде */

	node_info stack[MAX_STACK_SIZE];					/**< Стек для преобразования выражений */
	size_t stack_size;									/**< Размер стека */
	size_t skip_counter;								/**< Счётчик для пропуска нод после перестановки */
	// TODO: а если в выражении вырезки есть вырезка, надо обдумать и этот случай
	size_t slice_depth;									/**< Количество узлов после OP_SLICE_IDENT */
	size_t slice_stack_size;							/**< Размер стека в начале вырезки */
} information;


static inline void struct_declaration(information *const info, size_t mode_ref)
{
	uni_printf(info->io, "%%struct_opt.%zi = type { ", mode_ref);

	const item_t n = mode_get(info->sx, mode_ref + 2);
	for (item_t i = 0; i < n; i += 2)
	{
		switch (mode_get(info->sx, mode_ref + 3 + i))
		{
			case mode_integer:
				uni_printf(info->io, "%si32", i == 0 ? "" : ", ");
				break;

			default:
			break;
		}
	}

	uni_printf(info->io, " }\n");
}


static inline size_t stack_peek_depth(information *const info)
{
	return info->stack[info->stack_size - 1].depth;
}

static inline int stack_push(information *const info, node_info *const nd)
{
	if (info->stack_size == MAX_STACK_SIZE)
	{
		return -1;
	}

	info->stack[info->stack_size++] = *nd;
	return 0;
}

static inline node_info *stack_pop(information *const info)
{
	if (info->stack_size == 0)
	{
		return NULL;
	}

	return &info->stack[--info->stack_size];
}

static inline void stack_resize(information *const info, const int size)
{
	info->stack_size = size;
}


static int transposition(node_info *const expr, node_info *const cur, information *const info)
{
	if (expr == NULL || cur == NULL)
	{
		system_error(transposition_not_possible);
		return -1;
	}
	counter2++;
	// if (counter2 == 3)
	// {
	// 	tables_and_tree("before3.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
	// }

	node_order(expr->ref_node, cur->ref_node);
	node tmp;
	node_copy(&tmp, expr->ref_node);
	node_copy(expr->ref_node, cur->ref_node);
	node_copy(cur->ref_node, &tmp);

	// if (counter2 == 3)
	// {
	// 	tables_and_tree("after3.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
	// }

	// node *temp = expr->ref_node;
	node node_to_order;
	node_copy(&node_to_order, expr->ref_node);
	for (size_t i = 1; i < expr->depth; i++)
	{
		// if (counter2 == 3 && i == 2)
		// {
		// 	printf("before node_get_child:\n");
		// 	printf("cur->ref_node type =  %i\n", node_get_type(cur->ref_node));
		// 	printf("temp type = %i\n\n", node_get_type(temp));
		// }

		node_to_order = node_get_child(&node_to_order, 0);

		// if (counter2 == 3 && i == 2)
		// {
		// 	printf("after node_get_child:\n");
		// 	printf("cur->ref_node type = %i\n", node_get_type(cur->ref_node));
		// 	printf("temp type = %i\n\n", node_get_type(temp));
		// }
		// temp = &node_to_order;
		// node_copy(temp, &node_to_order);
		// if (counter2 == 3 && i == 1)
		// {
		// 	tables_and_tree("before.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
		// 	printf("before node_order:\n");
		// 	printf("print tree in before.txt\n");
		// 	printf("cur->ref_node type = %i\n", node_get_type(cur->ref_node));
		// 	printf("temp type = %i\n\n", node_get_type(temp));
		// }

		node_order(cur->ref_node, &node_to_order);

		// if (counter2 == 3 && i == 1)
		// {
		// 	tables_and_tree("after.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
		// 	printf("after node_order:\n");
		// 	printf("print tree in after.txt\n");
		// 	printf("cur->ref_node type = %i\n", node_get_type(cur->ref_node));
		// 	printf("temp type = %i\n\n", node_get_type(temp));
		// }

		// tmp = temp;
		node_copy(&tmp, &node_to_order);
		node_copy(&node_to_order, cur->ref_node);
		node_copy(cur->ref_node, &tmp);
		// temp = cur->ref_node;
		// cur->ref_node = tmp;

		// if (counter2 == 3 && i == 1)
		// {
		// 	tables_and_tree("after4.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
		// 	printf("after pointers permutation:\n");
		// 	printf("cur->ref_node type = %i\n", node_get_type(cur->ref_node));
		// 	printf("temp type = %i\n\n", node_get_type(temp));
		// }

		// temp = node_get_child(&temp, 0);
	}

	expr->depth += cur->depth;
	// cur->depth += expr->depth;
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

	if (info->slice_depth != 0 && info->skip_counter <= 1)
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
					node child_to_swap1 = node_get_child(nd, i - j);
					node child_to_swap2 = node_get_child(nd, i - j - 1);
					node_swap(&child_to_swap1, &child_to_swap2);

					// TODO: пока только для двумерных вырезок, потом надо подумать
					if (node_get_type(&child_to_swap2) == OP_IDENT_TO_VAL_D
						|| (node_get_type(&child_to_swap2) == OP_SLICE_IDENT
						&& (node_get_arg(&child_to_swap2, 1) == mode_float
						|| mode_get(info->sx, node_get_arg(&child_to_swap2, 1) + 1) == mode_float)))
					{
						N--;
					}
				}

				// перестановка OP_STRING
				for (size_t j = 0; j < N; j++)
				{
					node child_to_swap1 = node_get_child(nd, i - j);
					node child_to_swap2 = node_get_child(nd, i - j - 1);
					node_swap(&child_to_swap1, &child_to_swap2);
				}

				node nd_printf = node_get_child(nd, i - N - 1);
				node_set_arg(&nd_printf, 0, N);

				info->was_printf = 1;
			}
			break;

			case OP_EXPR_END:
			{
				if (info->slice_depth == 0)
				{
					// При обходе после перестановки нужно учитывать и OP_EXPR_END
					// if (info->skip_counter > 1)
					// {
					// 	info->skip_counter--;
					// }
				}
				else if (info->skip_counter <= 1)
				{
					// если вырезка не переставлена, то надо частично очистить стек и изменить глубину OP_SLICE_IDENT
					node nd_expr_end = node_get_child(&child, 0);
					if (node_get_type(&nd_expr_end) == OP_SLICE)
					{
						break;
					}

					stack_resize(info, info->slice_stack_size);

					node_info *slice_info = stack_pop(info);

					slice_info->depth = info->slice_depth;
					stack_push(info, slice_info);
					info->slice_depth = 0;
				}
			}
			break;

			case OP_DECL_ID:
			{
				const size_t elem_type = (size_t)node_get_arg(&child, 1);

				if (mode_get(info->sx, elem_type) == mode_struct)
				{
					struct_declaration(info, elem_type);
				}
			}
			break;

			default:
			{
				// если узел в переставленном выражении, то ничего делать не надо
				// if (info->skip_counter > 1)
				// {
				// 	printf("here %i\n", node_get_type(&child));
				// 	info->skip_counter--;
				// 	break;
				// }

				node_info nd_info = { &child, 1 };

				// перестановка узлов выражений
				switch (expression_type(&child))
				{
					case OPERAND:
						stack_push(info, &nd_info);
						break;
					case UNARY_OPERATION:
					{
						node_info *operand = stack_pop(info);

						if (node_get_type(nd_info.ref_node) == OP_ADDR_TO_VAL)
						{
							node_info log_info = { nd_info.ref_node, 1 };
							has_error |= transposition(&nd_info, &log_info, info);
						}

						// перестановка с операндом
						has_error |= transposition(operand, &nd_info, info);

						// TODO: тут, по-моему, ужасный костыль (а может и нет, вроде нормально работает)
						// TODO: проблемы, если вызов функции в выражении
						if (node_get_type(&child) == OP_CALL1)
						{
							node tmp = child;
							while (node_get_type(&tmp) != OP_CALL2)
							{
								node_set_next(&tmp);
								operand->depth++;
							}
							operand->depth--;
						}

						// добавляем в стек переставленное выражение
						has_error |=  stack_push(info, operand);

						info->skip_counter = stack_peek_depth(info);
					}
					break;
					case BINARY_OPERATION:
					{
						node_info *second = stack_pop(info);
						node_info *first = stack_pop(info);
						counter++;

						if (node_get_type(nd_info.ref_node) == OP_ADDR_TO_VAL)
						{
							node_info log_info = { nd_info.ref_node, 1 };
							has_error |= transposition(&nd_info, &log_info, info);
						}

						// if (counter == 2)
						// {
						// 	tables_and_tree("before2.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
						// }
						// перестановка со вторым операндом
						has_error |= transposition(second, &nd_info, info);
						// if (counter == 2)
						// {
						// 	tables_and_tree("after2.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
						// }


						// надо переставить second с родителем
						if (node_get_type(second->ref_node) == OP_AD_LOG_OR || node_get_type(second->ref_node) == OP_AD_LOG_AND
							|| node_get_type(second->ref_node) == OP_ADDR_TO_VAL)
						{
							node_info log_info = { second->ref_node, 1 };
							has_error |= transposition(second, &log_info, info);
						}

						// if (counter == 1)
						// {
						// 	tables_and_tree("before1.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
						// }
						// перестановка с первым операндом
						has_error |= transposition(first, second, info);

						// if (counter == 1)
						// {
						// 	tables_and_tree("after1.txt", &(info->sx->identifiers), &(info->sx->modes), &(info->sx->tree));
						// 	printf("first->depth = %i\n", first->depth);
						// }


						// добавляем в стек переставленное выражение
						has_error |= stack_push(info, first);

						info->skip_counter = stack_peek_depth(info);
						// printf("info->skip_counter = %i\n", info->skip_counter);
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
		if (node_get_type(&child) == OP_SLICE_IDENT && info->skip_counter <= 1)
		{
			info->slice_depth = 1;
			info->slice_stack_size = info->stack_size;
		}

		if (has_error || node_recursive(info, &child))
		{
			return has_error;
		}
	}

	return 0;
}

static int optimize_pass(universal_io *const io, syntax *const sx)
{
	information info;
	info.io = io;
	info.sx = sx;
	info.string_num = 1;
	info.was_printf = 0;
	info.stack_size = 0;
	info.skip_counter = 1;
	info.slice_depth = 0;
	info.slice_stack_size = 0;

	node nd = node_get_root(&sx->tree);
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		if (node_recursive(&info, &child))
		{
			return -1;
		}
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
