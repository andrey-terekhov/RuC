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

#include "codegen.h"
#include <stdlib.h>
#include "codes.h"
#include "errors.h"
#include "item.h"
#include "instructions.h"
#include "nodes.h"
#include "stack.h"
#include "old_tree.h"
#include "uniprinter.h"
#include "utf8.h"


const char *const DEFAULT_CODES = "codes.txt";

const size_t MAX_MEM_SIZE = 100000;
const size_t MAX_STACK_SIZE = 256;


/** Virtual machine environment */
typedef struct virtual
{
	syntax *sx;						/**< Syntax structure */

	vector memory;					/**< Memory table */
	vector processes;				/**< Init processes table */
	stack stk;						/**< Stack for logic operations */

	vector identifiers;				/**< Local identifiers table */
	vector representations;			/**< Local representations table */

	size_t max_threads;				/**< Max threads count */

	size_t addr_cond;				/**< Condition address */
	size_t addr_case;				/**< Case operator address */
	size_t addr_break;				/**< Break operator address */

	item_status target;				/**< Target tables item type */
} virtual;


static void block(virtual *const vm, node *const nd);


static inline void mem_increase(virtual *const vm, const size_t size)
{
	vector_increase(&vm->memory, size);
}

static inline void mem_add(virtual *const vm, const item_t value)
{
	vector_add(&vm->memory, value);
}

static inline void mem_set(virtual *const vm, const size_t index, const item_t value)
{
	vector_set(&vm->memory, index, value);
}

static inline item_t mem_get(const virtual *const vm, const size_t index)
{
	return vector_get(&vm->memory, index);
}

static inline size_t mem_size(const virtual *const vm)
{
	return vector_size(&vm->memory);
}


static inline void proc_set(virtual *const vm, const size_t index, const item_t value)
{
	vector_set(&vm->processes, index, value);
}

static inline item_t proc_get(const virtual *const vm, const size_t index)
{
	return vector_get(&vm->processes, index);
}


static void addr_begin_condition(virtual *const vm, const size_t addr)
{
	while (vm->addr_cond != addr)
	{
		const size_t ref = (size_t)mem_get(vm, vm->addr_cond);
		mem_set(vm, vm->addr_cond, (item_t)addr);
		vm->addr_cond = ref;
	}
}

static void addr_end_condition(virtual *const vm)
{
	while (vm->addr_cond)
	{
		const size_t ref = (size_t)mem_get(vm, vm->addr_cond);
		mem_set(vm, vm->addr_cond, (item_t)mem_size(vm));
		vm->addr_cond = ref;
	}
}

static void addr_end_break(virtual *const vm)
{
	while (vm->addr_break)
	{
		const size_t ref = (size_t)mem_get(vm, vm->addr_break);
		mem_set(vm, vm->addr_break, (item_t)mem_size(vm));
		vm->addr_break = ref;
	}
}


static void final_operation(virtual *const vm, node *const nd)
{
	node_t op = node_get_type(nd);
	while (op >= FINAL_OPERATION_START && op <= FINAL_OPERATION_END)
	{
		if (op != ND_NULL)
		{
			if (op == ND_ADLOGOR)
			{
				mem_add(vm, IC__DOUBLE);
				mem_add(vm, IC_BNE0);
				stack_push(&vm->stk, (item_t)mem_size(vm));
				mem_increase(vm, 1);
			}
			else if (op == ND_ADLOGAND)
			{
				mem_add(vm, IC__DOUBLE);
				mem_add(vm, IC_BE0);
				stack_push(&vm->stk, (item_t)mem_size(vm));
				mem_increase(vm, 1);
			}
			else
			{
				mem_add(vm, node_to_instruction(op));
				if (op == ND_LOGOR || op == ND_LOGAND)
				{
					mem_set(vm, (size_t)stack_pop(&vm->stk), (item_t)mem_size(vm));
				}
				else if (op == ND_COPY00 || op == ND_COPYST)
				{
					mem_add(vm, node_get_arg(nd, 0)); // d1
					mem_add(vm, node_get_arg(nd, 1)); // d2
					mem_add(vm, node_get_arg(nd, 2)); // длина
				}
				else if (op == ND_COPY01 || op == ND_COPY10 || op == ND_COPY0ST || op == ND_COPY0STASSIGN)
				{
					mem_add(vm, node_get_arg(nd, 0)); // d1
					mem_add(vm, node_get_arg(nd, 1)); // длина
				}
				else if (op == ND_COPY11 || op == ND_COPY1ST || op == ND_COPY1STASSIGN)
				{
					mem_add(vm, node_get_arg(nd, 0)); // длина
				}
				else if (node_is_assignment_operator(op))
				{
					mem_add(vm, node_get_arg(nd, 0));
				}
			}
		}

		node_set_next(nd);
		op = node_get_type(nd);
	}
}

/**
 *	Expression generation
 *
 *	@param	vm		Virtual machine environment
 *	@param	mode	@c -1 for expression on the same node,
 *					@c  0 for usual expression,
 *					@c  1 for expression in condition
 */
static void expression(virtual *const vm, node *const nd, int mode)
{
	if (mode != -1)
	{
		node_set_next(nd);
	}

	while (node_get_type(nd) != ND_EXPRESSION_END)
	{
		const item_t operation = node_get_type(nd);
		int was_operation = 1;

		switch (operation)
		{
			case ND_IDENT:
				break;
			case ND_IDENTTOADDR:
			{
				mem_add(vm, IC_LA);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case ND_IDENTTOVAL:
			{
				mem_add(vm, IC_LOAD);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case ND_IDENTTOVALD:
			{
				mem_add(vm, IC_LOADD);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case ND_ADDRTOVAL:
				mem_add(vm, IC_LAT);
				break;
			case ND_ADDRTOVALD:
				mem_add(vm, IC_LATD);
				break;
			case ND_CONST:
			{
				mem_add(vm, IC_LI);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case ND_CONSTD:
			{
				mem_add(vm, IC_LID);
				mem_add(vm, node_get_arg(nd, 0));
				mem_add(vm, node_get_arg(nd, 1));
			}
			break;
			case ND_STRING:
			case ND_STRINGD:
			{
				mem_add(vm, IC_LI);
				const size_t reserved = mem_size(vm) + 4;
				mem_add(vm, (item_t)reserved);
				mem_add(vm, IC_B);
				mem_increase(vm, 2);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					if (operation == ND_STRING)
					{
						mem_add(vm, node_get_arg(nd, (size_t)i + 1));
					}
					else
					{
						mem_add(vm, node_get_arg(nd, 2 * (size_t)i + 1));
						mem_add(vm, node_get_arg(nd, 2 * (size_t)i + 2));
					}
				}

				mem_set(vm, reserved - 1, N);
				mem_set(vm, reserved - 2, (item_t)mem_size(vm));
			}
			break;
			case ND_ARRAY_INIT:
			{
				const item_t N = node_get_arg(nd, 0);

				mem_add(vm, IC_BEGINIT);
				mem_add(vm, N);

				for (item_t i = 0; i < N; i++)
				{
					expression(vm, nd, 0);
				}
			}
			break;
			case ND_STRUCT_INIT:
			{
				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					expression(vm, nd, 0);
				}
			}
			break;
			case ND_SLICEIDENT:
			{
				mem_add(vm, IC_LOAD); // параметры - смещение идента и тип элемента
				mem_add(vm, node_get_arg(nd, 0)); // продолжение в след case
			}
			case ND_SLICE: // параметр - тип элемента
			{
				item_t type = node_get_arg(nd, operation == ND_SLICE ? 0 : 1);

				expression(vm, nd, 0);
				mem_add(vm, IC_SLICE);
				mem_add(vm, (item_t)size_of(vm->sx, type));
				if (type > 0 && mode_get(vm->sx, (size_t)type) == mode_array)
				{
					mem_add(vm, IC_LAT);
				}
			}
			break;
			case ND_SELECT:
			{
				mem_add(vm, IC_SELECT); // SELECT field_displ
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case ND_PRINT:
			{
				mem_add(vm, IC_PRINT);
				mem_add(vm, node_get_arg(nd, 0)); // type
			}
			break;
			case ND_CALL1:
			{
				mem_add(vm, IC_CALL1);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					expression(vm, nd, 0);
				}
			}
			break;
			case ND_CALL2:
			{
				mem_add(vm, IC_CALL2);
				mem_add(vm, ident_get_displ(vm->sx, (size_t)node_get_arg(nd, 0)));
			}
			break;
			default:
				was_operation = 0;
				break;
		}

		if (was_operation)
		{
			node_set_next(nd);
		}

		final_operation(vm, nd);

		if (node_get_type(nd) == ND_CONDITIONAL)
		{
			if (mode == 1)
			{
				return;
			}

			size_t addr = 0;
			do
			{
				mem_add(vm, IC_BE0);
				const size_t addr_else = mem_size(vm);
				mem_increase(vm, 1);

				expression(vm, nd, 0); // then
				mem_add(vm, IC_B);
				mem_add(vm, (item_t)addr);
				addr = mem_size(vm) - 1;
				mem_set(vm, addr_else, (item_t)mem_size(vm));

				expression(vm, nd, 1); // else или cond
			} while (node_get_type(nd) == ND_CONDITIONAL);

			while (addr)
			{
				const size_t ref = (size_t)mem_get(vm, addr);
				mem_set(vm, addr, (item_t)mem_size(vm));
				addr = ref;
			}

			final_operation(vm, nd);
		}
	}
}

static void structure(virtual *const vm, node *const nd)
{
	if (node_get_type(nd) == ND_STRUCT_INIT)
	{
		const item_t N = node_get_arg(nd, 0);
		node_set_next(nd);

		for (item_t i = 0; i < N; i++)
		{
			structure(vm, nd);
			node_set_next(nd); // TExprend
		}
	}
	else
	{
		expression(vm, nd, -1);
	}
}

static void identifier(virtual *const vm, node *const nd)
{
	const item_t old_displ = node_get_arg(nd, 0);
	const item_t type = node_get_arg(nd, 1);
	const item_t N = node_get_arg(nd, 2);

	/*
	 *	@param	all		Общее кол-во слов в структуре:
	 *						@c 0 нет инициализатора,
	 *						@c 1 есть инициализатор,
	 *						@c 2 есть инициализатор только из строк
	 */
	const item_t all = node_get_arg(nd, 3);
	const item_t process = node_get_arg(nd, 4);

	/*
	 *	@param	usual	Для массивов:
	 *						@c 0 с пустыми границами,
	 *						@c 1 без пустых границ
	 */
	const item_t usual = node_get_arg(nd, 5);
	const item_t instruction = node_get_arg(nd, 6);


	if (N == 0) // Обычная переменная int a; или struct point p;
	{
		if (process)
		{
			mem_add(vm, IC_STRUCTWITHARR);
			mem_add(vm, old_displ);
			mem_add(vm, proc_get(vm, (size_t)process));
		}
		if (all) // int a = или struct{} a =
		{
			if (type > 0 && mode_get(vm->sx, (size_t)type) == mode_struct)
			{
				node_set_next(nd);
				structure(vm, nd);

				mem_add(vm, IC_COPY0STASS);
				mem_add(vm, old_displ);
				mem_add(vm, all); // Общее количество слов
			}
			else
			{
				expression(vm, nd, 0);

				mem_add(vm, type == mode_float ? IC_ASSRV : IC_ASSV);
				mem_add(vm, old_displ);
			}
		}
	}
	else // Обработка массива int a[N1]...[NN] =
	{
		const item_t length = (item_t)size_of(vm->sx, type);

		mem_add(vm, IC_DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
		mem_add(vm, all == 0 ? N : abs((int)N) - 1);
		mem_add(vm, length);
		mem_add(vm, old_displ);
		mem_add(vm, proc_get(vm, (size_t)process));
		mem_add(vm, usual);
		mem_add(vm, all);
		mem_add(vm, instruction);

		if (all) // all == 1, если есть инициализация массива
		{
			expression(vm, nd, 0);

			mem_add(vm, IC_ARRINIT); // ARRINIT N d all displ usual
			mem_add(vm, abs((int)N));
			mem_add(vm, length);
			mem_add(vm, old_displ);
			mem_add(vm, usual);	// == 0 с пустыми границами
								// == 1 без пустых границ и без инициализации
		}
	}
}

static int declaration(virtual *const vm, node *const nd)
{
	switch (node_get_type(nd))
	{
		case ND_DECL_ARR:
		{
			const item_t N = node_get_arg(nd, 0);
			for (item_t i = 0; i < N; i++)
			{
				expression(vm, nd, 0);
			}
		}
		break;
		case ND_DECL_ID:
			identifier(vm, nd);
			break;

		case ND_DECL_STRUCT:
		{
			mem_add(vm, IC_B);
			mem_add(vm, 0);
			proc_set(vm, (size_t)node_get_arg(nd, 0), (item_t)mem_size(vm));
		}
		break;
		case ND_DECL_STRUCT_END:
		{
			const size_t num_proc = (size_t)node_get_arg(nd, 0);

			mem_add(vm, IC_STOP);
			mem_set(vm, (size_t)proc_get(vm, num_proc) - 1, (item_t)mem_size(vm));
		}
		break;

		default:
			return -1;
	}

	return 0;
}

static void compress_ident(virtual *const vm, const size_t ref)
{
	if (vector_get(&vm->sx->identifiers, ref) == ITEM_MAX)
	{
		mem_add(vm, ident_get_repr(vm->sx, ref));
		return;
	}

	const item_t new_ref = (item_t)vector_size(&vm->identifiers) - 1;
	vector_add(&vm->identifiers, (item_t)vector_size(&vm->representations) - 2);
	vector_add(&vm->identifiers, ident_get_mode(vm->sx, ref));
	vector_add(&vm->identifiers, ident_get_displ(vm->sx, ref));

	const char *buffer = repr_get_name(vm->sx, (size_t)ident_get_repr(vm->sx, ref));
	for (size_t i = 0; buffer[i] != '\0'; i += utf8_symbol_size(buffer[i]))
	{
		vector_add(&vm->representations, (item_t)utf8_convert(&buffer[i]));
	}
	vector_add(&vm->representations, '\0');

	vector_set(&vm->sx->identifiers, ref, ITEM_MAX);
	ident_set_repr(vm->sx, ref, new_ref);
	mem_add(vm, new_ref);
}

static void statement(virtual *const vm, node *const nd)
{
	switch (node_get_type(nd))
	{
		case ND_NULL:
			break;
		case ND_CREATEDIRECT:
			mem_add(vm, IC_CREATEDIRECT);
			vm->max_threads++;
			break;
		case ND_EXITDIRECT:
			mem_add(vm, IC_EXITDIRECT);
			break;
		case ND_BLOCK:
			block(vm, nd);
			break;
		case ND_IF:
		{
			const item_t ref_else = node_get_arg(nd, 0);

			expression(vm, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(vm, IC_BE0);
			size_t addr = mem_size(vm);
			mem_increase(vm, 1);
			statement(vm, nd);

			if (ref_else)
			{
				node_set_next(nd);
				mem_set(vm, addr, (item_t)mem_size(vm) + 2);
				mem_add(vm, IC_B);
				addr = mem_size(vm);
				mem_increase(vm, 1);
				statement(vm, nd);
			}
			mem_set(vm, addr, (item_t)mem_size(vm));
		}
		break;
		case ND_WHILE:
		{
			const size_t old_break = vm->addr_break;
			const size_t old_cond = vm->addr_cond;
			const size_t addr = mem_size(vm);

			vm->addr_cond = addr;
			expression(vm, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(vm, IC_BE0);
			vm->addr_break = mem_size(vm);
			mem_add(vm, 0);
			statement(vm, nd);

			addr_begin_condition(vm, addr);
			mem_add(vm, IC_B);
			mem_add(vm, (item_t)addr);
			addr_end_break(vm);

			vm->addr_break = old_break;
			vm->addr_cond = old_cond;
		}
		break;
		case ND_DO:
		{
			const size_t old_break = vm->addr_break;
			const size_t old_cond = vm->addr_cond;
			const item_t addr = (item_t)mem_size(vm);

			vm->addr_cond = 0;
			vm->addr_break = 0;

			node_set_next(nd);
			statement(vm, nd);
			addr_end_condition(vm);

			expression(vm, nd, 0);
			mem_add(vm, IC_BNE0);
			mem_add(vm, addr);
			addr_end_break(vm);

			vm->addr_break = old_break;
			vm->addr_cond = old_cond;
		}
		break;
		case ND_FOR:
		{
			const item_t ref_from = node_get_arg(nd, 0);
			const item_t ref_cond = node_get_arg(nd, 1);
			const item_t ref_incr = node_get_arg(nd, 2);

			node incr;
			node_copy(&incr, nd);
			size_t child_stmt = 0;

			if (ref_from)
			{
				node_set_next(&incr);
				if (declaration(vm, &incr))
				{
					expression(vm, &incr, -1);
				}
				child_stmt++;
			}

			const size_t old_break = vm->addr_break;
			const size_t old_cond = vm->addr_cond;
			vm->addr_cond = 0;
			vm->addr_break = 0;

			size_t initad = mem_size(vm);
			if (ref_cond)
			{
				expression(vm, &incr, 0); // condition
				mem_add(vm, IC_BE0);
				vm->addr_break = mem_size(vm);
				mem_add(vm, 0);
				child_stmt++;
			}

			if (ref_incr)
			{
				child_stmt++;
			}

			node stmt = node_get_child(nd, child_stmt);
			statement(vm, &stmt);
			addr_end_condition(vm);

			if (ref_incr)
			{
				expression(vm, &incr, 0); // increment
			}
			node_copy(nd, &stmt);

			mem_add(vm, IC_B);
			mem_add(vm, (item_t)initad);
			addr_end_break(vm);

			vm->addr_break = old_break;
			vm->addr_cond = old_cond;
		}
		break;
		case ND_GOTO:
		{
			mem_add(vm, IC_B);

			const item_t id_sign = node_get_arg(nd, 0);
			const size_t id = abs((int)id_sign);
			const item_t addr = ident_get_displ(vm->sx, id);

			if (addr > 0) // метка уже описана
			{
				mem_add(vm, addr);
			}
			else // метка еще не описана
			{
				ident_set_displ(vm->sx, id, -(item_t)mem_size(vm));

				// первый раз встретился переход на еще не описанную метку или нет
				mem_add(vm, id_sign < 0 ? 0 : addr);
			}
		}
		break;
		case ND_LABEL:
		{
			const item_t id = node_get_arg(nd, 0);
			item_t addr = ident_get_displ(vm->sx, (size_t)id);

			if (addr < 0) // были переходы на метку
			{
				while (addr) // проставить ссылку на метку во всех ранних переходах
				{
					item_t ref = mem_get(vm, (size_t)(-addr));
					mem_set(vm, (size_t)(-addr), (item_t)mem_size(vm));
					addr = ref;
				}
			}
			ident_set_displ(vm->sx, (size_t)id, (item_t)mem_size(vm));
		}
		break;
		case ND_SWITCH:
		{
			const size_t old_break = vm->addr_break;
			const size_t old_case = vm->addr_case;
			vm->addr_break = 0;
			vm->addr_case = 0;

			expression(vm, nd, 0);
			node_set_next(nd); // TExprend

			statement(vm, nd);
			if (vm->addr_case > 0)
			{
				mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
			}
			addr_end_break(vm);

			vm->addr_case = old_case;
			vm->addr_break = old_break;
		}
		break;
		case ND_CASE:
		{
			if (vm->addr_case)
			{
				mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
			}
			mem_add(vm, IC__DOUBLE);
			expression(vm, nd, 0);
			node_set_next(nd); // TExprend

			mem_add(vm, IC_EQEQ);
			mem_add(vm, IC_BE0);
			vm->addr_case = mem_size(vm);
			mem_increase(vm, 1);
			statement(vm, nd);
		}
		break;
		case ND_DEFAULT:
		{
			if (vm->addr_case)
			{
				mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
			}
			vm->addr_case = 0;

			node_set_next(nd);
			statement(vm, nd);
		}
		break;
		case ND_BREAK:
		{
			mem_add(vm, IC_B);
			mem_add(vm, (item_t)vm->addr_break);
			vm->addr_break = mem_size(vm) - 1;
		}
		break;
		case ND_CONTINUE:
		{
			mem_add(vm, IC_B);
			mem_add(vm, (item_t)vm->addr_cond);
			vm->addr_cond = mem_size(vm) - 1;
		}
		break;
		case ND_RETURN_VOID:
			mem_add(vm, IC_RETURNVOID);
			break;
		case ND_RETURN_VAL:
		{
			const item_t value = node_get_arg(nd, 0);
			expression(vm, nd, 0);

			mem_add(vm, IC_RETURNVAL);
			mem_add(vm, value);
		}
		break;
		case ND_PRINTID:
		{
			mem_add(vm, IC_PRINTID);
			compress_ident(vm, (size_t)node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		case ND_PRINTF:
		{
			mem_add(vm, IC_PRINTF);
			mem_add(vm, node_get_arg(nd, 0)); // общий размер того, что надо вывести
		}
		break;
		case ND_GETID:
		{
			mem_add(vm, IC_GETID);
			compress_ident(vm, (size_t)node_get_arg(nd, 0)); // ссылка в identtab
		}
		break;
		default:
			if (declaration(vm, nd))
			{
				expression(vm, nd, -1);
			}
			break;
	}
}

static void block(virtual *const vm, node *const nd)
{
	node_set_next(nd); // TBegin
	while (node_get_type(nd) != ND_BLOCK_END)
	{
		statement(vm, nd);
		node_set_next(nd);
	}
}

/** Генерация кодов */
static int codegen(virtual *const vm)
{
	node root = node_get_root(&vm->sx->tree);
	while (node_set_next(&root) == 0)
	{
		switch (node_get_type(&root))
		{
			case ND_FUNC_DEF:
			{
				const item_t ref_ident = node_get_arg(&root, 0);
				const item_t max_displ = node_get_arg(&root, 1);
				const size_t func = (size_t)ident_get_displ(vm->sx, (size_t)ref_ident);

				func_set(vm->sx, func, (item_t)mem_size(vm));
				mem_add(vm, IC_FUNCBEG);
				mem_add(vm, max_displ);

				const size_t old_pc = mem_size(vm);
				mem_increase(vm, 1);

				node_set_next(&root);
				block(vm, &root);

				mem_set(vm, old_pc, (item_t)mem_size(vm));
			}
			break;

			case ND_NULL:
			case ND_BLOCK_END:
				break;

			default:
				if (declaration(vm, &root))
				{
					system_error(node_unexpected, node_get_type(&root));
					return -1;
				}
				break;
		}
	}

	mem_add(vm, IC_CALL1);
	mem_add(vm, IC_CALL2);
	mem_add(vm, ident_get_displ(vm->sx, vm->sx->ref_main));
	mem_add(vm, IC_STOP);
	return 0;
}


static int output_table(universal_io *const io, const item_status target, const vector *const table)
{
	const size_t size = vector_size(table);
	for (size_t i = 0; i < size; i++)
	{
		const item_t item = vector_get(table, i);
		if (!item_check_var(target, item))
		{
			system_error(tables_cannot_be_compressed);
			return -1;
		}

		uni_printf(io, "%" PRIitem " ", item);
	}

	uni_printf(io, "\n");
	return 0;
}

/** Вывод таблиц в файл */
static int output_export(universal_io *const io, const virtual *const vm)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %" PRIitem " %zi\n"
		, vector_size(&vm->memory)
		, vector_size(&vm->sx->functions)
		, vector_size(&vm->identifiers)
		, vector_size(&vm->representations)
		, vector_size(&vm->sx->modes)
		, vm->sx->max_displg, vm->max_threads);

	return output_table(io, vm->target, &vm->memory)
		|| output_table(io, vm->target, &vm->sx->functions)
		|| output_table(io, vm->target, &vm->identifiers)
		|| output_table(io, vm->target, &vm->representations)
		|| output_table(io, vm->target, &vm->sx->modes);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_vm(const workspace *const ws, universal_io *const io, syntax *const sx)
{
	if (!ws_is_correct(ws) || !out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	virtual vm;
	vm.sx = sx;

	vm.memory = vector_create(MAX_MEM_SIZE);
	vm.processes = vector_create(sx->procd);
	vm.stk = stack_create(MAX_STACK_SIZE);

	const size_t records = vector_size(&sx->identifiers) / 4;
	vm.identifiers = vector_create(records * 3);
	vm.representations = vector_create(records * 8);

	vector_increase(&vm.memory, 4);
	vector_increase(&vm.processes, sx->procd);
	vm.max_threads = 0;

	vm.target = item_get_status(ws);


	int ret = codegen(&vm);
	if (!ret)
	{
		ret = output_export(io, &vm);
	}

#ifdef GENERATE_CODES
	tables_and_codes(DEFAULT_CODES, &sx->functions, &vm.processes, &vm.memory);
#endif


	vector_clear(&vm.memory);
	vector_clear(&vm.processes);
	stack_clear(&vm.stk);

	vector_clear(&vm.identifiers);
	vector_clear(&vm.representations);
	return ret;
}
