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
#include "codes.h"
#include "tree.h"
#include "uniprinter.h"
#include <string.h>


typedef struct expr_node_info
{
	node *parent;						/**< Родитель узла */ //проблема в указателе при выходе из node_recursive память очищается и он теряется
	size_t child_num;					/**< Номер ребёнка, которым является узел */
	size_t node_num;					/**< Количество узлов после данного узла при перестановке */
} expr_node_info;

typedef struct expr_stack
{
	expr_node_info stack[512];			/**< Стек */
	size_t stack_size;					/**< Размер стека */
} expr_stack;

typedef struct information
{
	universal_io *io;					/**< Вывод */

	item_t string_num;					/**< Номер строки */
	item_t was_printf;					/**< Флаг наличия printf в исходном коде */
	expr_stack stack;					/**< Стек для преобразования выражений */
} information;


static void expr_stack_push(information *const info, expr_node_info const expr_nd)
{
	info->stack.stack[info->stack.stack_size++] = expr_nd;
}

static expr_node_info expr_stack_pop(information *const info)
{
	return info->stack.stack[--info->stack.stack_size];
}

static void expr_stack_clear(information *const info)
{
	info->stack.stack_size = 0;
}

static int is_operand(node *const nd)
{
	switch (node_get_type(nd))
	{
		case TIdent:
		case TIdenttoval:
		case TConst:
			return 1;
	}

	return 0;
}

static int is_expression(node *const nd)
{
	switch (node_get_type(nd))
	{
		// бинарные операции
		case ASSV:
		case LPLUS:
			return 2;
	}

	return 0;
}

static void node_recursive(information *const info, node *const nd, syntax *const sx)
{
	// printf("children = %zi tc = %zi argc = %zi type = %zi\n", nd->children, nd->type, nd->argc, node_get_type(nd));
	for (size_t i = 0; i < node_get_amount(nd); i++)
	{
		node child = node_get_child(nd, i);

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
		}

		// перестановка узлов выражений
		if (is_operand(&child))
		{
			expr_node_info node_info = {nd, i, 1};
			node_copy(node_info.parent, nd);
			expr_stack_push(info, node_info);
		}
		else if (is_expression(&child) == 2)
		{
			expr_node_info second_operand = expr_stack_pop(info);
			expr_node_info first_operand = expr_stack_pop(info);

			// отладочные печати -- стек работает исправно
			// node child_second = node_get_child(second_operand.parent, second_operand.child_num); 
			// printf("child_second %zi\n", child_second.children);
			// node child_first = node_get_child(first_operand.parent, first_operand.child_num); 
			// printf("child_first %zi\n", child_first.children);
			// printf("operation %zi\n", child.children);

			// это работает только для простых выражений, нужно задействовать ещё node_num
			// printf("\nchange1:\n");
			// printf("first node parent to swap: children = %zi tc = %zi argc = %zi type = %zi child_num = %zi\n", 
			// 	nd->children, nd->type, nd->argc, node_get_type(nd), i);
			// node child1 = node_get_child(nd, i);
			// printf("first node child to swap: tc = %zi argc = %zi type = %zi \n", 
			// 	child1.type, child1.argc, node_get_type(&child1));
			// printf("second node parent to swap: children = %zi tc = %zi argc = %zi type = %zi child_num = %zi\n", 
			// 	second_operand.parent->children, second_operand.parent->type, second_operand.parent->argc, 
			// 	node_get_type(second_operand.parent), second_operand.child_num);
			// node child2 = node_get_child(second_operand.parent, second_operand.child_num);
			// printf("second node child to swap: tc = %zi argc = %zi type = %zi \n", 
			// 	child2.type, child2.argc, node_get_type(&child2));

			node_order(nd, i, second_operand.parent, second_operand.child_num);

			// printf("after change1:\n");
			// printf("first node parent to swap: children = %zi tc = %zi argc = %zi type = %zi child_num = %zi\n", 
			// 	nd->children, nd->type, nd->argc, node_get_type(nd), i);
			// node child11 = node_get_child(nd, i);
			// printf("first node child to swap: tc = %zi argc = %zi type = %zi \n", 
			// 	child11.type, child11.argc, node_get_type(&child11));
			// printf("second parent node to swap: children = %zi tc = %zi argc = %zi type = %zi child_num = %zi\n", 
			// 	second_operand.parent->children, second_operand.parent->type, second_operand.parent->argc, 
			// 	node_get_type(second_operand.parent), second_operand.child_num);
			// node child22 = node_get_child(second_operand.parent, second_operand.child_num);
			// printf("second node child to swap: children = %zi tc = %zi argc = %zi type = %zi \n", 
			// 	child22.children, child.type, child22.argc, node_get_type(&child22));

			// // эспериментально-отладочный if
			// if (second_operand.node_num == 3)
			// {
			// 	node child1 = node_get_child(second_operand.parent, second_operand.child_num); 
			// 	node_order(nd, i, &child1, 0); // в выражениях есть только один потомок
			// 	node child2 = node_get_child(&child1, 0); // в выражениях есть только один потомок
			// 	node_order(nd, i, &child2, 0); // в выражениях есть только один потомок
			// }
			node child_to_order_second = node_get_child(second_operand.parent, second_operand.child_num);
			for (size_t j = 0; j < second_operand.node_num - 1; j++)
			{
				node_order(nd, i, &child_to_order_second, 0);
				child_to_order_second = node_get_child(&child_to_order_second, 0);
			}

			node_order(second_operand.parent, second_operand.child_num, first_operand.parent, first_operand.child_num);

			// TODO: сделать перестановки при сложном первом аргументе

			first_operand.node_num = first_operand.node_num + second_operand.node_num + 1;
			expr_stack_push(info, first_operand);
		}

		node_recursive(info, &child, sx);

		if (node_get_type(nd))
		{
			expr_stack_clear(info);
		}
	}
}

static int optimize_pass(universal_io *const io, syntax *const sx)
{
	information info;
	info.io = io;
	info.string_num = 1;
	info.was_printf = 0;
	info.stack.stack_size = 0;

	node nd = node_get_root(&sx->tree);
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		node_recursive(&info, &child, sx);
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
