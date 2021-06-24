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
typedef struct encoder
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
} encoder;


static void emit_declaration(encoder *const enc, const node *const nd);
static void emit_statement(encoder *const enc, const node *const nd);
static void emit_block(encoder *const enc, const node *const nd);


static inline void mem_increase(encoder *const enc, const size_t size)
{
	vector_increase(&enc->memory, size);
}

static inline void mem_add(encoder *const enc, const item_t value)
{
	vector_add(&enc->memory, value);
}

static inline void mem_set(encoder *const enc, const size_t index, const item_t value)
{
	vector_set(&enc->memory, index, value);
}

static inline item_t mem_get(const encoder *const enc, const size_t index)
{
	return vector_get(&enc->memory, index);
}

static inline size_t mem_size(const encoder *const enc)
{
	return vector_size(&enc->memory);
}

static inline size_t mem_reserve(encoder *const enc)
{
	vector_increase(&enc->memory, 1);
	return mem_size(enc) - 1;
}


static inline void proc_set(encoder *const enc, const size_t index, const item_t value)
{
	vector_set(&enc->processes, index, value);
}

static inline item_t proc_get(const encoder *const enc, const size_t index)
{
	return vector_get(&enc->processes, index);
}


static void addr_begin_condition(encoder *const enc, const size_t addr)
{
	while (enc->addr_cond != addr)
	{
		const size_t ref = (size_t)mem_get(enc, enc->addr_cond);
		mem_set(enc, enc->addr_cond, (item_t)addr);
		enc->addr_cond = ref;
	}
}

static void addr_end_condition(encoder *const enc)
{
	while (enc->addr_cond)
	{
		const size_t ref = (size_t)mem_get(enc, enc->addr_cond);
		mem_set(enc, enc->addr_cond, (item_t)mem_size(enc));
		enc->addr_cond = ref;
	}
}

static void addr_end_break(encoder *const enc)
{
	while (enc->addr_break)
	{
		const size_t ref = (size_t)mem_get(enc, enc->addr_break);
		mem_set(enc, enc->addr_break, (item_t)mem_size(enc));
		enc->addr_break = ref;
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
static encoder enc_create(const workspace *const ws, syntax *const sx)
{
	encoder enc;
	enc.sx = sx;

	enc.memory = vector_create(MAX_MEM_SIZE);
	enc.processes = vector_create(sx->procd);
	enc.stk = stack_create(MAX_STACK_SIZE);

	const size_t records = vector_size(&sx->identifiers) / 4;
	enc.identifiers = vector_create(records * 3);
	enc.representations = vector_create(records * 8);

	vector_increase(&enc.memory, 4);
	vector_increase(&enc.processes, sx->procd);
	enc.max_threads = 0;

	enc.target = item_get_status(ws);

	enc.was_error = 0;

	return enc;
}

/** Print table */
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
 *	Export codes for virtual machine
 *
 *	@param	io		Universal io structure
 *	@param	enc		Code generator
 *
 *	@return	@c 0 on success, @c -1 on error
 */
static int enc_export(universal_io *const io, const encoder *const enc)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %" PRIitem " %zi\n"
		, vector_size(&enc->memory)
		, vector_size(&enc->sx->functions)
		, vector_size(&enc->identifiers)
		, vector_size(&enc->representations)
		, vector_size(&enc->sx->types)
		, enc->sx->max_displg, enc->max_threads);

	return print_table(io, enc->target, &enc->memory)
		|| print_table(io, enc->target, &enc->sx->functions)
		|| print_table(io, enc->target, &enc->identifiers)
		|| print_table(io, enc->target, &enc->representations)
		|| print_table(io, enc->target, &enc->sx->types);
}

/**
 *	Clear Code generator structure
 *
 *	@param	enc		Code generator
 */
static void enc_clear(encoder *const enc)
{
	vector_clear(&enc->memory);
	vector_clear(&enc->processes);
	stack_clear(&enc->stk);

	vector_clear(&enc->identifiers);
	vector_clear(&enc->representations);
}


static void final_operation(encoder *const enc, node *const nd)
{
	operation_t op = (operation_t)node_get_type(nd);
	while (op > BEGIN_OP_FINAL && op < END_OP_FINAL)
	{
		if (op != OP_NOP)
		{
			if (op == OP_AD_LOG_OR)
			{
				mem_add(enc, IC_DUPLICATE);
				mem_add(enc, IC_BNE0);
				stack_push(&enc->stk, (item_t)mem_size(enc));
				mem_increase(enc, 1);
			}
			else if (op == OP_AD_LOG_AND)
			{
				mem_add(enc, IC_DUPLICATE);
				mem_add(enc, IC_BE0);
				stack_push(&enc->stk, (item_t)mem_size(enc));
				mem_increase(enc, 1);
			}
			else
			{
				mem_add(enc, (instruction_t)op);
				if (op == OP_LOG_OR || op == OP_LOG_AND)
				{
					mem_set(enc, (size_t)stack_pop(&enc->stk), (item_t)mem_size(enc));
				}
				else if (op == OP_COPY00 || op == OP_COPYST)
				{
					mem_add(enc, node_get_arg(nd, 0)); // d1
					mem_add(enc, node_get_arg(nd, 1)); // d2
					mem_add(enc, node_get_arg(nd, 2)); // длина
				}
				else if (op == OP_COPY01 || op == OP_COPY10 || op == OP_COPY0ST || op == OP_COPY0ST_ASSIGN)
				{
					mem_add(enc, node_get_arg(nd, 0)); // d1
					mem_add(enc, node_get_arg(nd, 1)); // длина
				}
				else if (op == OP_COPY11 || op == OP_COPY1ST || op == OP_COPY1ST_ASSIGN)
				{
					mem_add(enc, node_get_arg(nd, 0)); // длина
				}
				else if (operation_is_assignment(op))
				{
					mem_add(enc, node_get_arg(nd, 0));
				}
			}
		}

		node_set_next(nd);
		op = (operation_t)node_get_type(nd);
	}
}

/**
 *	Expression generation
 *
 *	@param	enc				Code generator
 *	@param	is_in_condition	@c true for expression in condition
 */
static void expression(encoder *const enc, node *const nd, const bool is_in_condition)
{
	while (node_get_type(nd) != OP_EXPR_END)
	{
		const operation_t operation = (operation_t)node_get_type(nd);
		int was_operation = 1;

		switch (operation)
		{
			case OP_IDENT:
				break;
			case OP_IDENT_TO_ADDR:
			{
				mem_add(enc, IC_LA);
				mem_add(enc, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL:
			{
				mem_add(enc, IC_LOAD);
				mem_add(enc, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL_D:
			{
				mem_add(enc, IC_LOADD);
				mem_add(enc, node_get_arg(nd, 0));
			}
			break;
			case OP_ADDR_TO_VAL:
				mem_add(enc, IC_LAT);
				break;
			case OP_ADDR_TO_VAL_D:
				mem_add(enc, IC_LATD);
				break;
			case OP_CONST:
			{
				mem_add(enc, IC_LI);
				mem_add(enc, node_get_arg(nd, 0));
			}
			break;
			case OP_CONST_D:
			{
				mem_add(enc, IC_LID);
				mem_add(enc, node_get_arg(nd, 0));
				mem_add(enc, node_get_arg(nd, 1));
			}
			break;
			case OP_STRING:
			case OP_STRING_D:
			{
				mem_add(enc, IC_LI);
				const size_t reserved = mem_size(enc) + 4;
				mem_add(enc, (item_t)reserved);
				mem_add(enc, IC_B);
				mem_increase(enc, 2);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					if (operation == OP_STRING)
					{
						mem_add(enc, node_get_arg(nd, (size_t)i + 1));
					}
					else
					{
						mem_add(enc, node_get_arg(nd, 2 * (size_t)i + 1));
						mem_add(enc, node_get_arg(nd, 2 * (size_t)i + 2));
					}
				}

				mem_set(enc, reserved - 1, N);
				mem_set(enc, reserved - 2, (item_t)mem_size(enc));
			}
			break;
			case OP_ARRAY_INIT:
			{
				const item_t N = node_get_arg(nd, 0);

				mem_add(enc, IC_BEG_INIT);
				mem_add(enc, N);

				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(enc, nd, false);
				}
			}
			break;
			case OP_STRUCT_INIT:
			{
				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(enc, nd, false);
				}
			}
			break;
			case OP_SLICE_IDENT:
			{
				mem_add(enc, IC_LOAD); // параметры - смещение идента и тип элемента
				mem_add(enc, node_get_arg(nd, 0)); // продолжение в след case
			}
			case OP_SLICE: // параметр - тип элемента
			{
				item_t type = node_get_arg(nd, operation == OP_SLICE ? 0 : 1);

				node_set_next(nd);
				expression(enc, nd, false);
				mem_add(enc, IC_SLICE);
				mem_add(enc, (item_t)size_of(enc->sx, type));
				if (type_is_array(enc->sx, type))
				{
					mem_add(enc, IC_LAT);
				}
			}
			break;
			case OP_SELECT:
			{
				mem_add(enc, IC_SELECT); // SELECT field_displ
				mem_add(enc, node_get_arg(nd, 0));
			}
			break;
			case OP_PRINT:
			{
				mem_add(enc, IC_PRINT);
				mem_add(enc, node_get_arg(nd, 0)); // type
			}
			break;
			case OP_CALL1:
			{
				mem_add(enc, IC_CALL1);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(enc, nd, 0);
				}
			}
			break;
			case OP_CALL2:
			{
				mem_add(enc, IC_CALL2);
				mem_add(enc, ident_get_displ(enc->sx, (size_t)node_get_arg(nd, 0)));
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

		final_operation(enc, nd);

		if (node_get_type(nd) == OP_CONDITIONAL)
		{
			if (is_in_condition)
			{
				return;
			}

			size_t addr = 0;
			do
			{
				mem_add(enc, IC_BE0);
				const size_t addr_else = mem_size(enc);
				mem_increase(enc, 1);

				node_set_next(nd);
				expression(enc, nd, false); // then
				mem_add(enc, IC_B);
				mem_add(enc, (item_t)addr);
				addr = mem_size(enc) - 1;
				mem_set(enc, addr_else, (item_t)mem_size(enc));

				node_set_next(nd);
				expression(enc, nd, true); // else или cond
			} while (node_get_type(nd) == OP_CONDITIONAL);

			while (addr)
			{
				const size_t ref = (size_t)mem_get(enc, addr);
				mem_set(enc, addr, (item_t)mem_size(enc));
				addr = ref;
			}

			final_operation(enc, nd);
		}
	}
}

/**
 *	Emit expression
 *
 *	@note	Это переходник под старый expression(), который использует node_set_next()
 *	TODO:	переписать генерацию выражений
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_expression(encoder *const enc, const node *const nd)
{
	node internal_node;
	node_copy(&internal_node, nd);
	expression(enc, &internal_node, false);
}


/**
 *	Emit function definition
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_function_definition(encoder *const enc, const node *const nd)
{
	const size_t ref_func = (size_t)ident_get_displ(enc->sx, (size_t)node_get_arg(nd, 0));
	func_set(enc->sx, ref_func, (item_t)mem_size(enc));

	mem_add(enc, IC_FUNC_BEG);
	mem_add(enc, node_get_arg(nd, 1));

	const size_t old_pc = mem_reserve(enc);

	const node func_body = node_get_child(nd, 0);
	emit_block(enc, &func_body);

	mem_set(enc, old_pc, (item_t)mem_size(enc));
}

/**
 *	Emit variable declaration
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_variable_declaration(encoder *const enc, const node *const nd)
{
	const item_t old_displ = node_get_arg(nd, 0);
	const item_t type = node_get_arg(nd, 1);
	const item_t process = node_get_arg(nd, 4);

	if (process)
	{
		mem_add(enc, IC_STRUCT_WITH_ARR);
		mem_add(enc, old_displ);
		mem_add(enc, proc_get(enc, (size_t)process));
	}

	const node initializer = node_get_child(nd, 0);
	if (node_is_correct(&initializer)) // int a = или struct{} a =
	{
		emit_expression(enc, &initializer);

		if (type_is_struct(enc->sx, type))
		{
			mem_add(enc, IC_COPY0ST_ASSIGN);
			mem_add(enc, old_displ);
			mem_add(enc, node_get_arg(nd, 3)); // Общее количество слов
		}
		else
		{
			mem_add(enc, type_is_float(type) ? IC_ASSIGN_R_V : IC_ASSIGN_V);
			mem_add(enc, old_displ);
		}
	}
}

/**
 *	Emit array declaration
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_array_declaration(encoder *const enc, const node *const nd)
{
	const size_t bounds = (size_t)node_get_arg(nd, 0);
	for (size_t i = 0; i < bounds; i++)
	{
		const node expression = node_get_child(nd, i);
		emit_expression(enc, &expression);
	}

	const node decl_id = node_get_child(nd, bounds);

	bool has_initializer = false;
	const node initializer = node_get_child(nd, bounds + 1);
	if (node_is_correct(&initializer))
	{
		has_initializer = true;
	}

	mem_add(enc, IC_DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке

	const item_t dimensions = node_get_arg(&decl_id, 2);
	mem_add(enc, has_initializer ? dimensions - 1 : dimensions);

	const item_t length = (item_t)size_of(enc->sx, node_get_arg(&decl_id, 1));
	mem_add(enc, length);

	const item_t old_displ = node_get_arg(&decl_id, 0);
	mem_add(enc, old_displ);
	mem_add(enc, proc_get(enc, (size_t)node_get_arg(&decl_id, 4)));

	/*
	 *	@param	usual	Для массивов:
	 *						@c 0 с пустыми границами,
	 *						@c 1 без пустых границ
	 */
	const item_t usual = node_get_arg(&decl_id, 5);
	mem_add(enc, usual);
	mem_add(enc, node_get_arg(&decl_id, 3));
	mem_add(enc, node_get_arg(&decl_id, 6));

	if (has_initializer)
	{
		emit_expression(enc, &initializer);

		mem_add(enc, IC_ARR_INIT);
		mem_add(enc, dimensions);
		mem_add(enc, length);
		mem_add(enc, old_displ);
		mem_add(enc, usual);
	}
}

/**
 *	Emit struct declaration
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_struct_declaration(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_B);
	mem_add(enc, 0);
	proc_set(enc, (size_t)node_get_arg(nd, 0), (item_t)mem_size(enc));

	const size_t children = node_get_amount(nd);
	for (size_t i = 0; i < children - 1; i++)
	{
		const node child = node_get_child(nd, i);
		emit_declaration(enc, &child);
	}

	const node decl_end = node_get_child(nd, children - 1);
	const size_t num_proc = (size_t)node_get_arg(&decl_end, 0);

	mem_add(enc, IC_STOP);
	mem_set(enc, (size_t)proc_get(enc, num_proc) - 1, (item_t)mem_size(enc));
}

/**
 *	Emit declaration
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_declaration(encoder *const enc, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_FUNC_DEF:
			emit_function_definition(enc, nd);
			break;

		case OP_DECL_ID:
			emit_variable_declaration(enc, nd);
			break;

		case OP_DECL_ARR:
			emit_array_declaration(enc, nd);
			break;

		case OP_DECL_STRUCT:
			emit_struct_declaration(enc, nd);
			break;

		case OP_BLOCK_END:
			// Это старый признак конца программы
			// TODO: убрать, когда уберем OP_BLOCK_END
			break;

		default:
			system_error(node_unexpected, node_get_type(nd));
			enc->was_error = 1;
			break;
	}
}


/**
 *	Emit labeled statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_labeled_statement(encoder *const enc, const node *const nd)
{
	const item_t label_id = node_get_arg(nd, 0);
	item_t addr = ident_get_displ(enc->sx, (size_t)label_id);

	if (addr < 0)
	{
		// Были переходы на метку
		while (addr)
		{
			// Проставить ссылку на метку во всех ранних переходах
			const item_t ref = mem_get(enc, (size_t)(-addr));
			mem_set(enc, (size_t)(-addr), (item_t)mem_size(enc));
			addr = ref;
		}
	}

	ident_set_displ(enc->sx, (size_t)label_id, (item_t)mem_size(enc));
}

/**
 *	Emit case statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_case_statement(encoder *const enc, const node *const nd)
{
	if (enc->addr_case)
	{
		mem_set(enc, enc->addr_case, (item_t)mem_size(enc));
	}

	mem_add(enc, IC_DUPLICATE);

	const node expression = node_get_child(nd, 0);
	emit_expression(enc, &expression);

	mem_add(enc, IC_EQ);
	mem_add(enc, IC_BE0);
	enc->addr_case = mem_reserve(enc);

	const node statement = node_get_child(nd, 1);
	emit_statement(enc, &statement);
}

/**
 *	Emit default statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_default_statement(encoder *const enc, const node *const nd)
{
	if (enc->addr_case)
	{
		mem_set(enc, enc->addr_case, (item_t)mem_size(enc));
	}
	enc->addr_case = 0;

	const node statement = node_get_child(nd, 0);
	emit_statement(enc, &statement);
}

/**
 *	Emit compound statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_block(encoder *const enc, const node *const nd)
{
	const size_t block_items = node_get_amount(nd);
	for (size_t i = 0; i < block_items; i++)
	{
		const node block_item = node_get_child(nd, i);
		emit_statement(enc, &block_item);
	}
}

/**
 *	Emit if statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_if_statement(encoder *const enc, const node *const nd)
{
	const node condition = node_get_child(nd, 0);
	emit_expression(enc, &condition);
	mem_add(enc, IC_BE0);
	size_t addr = mem_reserve(enc);

	const node then_stmt = node_get_child(nd, 1);
	emit_statement(enc, &then_stmt);

	const node else_stmt = node_get_child(nd, 2);
	if (node_is_correct(&else_stmt))
	{
		mem_set(enc, addr, (item_t)mem_size(enc) + 2);
		mem_add(enc, IC_B);
		addr = mem_reserve(enc);

		emit_statement(enc, &else_stmt);
	}

	mem_set(enc, addr, (item_t)mem_size(enc));
}

/**
 *	Emit switch statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_switch_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_case = enc->addr_case;
	enc->addr_break = 0;
	enc->addr_case = 0;

	const node condition = node_get_child(nd, 0);
	emit_expression(enc, &condition);

	const node statement = node_get_child(nd, 1);
	emit_statement(enc, &statement);

	if (enc->addr_case > 0)
	{
		mem_set(enc, enc->addr_case, (item_t)mem_size(enc));
	}
	addr_end_break(enc);

	enc->addr_case = old_addr_case;
	enc->addr_break = old_addr_break;
}

/**
 *	Emit while statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_while_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	const size_t addr = mem_size(enc);
	enc->addr_cond = addr;

	const node condition = node_get_child(nd, 0);
	emit_expression(enc, &condition);

	mem_add(enc, IC_BE0);
	enc->addr_break = mem_size(enc);
	mem_add(enc, 0);

	const node statement = node_get_child(nd, 1);
	emit_statement(enc, &statement);

	addr_begin_condition(enc, addr);
	mem_add(enc, IC_B);
	mem_add(enc, (item_t)addr);
	addr_end_break(enc);

	enc->addr_break = old_addr_break;
	enc->addr_cond = old_addr_cond;
}

/**
 *	Emit do statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_do_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	const item_t addr = (item_t)mem_size(enc);

	enc->addr_cond = 0;
	enc->addr_break = 0;

	const node statement = node_get_child(nd, 0);
	emit_statement(enc, &statement);
	addr_end_condition(enc);

	const node expression = node_get_child(nd, 1);
	emit_expression(enc, &expression);
	mem_add(enc, IC_BNE0);
	mem_add(enc, addr);
	addr_end_break(enc);

	enc->addr_break = old_addr_break;
	enc->addr_cond = old_addr_cond;
}

/**
 *	Emit for statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_for_statement(encoder *const enc, const node *const nd)
{
	size_t child_index = 0;

	const bool has_inition = (bool)node_get_arg(nd, 0);
	if (has_inition)
	{
		// Предполагая, что дерево правильно построено
		const node inition = node_get_child(nd, child_index++);
		emit_statement(enc, &inition);
	}

	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	enc->addr_cond = 0;
	enc->addr_break = 0;

	const size_t addr_inition = mem_size(enc);
	const bool has_condition = (bool)node_get_arg(nd, 1);
	if (has_condition)
	{
		const node condition = node_get_child(nd, child_index++);
		emit_expression(enc, &condition);
		mem_add(enc, IC_BE0);
		enc->addr_break = mem_size(enc);
		mem_add(enc, 0);
	}

	const bool has_increment = (bool)node_get_arg(nd, 2);

	const node statement = node_get_child(nd, child_index + has_increment);
	emit_statement(enc, &statement);
	addr_end_condition(enc);

	if (has_increment)
	{
		const node increment = node_get_child(nd, child_index);
		emit_expression(enc, &increment);
	}

	mem_add(enc, IC_B);
	mem_add(enc, (item_t)addr_inition);
	addr_end_break(enc);

	enc->addr_break = old_addr_break;
	enc->addr_cond = old_addr_cond;
}

/**
 *	Emit goto statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_goto_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_B);

	const item_t label_id = node_get_arg(nd, 0);
	const size_t id = (size_t)llabs(label_id);
	const item_t addr = ident_get_displ(enc->sx, id);

	if (addr > 0)
	{
		// Метка уже описана
		mem_add(enc, addr);
	}
	else
	{
		// Метка еще не описана
		ident_set_displ(enc->sx, id, -(item_t)mem_size(enc));

		// Если эта метка уже встречалась, ставим адрес предыдущего перехода сюда
		mem_add(enc, label_id < 0 ? 0 : addr);
	}
}

/**
 *	Emit continue statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_continue_statement(encoder *const enc, const node *const nd)
{
	(void)nd;
	mem_add(enc, IC_B);
	mem_add(enc, (item_t)enc->addr_cond);
	enc->addr_cond = mem_size(enc) - 1;
}

/**
 *	Emit break statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_break_statement(encoder *const enc, const node *const nd)
{
	(void)nd;
	mem_add(enc, IC_B);
	mem_add(enc, (item_t)enc->addr_break);
	enc->addr_break = mem_size(enc) - 1;
}

/**
 *	Emit return statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_return_statement(encoder *const enc, const node *const nd)
{
	if (node_get_amount(nd))
	{
		const node expression = node_get_child(nd, 0);
		emit_expression(enc, &expression);

		mem_add(enc, IC_RETURN_VAL);
		mem_add(enc, node_get_arg(nd, 0));
	}
	else
	{
		mem_add(enc, IC_RETURN_VOID);
	}
}

/**
 *	Emit t_create_direct statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_thread(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_CREATE_DIRECT);

	const size_t thread_items = node_get_amount(nd);
	for (size_t i = 0; i < thread_items; i++)
	{
		const node thread_item = node_get_child(nd, i);
		emit_statement(enc, &thread_item);
	}

	mem_add(enc, IC_EXIT_DIRECT);
}

static void compress_ident(encoder *const enc, const size_t ref)
{
	if (vector_get(&enc->sx->identifiers, ref) == ITEM_MAX)
	{
		mem_add(enc, ident_get_repr(enc->sx, ref));
		return;
	}

	const item_t new_ref = (item_t)vector_size(&enc->identifiers) - 1;
	vector_add(&enc->identifiers, (item_t)vector_size(&enc->representations) - 2);
	vector_add(&enc->identifiers, ident_get_type(enc->sx, ref));
	vector_add(&enc->identifiers, ident_get_displ(enc->sx, ref));

	const char *buffer = repr_get_name(enc->sx, (size_t)ident_get_repr(enc->sx, ref));
	for (size_t i = 0; buffer[i] != '\0'; i += utf8_symbol_size(buffer[i]))
	{
		vector_add(&enc->representations, (item_t)utf8_convert(&buffer[i]));
	}
	vector_add(&enc->representations, '\0');

	vector_set(&enc->sx->identifiers, ref, ITEM_MAX);
	ident_set_repr(enc->sx, ref, new_ref);
	mem_add(enc, new_ref);
}

/**
 *	Emit printid statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_printid_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_PRINTID);
	compress_ident(enc, (size_t)node_get_arg(nd, 0)); // Ссылка в identtab
}

/**
 *	Emit getid statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_getid_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_GETID);
	compress_ident(enc, (size_t)node_get_arg(nd, 0)); //Сссылка в identtab
}

/**
 *	Emit printf statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_printf_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_PRINTF);
	mem_add(enc, node_get_arg(nd, 0)); // Общий размер того, что надо вывести
}

/**
 *	Emit statement
 *
 *	@param	enc			Code generator
 *	@param	nd			Node in AST
 */
static void emit_statement(encoder *const enc, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_LABEL:
			emit_labeled_statement(enc, nd);
			break;

		case OP_CASE:
			emit_case_statement(enc, nd);
			break;

		case OP_DEFAULT:
			emit_default_statement(enc, nd);
			break;

		case OP_BLOCK:
			emit_block(enc, nd);
			break;

		case OP_NOP:
			break;

		case OP_IF:
			emit_if_statement(enc, nd);
			break;

		case OP_SWITCH:
			emit_switch_statement(enc, nd);
			break;

		case OP_WHILE:
			emit_while_statement(enc, nd);
			break;

		case OP_DO:
			emit_do_statement(enc, nd);
			break;

		case OP_FOR:
			emit_for_statement(enc, nd);
			break;

		case OP_GOTO:
			emit_goto_statement(enc, nd);
			break;

		case OP_CONTINUE:
			emit_continue_statement(enc, nd);
			break;

		case OP_BREAK:
			emit_break_statement(enc, nd);
			break;

		case OP_RETURN_VAL:
		case OP_RETURN_VOID:
			emit_return_statement(enc, nd);
			break;

		case OP_CREATE_DIRECT:
		case OP_CREATE:
			emit_thread(enc, nd);
			break;

		case OP_PRINTID:
			emit_printid_statement(enc, nd);
			break;

		case OP_GETID:
			emit_getid_statement(enc, nd);
			break;

		case OP_PRINTF:
			emit_printf_statement(enc, nd);
			break;

		case OP_BLOCK_END:
		case OP_EXIT_DIRECT:
		case OP_EXIT:
			// Пока что это чтобы не менять дерево
			// Но на самом деле такие узлы не нужны, так как реализация дерева знает количество потомков
			break;

		case OP_DECL_ID:
		case OP_DECL_ARR:
		case OP_DECL_STRUCT:
			emit_declaration(enc, nd);
			break;

		default:
			emit_expression(enc, nd);
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

	encoder enc = enc_create(ws, sx);
	const node root = node_get_root(&sx->tree);
	const size_t items = node_get_amount(&root);
	for (size_t i = 0; i < items && !enc.was_error; i++)
	{
		const node item = node_get_child(&root, i);
		emit_declaration(&enc, &item);
	}

	mem_add(&enc, IC_CALL1);
	mem_add(&enc, IC_CALL2);
	mem_add(&enc, ident_get_displ(enc.sx, enc.sx->ref_main));
	mem_add(&enc, IC_STOP);

	int ret = enc.was_error || enc_export(io, &enc);

#ifndef NDEBUG
	tables_and_codes(DEFAULT_CODES, &sx->functions, &enc.processes, &enc.memory);
#endif

	enc_clear(&enc);
	return ret;
}
