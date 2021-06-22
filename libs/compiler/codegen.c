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
typedef struct codegenerator
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
} codegenerator;

/**
 *	Create virtual machine codes emitter structure
 *
 *	@param	ws		Compiler workspace
 *	@param	sx		Syntax structure
 *
 *	@return	Virtual machine environment structure
 */
static codegenerator cg_create(const workspace *const ws, syntax *const sx)
{
	codegenerator cg;
	cg.sx = sx;

	cg.memory = vector_create(MAX_MEM_SIZE);
	cg.processes = vector_create(sx->procd);
	cg.stk = stack_create(MAX_STACK_SIZE);

	const size_t records = vector_size(&sx->identifiers) / 4;
	cg.identifiers = vector_create(records * 3);
	cg.representations = vector_create(records * 8);

	vector_increase(&cg.memory, 4);
	vector_increase(&cg.processes, sx->procd);
	cg.max_threads = 0;

	cg.target = item_get_status(ws);

	cg.was_error = 0;

	return cg;
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
 *	@param	ws		Compiler workspace
 */
static int cg_export(universal_io *const io, const codegenerator *const cg)
{
	uni_printf(io, "#!/usr/bin/ruc-vm\n");

	uni_printf(io, "%zi %zi %zi %zi %zi %" PRIitem " %zi\n"
		, vector_size(&cg->memory)
		, vector_size(&cg->sx->functions)
		, vector_size(&cg->identifiers)
		, vector_size(&cg->representations)
		, vector_size(&cg->sx->types)
		, cg->sx->max_displg, cg->max_threads);

	return print_table(io, cg->target, &cg->memory)
		|| print_table(io, cg->target, &cg->sx->functions)
		|| print_table(io, cg->target, &cg->identifiers)
		|| print_table(io, cg->target, &cg->representations)
		|| print_table(io, cg->target, &cg->sx->types);
}

/**
 *	Clear virtual machine environment structure
 *
 *	@param	ws		Compiler workspace
 */
static void cg_clear(codegenerator *const cg)
{
	vector_clear(&cg->memory);
	vector_clear(&cg->processes);
	stack_clear(&cg->stk);

	vector_clear(&cg->identifiers);
	vector_clear(&cg->representations);
}


static void emit_statement(codegenerator *const cg, const node *const nd);


static inline void mem_increase(codegenerator *const cg, const size_t size)
{
	vector_increase(&cg->memory, size);
}

static inline void mem_add(codegenerator *const cg, const item_t value)
{
	vector_add(&cg->memory, value);
}

static inline void mem_set(codegenerator *const cg, const size_t index, const item_t value)
{
	vector_set(&cg->memory, index, value);
}

static inline item_t mem_get(const codegenerator *const cg, const size_t index)
{
	return vector_get(&cg->memory, index);
}

static inline size_t mem_size(const codegenerator *const cg)
{
	return vector_size(&cg->memory);
}

static inline size_t mem_reserve(codegenerator *const cg)
{
	vector_increase(&cg->memory, 1);
	return mem_size(cg) - 1;
}


static inline void proc_set(codegenerator *const cg, const size_t index, const item_t value)
{
	vector_set(&cg->processes, index, value);
}

static inline item_t proc_get(const codegenerator *const cg, const size_t index)
{
	return vector_get(&cg->processes, index);
}


static void addr_begin_condition(codegenerator *const cg, const size_t addr)
{
	while (cg->addr_cond != addr)
	{
		const size_t ref = (size_t)mem_get(cg, cg->addr_cond);
		mem_set(cg, cg->addr_cond, (item_t)addr);
		cg->addr_cond = ref;
	}
}

static void addr_end_condition(codegenerator *const cg)
{
	while (cg->addr_cond)
	{
		const size_t ref = (size_t)mem_get(cg, cg->addr_cond);
		mem_set(cg, cg->addr_cond, (item_t)mem_size(cg));
		cg->addr_cond = ref;
	}
}

static void addr_end_break(codegenerator *const cg)
{
	while (cg->addr_break)
	{
		const size_t ref = (size_t)mem_get(cg, cg->addr_break);
		mem_set(cg, cg->addr_break, (item_t)mem_size(cg));
		cg->addr_break = ref;
	}
}


static void compress_ident(codegenerator *const cg, const size_t ref)
{
	if (vector_get(&cg->sx->identifiers, ref) == ITEM_MAX)
	{
		mem_add(cg, ident_get_repr(cg->sx, ref));
		return;
	}

	const item_t new_ref = (item_t)vector_size(&cg->identifiers) - 1;
	vector_add(&cg->identifiers, (item_t)vector_size(&cg->representations) - 2);
	vector_add(&cg->identifiers, ident_get_type(cg->sx, ref));
	vector_add(&cg->identifiers, ident_get_displ(cg->sx, ref));

	const char *buffer = repr_get_name(cg->sx, (size_t)ident_get_repr(cg->sx, ref));
	for (size_t i = 0; buffer[i] != '\0'; i += utf8_symbol_size(buffer[i]))
	{
		vector_add(&cg->representations, (item_t)utf8_convert(&buffer[i]));
	}
	vector_add(&cg->representations, '\0');

	vector_set(&cg->sx->identifiers, ref, ITEM_MAX);
	ident_set_repr(cg->sx, ref, new_ref);
	mem_add(cg, new_ref);
}


static void final_operation(codegenerator *const cg, node *const nd)
{
	operation_t op = node_get_type(nd);
	while (op > BEGIN_OP_FINAL && op < END_OP_FINAL)
	{
		if (op != OP_NOP)
		{
			if (op == OP_AD_LOG_OR)
			{
				mem_add(cg, IC_DUPLICATE);
				mem_add(cg, IC_BNE0);
				stack_push(&cg->stk, (item_t)mem_size(cg));
				mem_increase(cg, 1);
			}
			else if (op == OP_AD_LOG_AND)
			{
				mem_add(cg, IC_DUPLICATE);
				mem_add(cg, IC_BE0);
				stack_push(&cg->stk, (item_t)mem_size(cg));
				mem_increase(cg, 1);
			}
			else
			{
				mem_add(cg, (instruction_t)op);
				if (op == OP_LOG_OR || op == OP_LOG_AND)
				{
					mem_set(cg, (size_t)stack_pop(&cg->stk), (item_t)mem_size(cg));
				}
				else if (op == OP_COPY00 || op == OP_COPYST)
				{
					mem_add(cg, node_get_arg(nd, 0)); // d1
					mem_add(cg, node_get_arg(nd, 1)); // d2
					mem_add(cg, node_get_arg(nd, 2)); // длина
				}
				else if (op == OP_COPY01 || op == OP_COPY10 || op == OP_COPY0ST || op == OP_COPY0ST_ASSIGN)
				{
					mem_add(cg, node_get_arg(nd, 0)); // d1
					mem_add(cg, node_get_arg(nd, 1)); // длина
				}
				else if (op == OP_COPY11 || op == OP_COPY1ST || op == OP_COPY1ST_ASSIGN)
				{
					mem_add(cg, node_get_arg(nd, 0)); // длина
				}
				else if (operation_is_assignment(op))
				{
					mem_add(cg, node_get_arg(nd, 0));
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
 *	@param	cg		Virtual machine environment
 *	@param	mode	@c -1 for expression on the same node,
 *					@c  0 for usual expression,
 *					@c  1 for expression in condition
 */
static void expression(codegenerator *const cg, node *const nd, int mode)
{
	while (node_get_type(nd) != OP_EXPR_END)
	{
		const operation_t operation = node_get_type(nd);
		int was_operation = 1;

		switch (operation)
		{
			case OP_IDENT:
				break;
			case OP_IDENT_TO_ADDR:
			{
				mem_add(cg, IC_LA);
				mem_add(cg, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL:
			{
				mem_add(cg, IC_LOAD);
				mem_add(cg, node_get_arg(nd, 0));
			}
			break;
			case OP_IDENT_TO_VAL_D:
			{
				mem_add(cg, IC_LOADD);
				mem_add(cg, node_get_arg(nd, 0));
			}
			break;
			case OP_ADDR_TO_VAL:
				mem_add(cg, IC_LAT);
				break;
			case OP_ADDR_TO_VAL_D:
				mem_add(cg, IC_LATD);
				break;
			case OP_CONST:
			{
				mem_add(cg, IC_LI);
				mem_add(cg, node_get_arg(nd, 0));
			}
			break;
			case OP_CONST_D:
			{
				mem_add(cg, IC_LID);
				mem_add(cg, node_get_arg(nd, 0));
				mem_add(cg, node_get_arg(nd, 1));
			}
			break;
			case OP_STRING:
			case OP_STRING_D:
			{
				mem_add(cg, IC_LI);
				const size_t reserved = mem_size(cg) + 4;
				mem_add(cg, (item_t)reserved);
				mem_add(cg, IC_B);
				mem_increase(cg, 2);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					if (operation == OP_STRING)
					{
						mem_add(cg, node_get_arg(nd, (size_t)i + 1));
					}
					else
					{
						mem_add(cg, node_get_arg(nd, 2 * (size_t)i + 1));
						mem_add(cg, node_get_arg(nd, 2 * (size_t)i + 2));
					}
				}

				mem_set(cg, reserved - 1, N);
				mem_set(cg, reserved - 2, (item_t)mem_size(cg));
			}
			break;
			case OP_ARRAY_INIT:
			{
				const item_t N = node_get_arg(nd, 0);

				mem_add(cg, IC_BEG_INIT);
				mem_add(cg, N);

				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(cg, nd, 0);
				}
			}
			break;
			case OP_STRUCT_INIT:
			{
				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(cg, nd, 0);
				}
			}
			break;
			case OP_SLICE_IDENT:
			{
				mem_add(cg, IC_LOAD); // параметры - смещение идента и тип элемента
				mem_add(cg, node_get_arg(nd, 0)); // продолжение в след case
			}
			case OP_SLICE: // параметр - тип элемента
			{
				item_t type = node_get_arg(nd, operation == OP_SLICE ? 0 : 1);

				node_set_next(nd);
				expression(cg, nd, 0);
				mem_add(cg, IC_SLICE);
				mem_add(cg, (item_t)size_of(cg->sx, type));
				if (type_is_array(cg->sx, type))
				{
					mem_add(cg, IC_LAT);
				}
			}
			break;
			case OP_SELECT:
			{
				mem_add(cg, IC_SELECT); // SELECT field_displ
				mem_add(cg, node_get_arg(nd, 0));
			}
			break;
			case OP_PRINT:
			{
				mem_add(cg, IC_PRINT);
				mem_add(cg, node_get_arg(nd, 0)); // type
			}
			break;
			case OP_CALL1:
			{
				mem_add(cg, IC_CALL1);

				const item_t N = node_get_arg(nd, 0);
				for (item_t i = 0; i < N; i++)
				{
					node_set_next(nd);
					expression(cg, nd, 0);
				}
			}
			break;
			case OP_CALL2:
			{
				mem_add(cg, IC_CALL2);
				mem_add(cg, ident_get_displ(cg->sx, (size_t)node_get_arg(nd, 0)));
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

		final_operation(cg, nd);

		if (node_get_type(nd) == OP_CONDITIONAL)
		{
			if (mode == 1)
			{
				return;
			}

			size_t addr = 0;
			do
			{
				mem_add(cg, IC_BE0);
				const size_t addr_else = mem_size(cg);
				mem_increase(cg, 1);

				node_set_next(nd);
				expression(cg, nd, 0); // then
				mem_add(cg, IC_B);
				mem_add(cg, (item_t)addr);
				addr = mem_size(cg) - 1;
				mem_set(cg, addr_else, (item_t)mem_size(cg));

				node_set_next(nd);
				expression(cg, nd, 1); // else или cond
			} while (node_get_type(nd) == OP_CONDITIONAL);

			while (addr)
			{
				const size_t ref = (size_t)mem_get(cg, addr);
				mem_set(cg, addr, (item_t)mem_size(cg));
				addr = ref;
			}

			final_operation(cg, nd);
		}
	}
}

/**
 *	Emit expression [C99 6.5]
 *
 *	@note	Это переходник под старый expression(), который использует set_next()
 *	TODO:	переписать генерацию выражений
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_expression(codegenerator *const cg, const node *const nd)
{
	node internal_node;
	node_copy(&internal_node, nd);
	expression(cg, &internal_node, 0);
}

/**
 *	Emit declaration [C99 6.7]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_declaration(codegenerator *const cg, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_FUNC_DEF:
		{
			const size_t ref_func = (size_t)ident_get_displ(cg->sx, (size_t)node_get_arg(nd, 0));
			func_set(cg->sx, ref_func, (item_t)mem_size(cg));

			mem_add(cg, IC_FUNC_BEG);
			mem_add(cg, node_get_arg(nd, 1));

			const size_t old_pc = mem_reserve(cg);

			const node func_body = node_get_child(nd, 0);
			emit_statement(cg, &func_body);

			mem_set(cg, old_pc, (item_t)mem_size(cg));
		}
		break;

		case OP_DECL_ID:
		{
			const item_t old_displ = node_get_arg(nd, 0);
			const item_t type = node_get_arg(nd, 1);
			const item_t process = node_get_arg(nd, 4);

			if (process)
			{
				mem_add(cg, IC_STRUCT_WITH_ARR);
				mem_add(cg, old_displ);
				mem_add(cg, proc_get(cg, (size_t)process));
			}

			const node initializer = node_get_child(nd, 0);
			if (node_is_correct(&initializer)) // int a = или struct{} a =
			{
				emit_expression(cg, &initializer);

				if (type_is_struct(cg->sx, type))
				{
					mem_add(cg, IC_COPY0ST_ASSIGN);
					mem_add(cg, old_displ);
					mem_add(cg, node_get_arg(nd, 3)); // Общее количество слов
				}
				else
				{
					mem_add(cg, type_is_float(type) ? IC_ASSIGN_R_V : IC_ASSIGN_V);
					mem_add(cg, old_displ);
				}
			}
		}
		break;

		case OP_DECL_ARR:
		{
			const size_t bounds = (size_t)node_get_arg(nd, 0);
			for (size_t i = 0; i < bounds; i++)
			{
				const node expression = node_get_child(nd, i);
				emit_expression(cg, &expression);
			}

			const node decl_id = node_get_child(nd, bounds);

			int has_initializer = 0;
			const node initializer = node_get_child(nd, bounds + 1);
			if (node_is_correct(&initializer))
			{
				has_initializer = 1;
			}

			mem_add(cg, IC_DEFARR); // DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке

			const item_t dimensions = node_get_arg(&decl_id, 2);
			mem_add(cg, has_initializer ? dimensions - 1 : dimensions);

			const item_t length = (item_t)size_of(cg->sx, node_get_arg(&decl_id, 1));
			mem_add(cg, length);

			const item_t old_displ = node_get_arg(&decl_id, 0);
			mem_add(cg, old_displ);
			mem_add(cg, proc_get(cg, (size_t)node_get_arg(&decl_id, 4)));

			/*
			 *	@param	usual	Для массивов:
			 *						@c 0 с пустыми границами,
			 *						@c 1 без пустых границ
			 */
			const item_t usual = node_get_arg(&decl_id, 5);
			mem_add(cg, usual);
			mem_add(cg, node_get_arg(&decl_id, 3));
			mem_add(cg, node_get_arg(&decl_id, 6));

			if (has_initializer)
			{
				emit_expression(cg, &initializer);

				mem_add(cg, IC_ARR_INIT);
				mem_add(cg, dimensions);
				mem_add(cg, length);
				mem_add(cg, old_displ);
				mem_add(cg, usual);
			}
		}
		break;

		case OP_DECL_STRUCT:
		{
			mem_add(cg, IC_B);
			mem_add(cg, 0);
			proc_set(cg, (size_t)node_get_arg(nd, 0), (item_t)mem_size(cg));

			const size_t children = node_get_amount(nd);
			for (size_t i = 0; i < children - 1; i++)
			{
				const node child = node_get_child(nd, i);
				emit_declaration(cg, &child);
			}

			const node decl_end = node_get_child(nd, children - 1);
			const size_t num_proc = (size_t)node_get_arg(&decl_end, 0);

			mem_add(cg, IC_STOP);
			mem_set(cg, (size_t)proc_get(cg, num_proc) - 1, (item_t)mem_size(cg));
		}
		break;

		case OP_BLOCK_END:
			// Это старый признак конца программы
			// TODO: убрать, когда уберем OP_BLOCK_END
			break;

		default:
			system_error(node_unexpected, node_get_type(nd));
			cg->was_error = 1;
			break;
	}
}

/**
 *	Emit labeled statement [C99 6.8.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_labeled_statement(codegenerator *const cg, const node *const nd)
{
	const item_t label_id = node_get_arg(nd, 0);
	item_t addr = ident_get_displ(cg->sx, (size_t)label_id);

	if (addr < 0)
	{
		// Были переходы на метку
		while (addr)
		{
			// Проставить ссылку на метку во всех ранних переходах
			const item_t ref = mem_get(cg, (size_t)(-addr));
			mem_set(cg, (size_t)(-addr), (item_t)mem_size(cg));
			addr = ref;
		}
	}

	ident_set_displ(cg->sx, (size_t)label_id, (item_t)mem_size(cg));
}

/**
 *	Emit case statement [C99 6.8.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_case_statement(codegenerator *const cg, const node *const nd)
{
	if (cg->addr_case)
	{
		mem_set(cg, cg->addr_case, (item_t)mem_size(cg));
	}

	mem_add(cg, IC_DUPLICATE);

	const node expression = node_get_child(nd, 0);
	emit_expression(cg, &expression);

	mem_add(cg, IC_EQ);
	mem_add(cg, IC_BE0);
	cg->addr_case = mem_reserve(cg);

	const node statement = node_get_child(nd, 1);
	emit_statement(cg, &statement);
}

/**
 *	Emit default statement [C99 6.8.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_default_statement(codegenerator *const cg, const node *const nd)
{
	if (cg->addr_case)
	{
		mem_set(cg, cg->addr_case, (item_t)mem_size(cg));
	}
	cg->addr_case = 0;

	const node statement = node_get_child(nd, 0);
	emit_statement(cg, &statement);
}

/**
 *	Emit compound statement [C99 6.8.2]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_block(codegenerator *const cg, const node *const nd)
{
	const size_t block_items = node_get_amount(nd);
	for (size_t i = 0; i < block_items; i++)
	{
		const node block_item = node_get_child(nd, i);
		emit_statement(cg, &block_item);
	}
}

/**
 *	Emit if statement [C99 6.8.4.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_if_statement(codegenerator *const cg, const node *const nd)
{
	const node condition = node_get_child(nd, 0);
	emit_expression(cg, &condition);
	mem_add(cg, IC_BE0);
	size_t addr = mem_reserve(cg);

	const node then_stmt = node_get_child(nd, 1);
	emit_statement(cg, &then_stmt);

	const node else_stmt = node_get_child(nd, 2);
	if (node_is_correct(&else_stmt))
	{
		mem_set(cg, addr, (item_t)mem_size(cg) + 2);
		mem_add(cg, IC_B);
		addr = mem_reserve(cg);

		emit_statement(cg, &else_stmt);
	}

	mem_set(cg, addr, (item_t)mem_size(cg));
}

/**
 *	Emit switch statement [C99 6.8.4.2]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_switch_statement(codegenerator *const cg, const node *const nd)
{
	const size_t old_addr_break = cg->addr_break;
	const size_t old_addr_case = cg->addr_case;
	cg->addr_break = 0;
	cg->addr_case = 0;

	const node condition = node_get_child(nd, 0);
	emit_expression(cg, &condition);

	const node statement = node_get_child(nd, 1);
	emit_statement(cg, &statement);

	if (cg->addr_case > 0)
	{
		mem_set(cg, cg->addr_case, (item_t)mem_size(cg));
	}
	addr_end_break(cg);

	cg->addr_case = old_addr_case;
	cg->addr_break = old_addr_break;
}

/**
 *	Emit while statement [C99 6.8.5.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_while_statement(codegenerator *const cg, const node *const nd)
{
	const size_t old_addr_break = cg->addr_break;
	const size_t old_addr_cond = cg->addr_cond;
	const size_t addr = mem_size(cg);
	cg->addr_cond = addr;

	const node condition = node_get_child(nd, 0);
	emit_expression(cg, &condition);

	mem_add(cg, IC_BE0);
	cg->addr_break = mem_size(cg);
	mem_add(cg, 0);

	const node statement = node_get_child(nd, 1);
	emit_statement(cg, &statement);

	addr_begin_condition(cg, addr);
	mem_add(cg, IC_B);
	mem_add(cg, (item_t)addr);
	addr_end_break(cg);

	cg->addr_break = old_addr_break;
	cg->addr_cond = old_addr_cond;
}

/**
 *	Emit do statement [C99 6.8.5.2]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_do_statement(codegenerator *const cg, const node *const nd)
{
	const size_t old_addr_break = cg->addr_break;
	const size_t old_addr_cond = cg->addr_cond;
	const item_t addr = (item_t)mem_size(cg);

	cg->addr_cond = 0;
	cg->addr_break = 0;

	const node statement = node_get_child(nd, 0);
	emit_statement(cg, &statement);
	addr_end_condition(cg);

	const node expression = node_get_child(nd, 1);
	emit_expression(cg, &expression);
	mem_add(cg, IC_BNE0);
	mem_add(cg, addr);
	addr_end_break(cg);

	cg->addr_break = old_addr_break;
	cg->addr_cond = old_addr_cond;
}

/**
 *	Emit for statement [C99 6.8.5.3]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_for_statement(codegenerator *const cg, const node *const nd)
{
	size_t child_index = 0;

	const int has_inition = (int)node_get_arg(nd, 0);
	if (has_inition)
	{
		// Предполагая, что дерево правильно построено
		const node inition = node_get_child(nd, child_index++);
		emit_statement(cg, &inition);
	}

	const size_t old_addr_break = cg->addr_break;
	const size_t old_addr_cond = cg->addr_cond;
	cg->addr_cond = 0;
	cg->addr_break = 0;

	const size_t addr_inition = mem_size(cg);
	const int has_condition = (int)node_get_arg(nd, 1);
	if (has_condition)
	{
		const node condition = node_get_child(nd, child_index++);
		emit_expression(cg, &condition);
		mem_add(cg, IC_BE0);
		cg->addr_break = mem_size(cg);
		mem_add(cg, 0);
	}

	const int has_increment = (int)node_get_arg(nd, 2);

	const node statement = node_get_child(nd, child_index + has_increment);
	emit_statement(cg, &statement);
	addr_end_condition(cg);

	if (has_increment)
	{
		const node increment = node_get_child(nd, child_index);
		emit_expression(cg, &increment);
	}

	mem_add(cg, IC_B);
	mem_add(cg, (item_t)addr_inition);
	addr_end_break(cg);

	cg->addr_break = old_addr_break;
	cg->addr_cond = old_addr_cond;
}

/**
 *	Emit goto statement [C99 6.8.6.1]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_goto_statement(codegenerator *const cg, const node *const nd)
{
	mem_add(cg, IC_B);

	const item_t label_id = node_get_arg(nd, 0);
	const size_t id = (size_t)llabs(label_id);
	const item_t addr = ident_get_displ(cg->sx, id);

	if (addr > 0)
	{
		// Метка уже описана
		mem_add(cg, addr);
	}
	else
	{
		// Метка еще не описана
		ident_set_displ(cg->sx, id, -(item_t)mem_size(cg));

		// Первый раз встретился переход на еще не описанную метку или нет
		mem_add(cg, label_id < 0 ? 0 : addr);
	}
}

/**
 *	Emit continue statement [C99 6.8.6.2]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_continue_statement(codegenerator *const cg, const node *const nd)
{
	(void)nd;
	mem_add(cg, IC_B);
	mem_add(cg, (item_t)cg->addr_cond);
	cg->addr_cond = mem_size(cg) - 1;
}

/**
 *	Emit break statement [C99 6.8.6.3]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_break_statement(codegenerator *const cg, const node *const nd)
{
	(void)nd;
	mem_add(cg, IC_B);
	mem_add(cg, (item_t)cg->addr_break);
	cg->addr_break = mem_size(cg) - 1;
}

/**
 *	Emit return statement [C99 6.8.6.4]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_return_statement(codegenerator *const cg, const node *const nd)
{
	if (node_get_amount(nd))
	{
		const node expression = node_get_child(nd, 0);
		emit_expression(cg, &expression);

		mem_add(cg, IC_RETURN_VAL);
		mem_add(cg, node_get_arg(nd, 0));
	}
	else
	{
		mem_add(cg, IC_RETURN_VOID);
	}
}

/**
 *	Emit t_create_direct statement [RuC]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_thread(codegenerator *const cg, const node *const nd)
{
	mem_add(cg, IC_CREATE_DIRECT);

	const size_t thread_items = node_get_amount(nd);
	for (size_t i = 0; i < thread_items; i++)
	{
		const node thread_item = node_get_child(nd, i);
		emit_statement(cg, &thread_item);
	}

	mem_add(cg, IC_EXIT_DIRECT);
}

/**
 *	Emit printid statement [RuC]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_printid_statement(codegenerator *const cg, const node *const nd)
{
	mem_add(cg, IC_PRINTID);
	compress_ident(cg, (size_t)node_get_arg(nd, 0)); // Ссылка в identtab
}

/**
 *	Emit getid statement [RuC]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_getid_statement(codegenerator *const cg, const node *const nd)
{
	mem_add(cg, IC_GETID);
	compress_ident(cg, (size_t)node_get_arg(nd, 0)); //Сссылка в identtab
}

/**
 *	Emit printf statement [RuC]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_printf_statement(codegenerator *const cg, const node *const nd)
{
	mem_add(cg, IC_PRINTF);
	mem_add(cg, node_get_arg(nd, 0)); // Общий размер того, что надо вывести
}

/**
 *	Emit statement [C99 6.8]
 *
 *	@param	cg			Virtual machine environment
 *	@param	nd			Node in AST
 */
static void emit_statement(codegenerator *const cg, const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_LABEL:
			emit_labeled_statement(cg, nd);
			break;

		case OP_CASE:
			emit_case_statement(cg, nd);
			break;

		case OP_DEFAULT:
			emit_default_statement(cg, nd);
			break;

		case OP_BLOCK:
			emit_block(cg, nd);
			break;

		case OP_NOP:
			break;

		case OP_IF:
			emit_if_statement(cg, nd);
			break;

		case OP_SWITCH:
			emit_switch_statement(cg, nd);
			break;

		case OP_WHILE:
			emit_while_statement(cg, nd);
			break;

		case OP_DO:
			emit_do_statement(cg, nd);
			break;

		case OP_FOR:
			emit_for_statement(cg, nd);
			break;

		case OP_GOTO:
			emit_goto_statement(cg, nd);
			break;

		case OP_CONTINUE:
			emit_continue_statement(cg, nd);
			break;

		case OP_BREAK:
			emit_break_statement(cg, nd);
			break;

		case OP_RETURN_VAL:
		case OP_RETURN_VOID:
			emit_return_statement(cg, nd);
			break;

		case OP_CREATE_DIRECT:
		case OP_CREATE:
			emit_thread(cg, nd);
			break;

		case OP_PRINTID:
			emit_printid_statement(cg, nd);
			break;

		case OP_GETID:
			emit_getid_statement(cg, nd);
			break;

		case OP_PRINTF:
			emit_printf_statement(cg, nd);
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
			emit_declaration(cg, nd);
			break;

		default:
			emit_expression(cg, nd);
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

	codegenerator cg = cg_create(ws, sx);
	const node root = node_get_root(&cg.sx->tree);
	const size_t items = node_get_amount(&root);
	for (size_t i = 0; i < items && !cg.was_error; i++)
	{
		const node item = node_get_child(&root, i);
		emit_declaration(&cg, &item);
	}

	mem_add(&cg, IC_CALL1);
	mem_add(&cg, IC_CALL2);
	mem_add(&cg, ident_get_displ(cg.sx, cg.sx->ref_main));
	mem_add(&cg, IC_STOP);

	int ret = cg.was_error || cg_export(io, &cg);

#ifndef NDEBUG
	tables_and_codes(DEFAULT_CODES, &sx->functions, &cg.processes, &cg.memory);
#endif

	cg_clear(&cg);
	return ret;
}
