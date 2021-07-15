/*
 *	Copyright 2015 Andrey Terekhov, Victor Y. Fadeev, Ilya Andreev
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
#include "operations.h"
#include "stack.h"
#include "tree.h"
#include "uniprinter.h"


static const char *const DEFAULT_CODES = "codes.txt";

static const size_t MAX_MEM_SIZE = 100000;
static const size_t MAX_STACK_SIZE = 256;


/** Virtual machine codes emitter */
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
	int was_error;					/**< Error flag */
} virtual;

static void emit_expression(virtual *const vm, const node *const nd);
static void emit_declaration(virtual *const vm, const node *const nd);
static void emit_statement(virtual *const vm, const node *const nd);


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

static inline size_t mem_reserve(virtual *const vm)
{
	return vector_add(&vm->memory, 0);
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


/**
 *	Create code generator
 *
 *	@param	ws		Compiler workspace
 *	@param	sx		Syntax structure
 *
 *	@return	Code generator
 */
static virtual vm_create(const workspace *const ws, syntax *const sx)
{
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
	vm.was_error = 0;

	return vm;
}

/**
 *	Print table
 *
 *	@param	io		Universal io structure
 *	@param	target	Target item type
 *	@param	table	Table for printing
 *
 *	@return	@c 0 on success, @c -1 on error
 */
static int print_table(universal_io *const io, const item_status target, const vector *const table)
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

/**
 *	Export codes of virtual machine
 *
 *	@param	io		Universal io structure
 *	@param	vm		Code generator
 *
 *	@return	@c 0 on success, @c -1 on error
 */
static int vm_export(universal_io *const io, const virtual *const vm)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %" PRIitem " %zi\n"
		, vector_size(&vm->memory)
		, vector_size(&vm->sx->functions)
		, vector_size(&vm->identifiers)
		, vector_size(&vm->representations)
		, vector_size(&vm->sx->types)
		, vm->sx->max_displg, vm->max_threads);

	return print_table(io, vm->target, &vm->memory)
		|| print_table(io, vm->target, &vm->sx->functions)
		|| print_table(io, vm->target, &vm->identifiers)
		|| print_table(io, vm->target, &vm->representations)
		|| print_table(io, vm->target, &vm->sx->types);
}

/**
 *	Clear code generator structure
 *
 *	@param	vm		Code generator
 */
static void vm_clear(virtual *const vm)
{
	vector_clear(&vm->memory);
	vector_clear(&vm->processes);
	stack_clear(&vm->stk);

	vector_clear(&vm->identifiers);
	vector_clear(&vm->representations);
}


static void final_operation(virtual *const vm, node *const nd)
{
	operation_t op = (operation_t)node_get_type(nd);
	while (op > BEGIN_OP_FINAL && op < END_OP_FINAL)
	{
		if (op != OP_NOP)
		{
			if (op == OP_AD_LOG_OR)
			{
				mem_add(vm, IC_DUPLICATE);
				mem_add(vm, IC_BNE0);
				stack_push(&vm->stk, (item_t)mem_size(vm));
				mem_increase(vm, 1);
			}
			else if (op == OP_AD_LOG_AND)
			{
				mem_add(vm, IC_DUPLICATE);
				mem_add(vm, IC_BE0);
				stack_push(&vm->stk, (item_t)mem_size(vm));
				mem_increase(vm, 1);
			}
			else
			{
				mem_add(vm, (instruction_t)op);
				if (op == OP_LOG_OR || op == OP_LOG_AND)
				{
					mem_set(vm, (size_t)stack_pop(&vm->stk), (item_t)mem_size(vm));
				}
				else if (op == OP_COPY00 || op == OP_COPYST)
				{
					mem_add(vm, node_get_arg(nd, 0)); // d1
					mem_add(vm, node_get_arg(nd, 1)); // d2
					mem_add(vm, node_get_arg(nd, 2)); // длина
				}
				else if (op == OP_COPY01 || op == OP_COPY10 || op == OP_COPY0ST || op == OP_COPY0ST_ASSIGN)
				{
					mem_add(vm, node_get_arg(nd, 0)); // d1
					mem_add(vm, node_get_arg(nd, 1)); // длина
				}
				else if (op == OP_COPY11 || op == OP_COPY1ST || op == OP_COPY1ST_ASSIGN)
				{
					mem_add(vm, node_get_arg(nd, 0)); // длина
				}
				else if (operation_is_assignment(op))
				{
					mem_add(vm, node_get_arg(nd, 0));
				}
			}
		}

		node_set_next(nd);
		op = (operation_t)node_get_type(nd);
	}
}

/**
 *	Emit call expression
 *
 *	@param	vm				Code generator
 *	@param	nd				Node in AST
 */
static void emit_call_expression(virtual *const vm, node *const nd)
{
	const size_t args = (size_t)node_get_arg(nd, 0);
	node nd_call2 = node_get_child(nd, args);
	const size_t func_id = (size_t)node_get_arg(&nd_call2, 0);

	if (func_id >= BEGIN_USER_FUNC)
	{
		// Это вызов пользовательской функции
		mem_add(vm, IC_CALL1);
		for (size_t i = 0; i < args; i++)
		{
			node nd_argument = node_get_child(nd, i);
			emit_expression(vm, &nd_argument);
		}

		mem_add(vm, IC_CALL2);
		mem_add(vm, ident_get_displ(vm->sx, func_id));
	}
	else
	{
		// Это вызов библиотечной функции
		for (size_t i = 0; i < args; i++)
		{
			node nd_argument = node_get_child(nd, i);
			emit_expression(vm, &nd_argument);
		}

		mem_add(vm, builtin_to_instruction((builtin_t)func_id));
	}

	node_copy(nd, &nd_call2);
}

/**
 *	Emit expression
 *
 *	@param	vm				Code generator
 *	@param	nd				Node in AST
 *	@param	is_in_condition	Set, if expression in condition
 */
static void expression(virtual *const vm, node *const nd, const bool is_in_condition)
{
	while (node_get_type(nd) != OP_EXPR_END)
	{
		const operation_t operation = (operation_t)node_get_type(nd);
		bool was_operation = true;

		switch (operation)
		{
			case OP_IDENT:
				break;
			case OP_IDENT_TO_ADDR:
			{
				mem_add(vm, IC_LA);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL:
			{
				mem_add(vm, IC_LOAD);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL_D:
			{
				mem_add(vm, IC_LOADD);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case OP_ADDR_TO_VAL:
				mem_add(vm, IC_LAT);
				break;
			case OP_ADDR_TO_VAL_D:
				mem_add(vm, IC_LATD);
				break;
			case OP_CONST:
			{
				mem_add(vm, IC_LI);
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case OP_CONST_D:
			{
				mem_add(vm, IC_LID);
				mem_add(vm, node_get_arg(nd, 0));
				mem_add(vm, node_get_arg(nd, 1));
			}
			break;
			case OP_STRING:
			case OP_STRING_D:
			{
				mem_add(vm, IC_LI);
				const size_t reserved = mem_size(vm) + 4;
				mem_add(vm, (item_t)reserved);
				mem_add(vm, IC_B);
				mem_increase(vm, 2);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					if (operation == OP_STRING)
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
			case OP_ARRAY_INIT:
			{
				const item_t N = node_get_arg(nd, 0);

				mem_add(vm, IC_BEG_INIT);
				mem_add(vm, N);

				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(vm, nd, false);
				}
			}
			break;
			case OP_STRUCT_INIT:
			{
				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(vm, nd, false);
				}
			}
			break;
			case OP_SLICE_IDENT:
			{
				mem_add(vm, IC_LOAD); // параметры - смещение идента и тип элемента
				mem_add(vm, node_get_arg(nd, 0)); // продолжение в след case
			}
			case OP_SLICE: // параметр - тип элемента
			{
				item_t type = node_get_arg(nd, operation == OP_SLICE ? 0 : 1);

				node_set_next(nd);
				expression(vm, nd, false);
				mem_add(vm, IC_SLICE);
				mem_add(vm, (item_t)size_of(vm->sx, type));
				if (type_is_array(vm->sx, type))
				{
					mem_add(vm, IC_LAT);
				}
			}
			break;
			case OP_SELECT:
			{
				mem_add(vm, IC_SELECT); // SELECT field_displ
				mem_add(vm, node_get_arg(nd, 0));
			}
			break;
			case OP_PRINT:
			{
				mem_add(vm, IC_PRINT);
				mem_add(vm, node_get_arg(nd, 0)); // type
			}
			break;
			case OP_CALL1:
				emit_call_expression(vm, nd);
				break;
			default:
				was_operation = false;
				break;
		}

		if (was_operation)
		{
			node_set_next(nd);
		}

		final_operation(vm, nd);

		if (node_get_type(nd) == OP_CONDITIONAL)
		{
			if (is_in_condition)
			{
				return;
			}

			size_t addr = 0;
			do
			{
				mem_add(vm, IC_BE0);
				const size_t addr_else = mem_size(vm);
				mem_increase(vm, 1);

				node_set_next(nd);
				expression(vm, nd, false); // then
				mem_add(vm, IC_B);
				mem_add(vm, (item_t)addr);
				addr = mem_size(vm) - 1;
				mem_set(vm, addr_else, (item_t)mem_size(vm));

				node_set_next(nd);
				expression(vm, nd, true); // else или cond
			} while (node_get_type(nd) == OP_CONDITIONAL);

			while (addr)
			{
				const size_t ref = (size_t)mem_get(vm, addr);
				mem_set(vm, addr, (item_t)mem_size(vm));
				addr = ref;
			}

			final_operation(vm, nd);
			node_set_next(nd); // TExprend
		}
	}
}

/**
 *	Emit expression
 *
 *	@note	Это переходник под старый expression(), который использует node_set_next()
 *	TODO:	переписать генерацию выражений
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_expression(virtual *const vm, const node *const nd)
{
	node nd_internal;
	node_copy(&nd_internal, nd);
	expression(vm, &nd_internal, false);
}


/**
 *	Emit function definition
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_function_definition(virtual *const vm, const node *const nd)
{
	const size_t ref_func = (size_t)ident_get_displ(vm->sx, (size_t)node_get_arg(nd, 0));
	func_set(vm->sx, ref_func, (item_t)mem_size(vm));

	mem_add(vm, IC_FUNC_BEG);
	mem_add(vm, node_get_arg(nd, 1));

	const size_t old_pc = mem_reserve(vm);

	const node nd_func_body = node_get_child(nd, 0);
	emit_statement(vm, &nd_func_body);

	mem_set(vm, old_pc, (item_t)mem_size(vm));
}

/**
 *	Emit variable declaration
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_variable_declaration(virtual *const vm, const node *const nd)
{
	const item_t old_displ = node_get_arg(nd, 0);
	const item_t type = node_get_arg(nd, 1);
	const item_t process = node_get_arg(nd, 4);

	if (process)
	{
		mem_add(vm, IC_STRUCT_WITH_ARR);
		mem_add(vm, old_displ);
		mem_add(vm, proc_get(vm, (size_t)process));
	}

	if (node_get_amount(nd) > 0) // int a = или struct{} a =
	{
		const node nd_initializer = node_get_child(nd, 0);
		emit_expression(vm, &nd_initializer);

		if (type_is_struct(vm->sx, type))
		{
			mem_add(vm, IC_COPY0ST_ASSIGN);
			mem_add(vm, old_displ);
			mem_add(vm, node_get_arg(nd, 3)); // Общее количество слов
		}
		else
		{
			mem_add(vm, type_is_float(type) ? IC_ASSIGN_R_V : IC_ASSIGN_V);
			mem_add(vm, old_displ);
		}
	}
}

/**
 *	Emit array declaration
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_array_declaration(virtual *const vm, const node *const nd)
{
	const size_t bounds = (size_t)node_get_arg(nd, 0);
	for (size_t i = 0; i < bounds; i++)
	{
		const node nd_expression = node_get_child(nd, i);
		emit_expression(vm, &nd_expression);
	}

	mem_add(vm, IC_DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке

	const node nd_decl_id = node_get_child(nd, bounds);
	const item_t dimensions = node_get_arg(&nd_decl_id, 2);
	const bool has_initializer = bounds + 1 < node_get_amount(nd);
	mem_add(vm, has_initializer ? dimensions - 1 : dimensions);

	const item_t length = (item_t)size_of(vm->sx, node_get_arg(&nd_decl_id, 1));
	mem_add(vm, length);

	const item_t old_displ = node_get_arg(&nd_decl_id, 0);
	mem_add(vm, old_displ);
	mem_add(vm, proc_get(vm, (size_t)node_get_arg(&nd_decl_id, 4)));	// process

	const item_t usual = node_get_arg(&nd_decl_id, 5);
	mem_add(vm, usual);													// has empty bounds
	mem_add(vm, node_get_arg(&nd_decl_id, 3));							// has initializer
	mem_add(vm, node_get_arg(&nd_decl_id, 6));							// is in structure

	if (has_initializer)
	{
		const node nd_initializer = node_get_child(nd, bounds + 1);
		emit_expression(vm, &nd_initializer);

		mem_add(vm, IC_ARR_INIT);
		mem_add(vm, dimensions);
		mem_add(vm, length);
		mem_add(vm, old_displ);
		mem_add(vm, usual);
	}
}

/**
 *	Emit struct declaration
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_struct_declaration(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_B);
	mem_add(vm, 0);

	const size_t num_proc = (size_t)node_get_arg(nd, 0);
	proc_set(vm, num_proc, (item_t)mem_size(vm));

	const size_t amount = node_get_amount(nd);
	for (size_t i = 0; i < amount - 1; i++)
	{
		const node nd_decl = node_get_child(nd, i);
		emit_declaration(vm, &nd_decl);
	}

	mem_add(vm, IC_STOP);
	mem_set(vm, (size_t)proc_get(vm, num_proc) - 1, (item_t)mem_size(vm));
}

/**
 *	Emit declaration
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_declaration(virtual *const vm, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_FUNC_DEF:
			emit_function_definition(vm, nd);
			break;

		case OP_DECL_ID:
			emit_variable_declaration(vm, nd);
			break;

		case OP_DECL_ARR:
			emit_array_declaration(vm, nd);
			break;

		case OP_DECL_STRUCT:
			emit_struct_declaration(vm, nd);
			break;

		case OP_BLOCK_END:
			// Это старый признак конца программы
			// TODO: убрать, когда уберем OP_BLOCK_END
			break;

		default:
			system_error(node_unexpected, node_get_type(nd));
			vm->was_error = 1;
			break;
	}
}


/**
 *	Emit labeled statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_labeled_statement(virtual *const vm, const node *const nd)
{
	const item_t label_id = node_get_arg(nd, 0);
	item_t addr = ident_get_displ(vm->sx, (size_t)label_id);

	if (addr < 0)
	{
		// Были переходы на метку
		while (addr != 0)
		{
			// Проставить ссылку на метку во всех ранних переходах
			const item_t ref = mem_get(vm, (size_t)(-addr));
			mem_set(vm, (size_t)(-addr), (item_t)mem_size(vm));
			addr = ref;
		}
	}

	ident_set_displ(vm->sx, (size_t)label_id, (item_t)mem_size(vm));

	const node nd_statement = node_get_child(nd, 0);
	emit_statement(vm, &nd_statement);
}

/**
 *	Emit case statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_case_statement(virtual *const vm, const node *const nd)
{
	if (vm->addr_case != 0)
	{
		mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
	}

	mem_add(vm, IC_DUPLICATE);

	const node nd_expression = node_get_child(nd, 0);
	emit_expression(vm, &nd_expression);

	mem_add(vm, IC_EQ);
	mem_add(vm, IC_BE0);
	vm->addr_case = mem_reserve(vm);

	const node nd_statement = node_get_child(nd, 1);
	emit_statement(vm, &nd_statement);
}

/**
 *	Emit default statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_default_statement(virtual *const vm, const node *const nd)
{
	if (vm->addr_case != 0)
	{
		mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
	}
	vm->addr_case = 0;

	const node nd_statement = node_get_child(nd, 0);
	emit_statement(vm, &nd_statement);
}

/**
 *	Emit compound statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_block(virtual *const vm, const node *const nd)
{
	const size_t amount = node_get_amount(nd);
	for (size_t i = 0; i < amount; i++)
	{
		const node nd_block_item = node_get_child(nd, i);
		emit_statement(vm, &nd_block_item);
	}
}

/**
 *	Emit if statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_if_statement(virtual *const vm, const node *const nd)
{
	const node nd_condition = node_get_child(nd, 0);
	emit_expression(vm, &nd_condition);
	mem_add(vm, IC_BE0);
	size_t addr = mem_reserve(vm);

	const node nd_then = node_get_child(nd, 1);
	emit_statement(vm, &nd_then);

	if (node_get_amount(nd) > 2)
	{
		mem_set(vm, addr, (item_t)mem_size(vm) + 2);
		mem_add(vm, IC_B);
		addr = mem_reserve(vm);

		const node nd_else = node_get_child(nd, 2);
		emit_statement(vm, &nd_else);
	}

	mem_set(vm, addr, (item_t)mem_size(vm));
}

/**
 *	Emit switch statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_switch_statement(virtual *const vm, const node *const nd)
{
	const size_t old_addr_break = vm->addr_break;
	const size_t old_addr_case = vm->addr_case;
	vm->addr_break = 0;
	vm->addr_case = 0;

	const node nd_condition = node_get_child(nd, 0);
	emit_expression(vm, &nd_condition);

	const node nd_statement = node_get_child(nd, 1);
	emit_statement(vm, &nd_statement);

	if (vm->addr_case > 0)
	{
		mem_set(vm, vm->addr_case, (item_t)mem_size(vm));
	}
	addr_end_break(vm);

	vm->addr_case = old_addr_case;
	vm->addr_break = old_addr_break;
}

/**
 *	Emit while statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_while_statement(virtual *const vm, const node *const nd)
{
	const size_t old_addr_break = vm->addr_break;
	const size_t old_addr_cond = vm->addr_cond;
	const size_t addr = mem_size(vm);
	vm->addr_cond = addr;

	const node nd_condition = node_get_child(nd, 0);
	emit_expression(vm, &nd_condition);

	mem_add(vm, IC_BE0);
	vm->addr_break = mem_size(vm);
	mem_add(vm, 0);

	const node nd_statement = node_get_child(nd, 1);
	emit_statement(vm, &nd_statement);

	addr_begin_condition(vm, addr);
	mem_add(vm, IC_B);
	mem_add(vm, (item_t)addr);
	addr_end_break(vm);

	vm->addr_break = old_addr_break;
	vm->addr_cond = old_addr_cond;
}

/**
 *	Emit do statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_do_statement(virtual *const vm, const node *const nd)
{
	const size_t old_addr_break = vm->addr_break;
	const size_t old_addr_cond = vm->addr_cond;
	const item_t addr = (item_t)mem_size(vm);

	vm->addr_cond = 0;
	vm->addr_break = 0;

	const node nd_statement = node_get_child(nd, 0);
	emit_statement(vm, &nd_statement);
	addr_end_condition(vm);

	const node nd_expression = node_get_child(nd, 1);
	emit_expression(vm, &nd_expression);
	mem_add(vm, IC_BNE0);
	mem_add(vm, addr);
	addr_end_break(vm);

	vm->addr_break = old_addr_break;
	vm->addr_cond = old_addr_cond;
}

/**
 *	Emit for statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_for_statement(virtual *const vm, const node *const nd)
{
	size_t child_index = 0;

	const bool has_init = node_get_arg(nd, 0) != 0;
	if (has_init)
	{
		// Предполагая, что дерево правильно построено
		const node nd_init = node_get_child(nd, child_index++);
		emit_statement(vm, &nd_init);
	}

	const size_t old_addr_break = vm->addr_break;
	const size_t old_addr_cond = vm->addr_cond;
	vm->addr_cond = 0;
	vm->addr_break = 0;

	const size_t addr_init = mem_size(vm);
	const bool has_condition = node_get_arg(nd, 1) != 0;
	if (has_condition)
	{
		const node nd_condition = node_get_child(nd, child_index++);
		emit_expression(vm, &nd_condition);
		mem_add(vm, IC_BE0);
		vm->addr_break = mem_size(vm);
		mem_add(vm, 0);
	}

	const bool has_increment = node_get_arg(nd, 2) != 0;

	const node nd_statement = node_get_child(nd, child_index + (has_increment ? 1 : 0));
	emit_statement(vm, &nd_statement);
	addr_end_condition(vm);

	if (has_increment)
	{
		const node nd_increment = node_get_child(nd, child_index);
		emit_expression(vm, &nd_increment);
	}

	mem_add(vm, IC_B);
	mem_add(vm, (item_t)addr_init);
	addr_end_break(vm);

	vm->addr_break = old_addr_break;
	vm->addr_cond = old_addr_cond;
}

/**
 *	Emit goto statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_goto_statement(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_B);

	const item_t label_id = node_get_arg(nd, 0);
	const size_t id = (size_t)llabs(label_id);
	const item_t addr = ident_get_displ(vm->sx, id);

	if (addr > 0)
	{
		// Метка уже описана
		mem_add(vm, addr);
	}
	else
	{
		// Метка еще не описана
		ident_set_displ(vm->sx, id, -(item_t)mem_size(vm));

		// Если эта метка уже встречалась, ставим адрес предыдущего перехода сюда
		mem_add(vm, label_id < 0 ? 0 : addr);
	}
}

/**
 *	Emit continue statement
 *
 *	@param	vm			Code generator
 */
static void emit_continue_statement(virtual *const vm)
{
	mem_add(vm, IC_B);
	mem_add(vm, (item_t)vm->addr_cond);
	vm->addr_cond = mem_size(vm) - 1;
}

/**
 *	Emit break statement
 *
 *	@param	vm			Code generator
 */
static void emit_break_statement(virtual *const vm)
{
	mem_add(vm, IC_B);
	mem_add(vm, (item_t)vm->addr_break);
	vm->addr_break = mem_size(vm) - 1;
}

/**
 *	Emit return statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_return_statement(virtual *const vm, const node *const nd)
{
	if (node_get_amount(nd) > 0)
	{
		const node nd_expr = node_get_child(nd, 0);
		emit_expression(vm, &nd_expr);

		mem_add(vm, IC_RETURN_VAL);
		mem_add(vm, node_get_arg(nd, 0));
	}
	else
	{
		mem_add(vm, IC_RETURN_VOID);
	}
}

/**
 *	Emit t_create_direct statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_thread(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_CREATE_DIRECT);

	const size_t amount = node_get_amount(nd);
	for (size_t i = 0; i < amount; i++)
	{
		const node nd_thread_item = node_get_child(nd, i);
		emit_statement(vm, &nd_thread_item);
	}

	mem_add(vm, IC_EXIT_DIRECT);
	vm->max_threads++;
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
	vector_add(&vm->identifiers, ident_get_type(vm->sx, ref));
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

/**
 *	Emit printid statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static inline void emit_printid_statement(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_PRINTID);
	compress_ident(vm, (size_t)node_get_arg(nd, 0)); // Ссылка в identtab
}

/**
 *	Emit getid statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static inline void emit_getid_statement(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_GETID);
	compress_ident(vm, (size_t)node_get_arg(nd, 0)); //Сссылка в identtab
}

/**
 *	Emit printf statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static inline void emit_printf_statement(virtual *const vm, const node *const nd)
{
	mem_add(vm, IC_PRINTF);
	mem_add(vm, node_get_arg(nd, 0)); // Общий размер того, что надо вывести
}

/**
 *	Emit statement
 *
 *	@param	vm			Code generator
 *	@param	nd			Node in AST
 */
static void emit_statement(virtual *const vm, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_LABEL:
			emit_labeled_statement(vm, nd);
			break;

		case OP_CASE:
			emit_case_statement(vm, nd);
			break;

		case OP_DEFAULT:
			emit_default_statement(vm, nd);
			break;

		case OP_BLOCK:
			emit_block(vm, nd);
			break;

		case OP_NOP:
			break;

		case OP_IF:
			emit_if_statement(vm, nd);
			break;

		case OP_SWITCH:
			emit_switch_statement(vm, nd);
			break;

		case OP_WHILE:
			emit_while_statement(vm, nd);
			break;

		case OP_DO:
			emit_do_statement(vm, nd);
			break;

		case OP_FOR:
			emit_for_statement(vm, nd);
			break;

		case OP_GOTO:
			emit_goto_statement(vm, nd);
			break;

		case OP_CONTINUE:
			emit_continue_statement(vm);
			break;

		case OP_BREAK:
			emit_break_statement(vm);
			break;

		case OP_RETURN_VAL:
		case OP_RETURN_VOID:
			emit_return_statement(vm, nd);
			break;

		case OP_CREATE_DIRECT:
			emit_thread(vm, nd);
			break;

		case OP_PRINTID:
			emit_printid_statement(vm, nd);
			break;

		case OP_GETID:
			emit_getid_statement(vm, nd);
			break;

		case OP_PRINTF:
			emit_printf_statement(vm, nd);
			break;

		case OP_BLOCK_END:
		case OP_EXIT_DIRECT:
			// Пока что это чтобы не менять дерево
			// Но на самом деле такие узлы не нужны, так как реализация дерева знает количество потомков
			break;

		case OP_DECL_ID:
		case OP_DECL_ARR:
		case OP_DECL_STRUCT:
			emit_declaration(vm, nd);
			break;

		default:
			emit_expression(vm, nd);
			break;
	}
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

	virtual vm = vm_create(ws, sx);

	const node root = node_get_root(&sx->tree);
	const size_t amount = node_get_amount(&root);
	for (size_t i = 0; i < amount && !vm.was_error; i++)
	{
		const node nd = node_get_child(&root, i);
		emit_declaration(&vm, &nd);
	}

	mem_add(&vm, IC_CALL1);
	mem_add(&vm, IC_CALL2);
	mem_add(&vm, ident_get_displ(vm.sx, vm.sx->ref_main));
	mem_add(&vm, IC_STOP);

#ifndef NDEBUG
	tables_and_codes(DEFAULT_CODES, &sx->functions, &vm.processes, &vm.memory);
#endif

	int ret = vm.was_error || vm_export(io, &vm);

	vm_clear(&vm);
	return ret;
}
