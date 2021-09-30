/*
 *	Copyright 2015 Andrey Terekhov, Victor Y. Fadeev
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
#include "AST.h"
#include "errors.h"
#include "instructions.h"
#include "item.h"
#include "string.h"
#include "tree.h"
#include "uniprinter.h"
#include "writer.h"


static const char *const DEFAULT_CODES = "codes.txt";
static const size_t MAX_MEM_SIZE = 100000;


/** Kinds of lvalue */
typedef enum OPERAND
{
	VARIABLE,		/**< Variable operand */
	ADDRESS,		/**< Address operand */
} operand_t;


/** RuC-VM Intermediate Representation encoder */
typedef struct encoder
{
	syntax *sx;						/**< Syntax structure */

	vector memory;					/**< Memory table */
	vector iniprocs;				/**< Init procedures */

	vector identifiers;				/**< Local identifiers table */
	vector representations;			/**< Local representations table */

	size_t addr_cond;				/**< Condition address */
	size_t addr_case;				/**< Case operator address */
	size_t addr_break;				/**< Break operator address */

	item_status target;				/**< Target tables item type */
} encoder;

typedef struct lvalue
{
	item_t type;					/**< Value type */
	operand_t kind;					/**< Value kind */
	item_t displ;					/**< Value displacement */
} lvalue;


static void emit_void_expression(encoder *const enc, const node *const nd);
static lvalue emit_lvalue(encoder *const enc, const node *const nd);
static void emit_expression(encoder *const enc, const node *const nd);
static void emit_statement(encoder *const enc, const node *const nd);


/*
 *	 __  __     ______   __     __         ______
 *	/\ \/\ \   /\__  _\ /\ \   /\ \       /\  ___\
 *	\ \ \_\ \  \/_/\ \/ \ \ \  \ \ \____  \ \___  \
 *	 \ \_____\    \ \_\  \ \_\  \ \_____\  \/\_____\
 *	  \/_____/     \/_/   \/_/   \/_____/   \/_____/
 */


static inline int mem_increase(encoder *const enc, const size_t size)
{
	return vector_increase(&enc->memory, size);
}

static inline size_t mem_add(encoder *const enc, const item_t value)
{
	return vector_add(&enc->memory, value);
}

static inline size_t mem_add_double(encoder *const enc, const double value)
{
	int64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const int32_t fst = num64 & 0x00000000ffffffff;
	const int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	mem_add(enc, fst);
	return mem_add(enc, snd);
}

static inline int mem_set(encoder *const enc, const size_t index, const item_t value)
{
	return vector_set(&enc->memory, index, value);
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
	return vector_add(&enc->memory, 0);
}


static inline int proc_set(encoder *const enc, const size_t index, const item_t value)
{
	return vector_set(&enc->iniprocs, index, value);
}

static inline item_t proc_get(const encoder *const enc, const size_t index)
{
	return vector_get(&enc->iniprocs, index);
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
 *	Create encoder
 *
 *	@param	ws			Compiler workspace
 *	@param	sx			Syntax structure
 *
 *	@return	Encoder
 */
static encoder enc_create(const workspace *const ws, syntax *const sx)
{
	encoder enc;
	enc.sx = sx;

	enc.memory = vector_create(MAX_MEM_SIZE);
	enc.iniprocs = vector_create(0);

	const size_t records = vector_size(&sx->identifiers) / 4;
	enc.identifiers = vector_create(records * 3);
	enc.representations = vector_create(records * 8);

	vector_increase(&enc.memory, 4);
	vector_increase(&enc.iniprocs, vector_size(&enc.sx->types));

	enc.target = item_get_status(ws);
	return enc;
}

/**
 *	Print table
 *
 *	@param	enc			Encoder
 *	@param	table		Table for printing
 *
 *	@return	@c 0 on success, @c -1 on error
 */
static int print_table(const encoder *const enc, const vector *const table)
{
	const size_t size = vector_size(table);
	for (size_t i = 0; i < size; i++)
	{
		const item_t item = vector_get(table, i);
		if (!item_check_var(enc->target, item))
		{
			system_error(tables_cannot_be_compressed);
			return -1;
		}

		uni_printf(enc->sx->io, "%" PRIitem " ", item);
	}

	uni_printf(enc->sx->io, "\n");
	return 0;
}

/**
 *	Export codes of virtual machine
 *
 *	@param	enc			Encoder
 *
 *	@return	@c 0 on success, @c -1 on error
 */
static int enc_export(const encoder *const enc)
{
	uni_printf(enc->sx->io, "#!/usr/bin/ruc-vm\n");

	uni_printf(enc->sx->io, "%zi %zi %zi %zi %zi %" PRIitem " 0\n"
		, vector_size(&enc->memory)
		, vector_size(&enc->sx->functions)
		, vector_size(&enc->identifiers)
		, vector_size(&enc->representations)
		, vector_size(&enc->sx->types)
		, enc->sx->max_displg);

	return print_table(enc, &enc->memory)
		|| print_table(enc, &enc->sx->functions)
		|| print_table(enc, &enc->identifiers)
		|| print_table(enc, &enc->representations)
		|| print_table(enc, &enc->sx->types);
}

/**
 *	Free allocated memory
 *
 *	@param	enc			Encoder
 */
static void enc_clear(encoder *const enc)
{
	vector_clear(&enc->memory);
	vector_clear(&enc->iniprocs);
	vector_clear(&enc->identifiers);
	vector_clear(&enc->representations);
}


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 * 	Emit load of lvalue
 *
 * 	@param	enc		Encoder
 */
static void emit_load_of_lvalue(encoder *const enc, lvalue value)
{
	switch (value.kind)
	{
		case VARIABLE:
		{
			if (type_is_structure(enc->sx, value.type))
			{
				mem_add(enc, IC_COPY0ST);
				mem_add(enc, value.displ);
				mem_add(enc, (item_t)type_size(enc->sx, value.type));
			}
			else
			{
				mem_add(enc, type_is_floating(value.type) ? IC_LOADD : IC_LOAD);
				mem_add(enc, value.displ);
			}

			return;
		}

		case ADDRESS:
		{
			if (type_is_structure(enc->sx, value.type))
			{
				mem_add(enc, IC_COPY1ST);
				mem_add(enc, (item_t)type_size(enc->sx, value.type));
			}
			else if (!type_is_array(enc->sx, value.type) && !type_is_pointer(enc->sx, value.type))
			{
				mem_add(enc, type_is_integer(value.type) ? IC_LAT : IC_LATD);
			}

			return;
		}

		default:
			return;
	}
}

/**
 *	Emit identifier expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Expression value
 */
static lvalue emit_identifier_expression(encoder *const enc, const node *const nd)
{
	const size_t identifier = expression_identifier_get_id(nd);
	const item_t type = ident_get_type(enc->sx, identifier);
	const item_t displ = ident_get_displ(enc->sx, identifier);

	return (lvalue){ .kind = VARIABLE, .type = type, .displ = displ };
}

/**
 *	Emit subscript expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return Expression value
 */
static lvalue emit_subscript_expression(encoder *const enc, const node *const nd)
{
	const node base = expression_subscript_get_base(nd);
	emit_expression(enc, &base);

	const node index = expression_subscript_get_index(nd);
	emit_expression(enc, &index);

	mem_add(enc, IC_SLICE);

	const item_t type = expression_get_type(nd);
	mem_add(enc, (item_t)type_size(enc->sx, type));
	if (type_is_array(enc->sx, type))
	{
		mem_add(enc, IC_LAT);
	}

	return (lvalue){ .kind = ADDRESS, .type = type };
}

/**
 *	Emit member expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Expression value
 */
static lvalue emit_member_expression(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);
	const bool is_arrow = expression_member_is_arrow(nd);
	const size_t member_index = expression_member_get_member_index(nd);
	const item_t struct_type = is_arrow ? type_pointer_get_element_type(enc->sx, base_type) : base_type;

	item_t member_displ = 0;
	for (size_t i = 0; i < member_index; i++)
	{
		member_displ += (item_t)type_size(enc->sx, type_structure_get_member_type(enc->sx, struct_type, i));
	}

	if (is_arrow)
	{
		emit_expression(enc, &base);
		mem_add(enc, IC_SELECT);
		mem_add(enc, member_displ);
		if (type_is_array(enc->sx, type) || type_is_pointer(enc->sx, type))
		{
			mem_add(enc, IC_LAT);
		}

		return (lvalue){ .kind = ADDRESS, .type = type };
	}
	else
	{
		const lvalue value = emit_lvalue(enc, &base);
		if (value.kind == VARIABLE)
		{
			return (lvalue){
					.kind = VARIABLE,
					.type = type,
					.displ = value.displ > 0 ? value.displ + member_displ : value.displ - member_displ
				};
		}
		else // if (enc->last_kind == ADDRESS)
		{
			mem_add(enc, IC_SELECT);
			mem_add(enc, member_displ);

			if (type_is_array(enc->sx, type) || type_is_pointer(enc->sx, type))
			{
				mem_add(enc, IC_LAT);
			}

			return (lvalue){ .kind = ADDRESS, .type = type };
		}
	}
}

/**
 *	Emit expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Expression value
 */
static lvalue emit_lvalue(encoder *const enc, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return emit_identifier_expression(enc, nd);

		case EXPR_SUBSCRIPT:
			return emit_subscript_expression(enc, nd);

		case EXPR_MEMBER:
			return emit_member_expression(enc, nd);

		case EXPR_UNARY:
		{
			// Тут может быть только UN_INDIRECTION
			const item_t type = expression_get_type(nd);
			const node operand = expression_unary_get_operand(nd);
			const lvalue value = emit_lvalue(enc, &operand);
			if (value.kind == VARIABLE)
			{
				mem_add(enc, IC_LOAD);
				mem_add(enc, value.displ);
			}

			return (lvalue){ .kind = ADDRESS, .type = type };
		}

		default:
			// Cannot be an lvalue
			system_error(node_unexpected, nd);
			return (lvalue){ .displ = ITEM_MAX };
	}
}

/**
 *	Emit literal expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_literal_expression(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);

	if (type_is_null_pointer(type))
	{
		mem_add(enc, IC_LI);
		mem_add(enc, 0);
	}
	else if (type_is_integer(type))
	{
		const int value = expression_literal_get_integer(nd);

		mem_add(enc, IC_LI);
		mem_add(enc, value);
	}
	else if (type_is_floating(type))
	{
		const double value = expression_literal_get_floating(nd);

		mem_add(enc, IC_LID);
		mem_add_double(enc, value);
	}
	else // if (type_is_string(enc->sx, type))
	{
		const size_t string_num = expression_literal_get_string(nd);
		const char *const string = string_get(enc->sx, string_num);

		mem_add(enc, IC_LI);
		const size_t reserved = mem_size(enc) + 4;
		mem_add(enc, (item_t)reserved);
		mem_add(enc, IC_B);
		mem_increase(enc, 2);


		item_t length = 0;
		for (size_t i = 0; string[i] != '\0'; i += utf8_symbol_size(string[i]))
		{
			mem_add(enc, utf8_convert(&string[i]));
			length++;
		}

		mem_set(enc, reserved - 1, length);
		mem_set(enc, reserved - 2, (item_t)mem_size(enc));
	}
}

/**
 *	Emit argument
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_argument(encoder *const enc, const node *const nd)
{
	const item_t arg_type = expression_get_type(nd);
	if (type_is_function(enc->sx, arg_type))
	{
		const item_t displ = ident_get_displ(enc->sx, expression_identifier_get_id(nd));
		mem_add(enc, displ < 0 ? IC_LOAD : IC_LI);
		mem_add(enc, llabs(displ));
	}
	else if (expression_get_class(nd) == EXPR_LIST)
	{
		const node fst = expression_list_get_subexpr(nd, 0);
		const item_t type = expression_get_type(&fst);
		const bool is_int = type_is_integer(type);

		mem_add(enc, IC_LI);
		const size_t reserved = mem_size(enc) + 4;
		mem_add(enc, (item_t)reserved);
		mem_add(enc, IC_B);
		mem_increase(enc, 2);

		const size_t size = expression_list_get_size(nd);
		for (size_t i = 0; i < size; i++)
		{
			const node subexpr = expression_list_get_subexpr(nd, i);
			if (expression_get_class(&subexpr) != EXPR_LITERAL)
			{
				system_error(wrong_init_in_actparam);
				enc->sx->was_error = true;
			}

			if (is_int)
			{
				mem_add(enc, expression_literal_get_integer(&subexpr));
			}
			else
			{
				const double value = expression_literal_get_floating(&subexpr);
				mem_add_double(enc, value);
			}
		}

		mem_set(enc, reserved - 1, (item_t)size);
		mem_set(enc, reserved - 2, (item_t)mem_size(enc));
	}
	else
	{
		emit_expression(enc, nd);
	}
}

/**
 *	Emit call expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_call_expression(encoder *const enc, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	const item_t func_type = expression_get_type(&callee);
	const size_t args = type_function_get_parameter_amount(enc->sx, func_type);

	const size_t func = expression_identifier_get_id(&callee);

	if (func >= BEGIN_USER_FUNC)
	{
		mem_add(enc, IC_CALL1);
	}

	for (size_t i = 0; i < args; i++)
	{
		const node argument = expression_call_get_argument(nd, i);
		emit_argument(enc, &argument);
	}

	if (func >= BEGIN_USER_FUNC)
	{
		mem_add(enc, IC_CALL2);
		mem_add(enc, ident_get_displ(enc->sx, func));
	}
	else
	{
		mem_add(enc, builtin_to_instruction((builtin_t)func));
	}
}

/**
 *	Emit rvalue of member expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_member_rvalue(encoder *const enc, const node *const nd)
{
	// Member expression может выдать rvalue только в одном случае: слева rvalue и оператор '.'
	const item_t type = expression_get_type(nd);
	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);
	const size_t member_index = expression_member_get_member_index(nd);

	item_t member_displ = 0;
	for (size_t i = 0; i < member_index; i++)
	{
		member_displ += (item_t)type_size(enc->sx, type_structure_get_member_type(enc->sx, base_type, i));
	}

	emit_expression(enc, &base);
	mem_add(enc, IC_COPYST);
	mem_add(enc, member_displ);
	mem_add(enc, type);
	mem_add(enc, (item_t)type_size(enc->sx, base_type));
}

/**
 *	Emit increment expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_increment_expression(encoder *const enc, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);

	const lvalue value = emit_lvalue(enc, &operand);

	const unary_t operator = expression_unary_get_operator(nd);
	instruction_t instruction = unary_to_instruction(operator);

	if (value.kind == ADDRESS)
	{
		instruction = instruction_to_address_ver(instruction);
	}

	if (type_is_floating(expression_get_type(nd)))
	{
		mem_add(enc, instruction_to_floating_ver(instruction));
	}
	else
	{
		mem_add(enc, instruction);
	}

	if (value.kind == VARIABLE)
	{
		mem_add(enc, value.displ);
	}
}

/**
 *	Emit cast expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_cast_expression(encoder *const enc, const node *const nd)
{
	const node subexpr = expression_cast_get_operand(nd);
	emit_expression(enc, &subexpr);

	mem_add(enc, IC_WIDEN);
}

/**
 *	Emit unary expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_unary_expression(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const unary_t operator = expression_unary_get_operator(nd);
	const node operand = expression_unary_get_operand(nd);
	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			emit_increment_expression(enc, nd);
			return;

		case UN_ADDRESS:
		{
			const lvalue value = emit_lvalue(enc, &operand);
			if (value.kind == VARIABLE)
			{
				mem_add(enc, IC_LA);
				mem_add(enc, value.displ);
			}

			return;
		}

		case UN_INDIRECTION:
			return;

		case UN_PLUS:
			emit_expression(enc, &operand);
			return;

		case UN_MINUS:
			emit_expression(enc, &operand);
			mem_add(enc, type_is_integer(type) ? IC_UNMINUS : IC_UNMINUS_R);
			return;

		case UN_NOT:
			emit_expression(enc, &operand);
			mem_add(enc, IC_NOT);
			return;

		case UN_LOGNOT:
			emit_expression(enc, &operand);
			mem_add(enc, IC_LOG_NOT);
			return;

		case UN_ABS:
			emit_expression(enc, &operand);
			mem_add(enc, type_is_integer(type) ? IC_ABSI : IC_ABS);
			return;
	}
}

/**
 *	Emit integral expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_integral_expression(encoder *const enc, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);

	const node LHS = expression_binary_get_LHS(nd);
	emit_expression(enc, &LHS);

	size_t addr = SIZE_MAX;
	const bool is_logical = operator == BIN_LOG_AND || operator == BIN_LOG_OR;
	if (is_logical)
	{
		mem_add(enc, IC_DUPLICATE);
		mem_add(enc, operator == BIN_LOG_AND ? IC_BE0 : IC_BNE0);
		addr = mem_reserve(enc);
	}

	const node RHS = expression_binary_get_RHS(nd);
	emit_expression(enc, &RHS);

	if (is_logical)
	{
		mem_set(enc, addr, (item_t)mem_size(enc) + 1);
	}

	const instruction_t instruction = binary_to_instruction(operator);
	if (type_is_floating(expression_get_type(&LHS)) || type_is_floating(expression_get_type(&RHS)))
	{
		mem_add(enc, instruction_to_floating_ver(instruction));
	}
	else
	{
		mem_add(enc, instruction);
	}
}

/**
 *	Emit assignment expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_assignment_expression(encoder *const enc, const node *const nd)
{
	const node LHS = expression_binary_get_LHS(nd);
	const lvalue left_value = emit_lvalue(enc, &LHS);

	const node RHS = expression_binary_get_RHS(nd);

	const item_t type = expression_get_type(nd);

	if (type_is_structure(enc->sx, type))
	{
		if (expression_is_lvalue(&RHS))
		{
			const lvalue right_value = emit_lvalue(enc, &RHS);

			instruction_t operator = left_value.kind == VARIABLE
				? right_value.kind == VARIABLE
					? IC_COPY00 : IC_COPY01
				: right_value.kind == VARIABLE
					? IC_COPY10 : IC_COPY11;

			mem_add(enc, operator);
			if (left_value.kind == VARIABLE)
			{
				mem_add(enc, left_value.displ);
			}

			if (right_value.kind == VARIABLE)
			{
				mem_add(enc, right_value.displ);
			}

			mem_add(enc, (item_t)type_size(enc->sx, type));
		}
		else
		{
			emit_expression(enc, &RHS);
			instruction_t operator = left_value.kind == VARIABLE ? IC_COPY0ST_ASSIGN : IC_COPY1ST_ASSIGN;
			mem_add(enc, operator);
			if (left_value.kind == VARIABLE)
			{
				mem_add(enc, left_value.displ);
			}

			mem_add(enc, (item_t)type_size(enc->sx, type));
		}
	}
	else // оба операнда базового типа или указатели
	{
		emit_expression(enc, &RHS);

		instruction_t operator = binary_to_instruction(expression_binary_get_operator(nd));
		if (left_value.kind == ADDRESS)
		{
			operator = instruction_to_address_ver(operator);
		}

		if (type_is_floating(type))
		{
			operator = instruction_to_floating_ver(operator);
		}

		mem_add(enc, operator);

		if (left_value.kind == VARIABLE)
		{
			mem_add(enc, left_value.displ);
		}
	}
}

/**
 *	Emit binary expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_binary_expression(encoder *const enc, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);

	if (operation_is_assignment(operator))
	{
		emit_assignment_expression(enc, nd);
	}
	else if (operator == BIN_COMMA)
	{
		const node LHS = expression_binary_get_LHS(nd);
		emit_void_expression(enc, &LHS);

		const node RHS = expression_binary_get_RHS(nd);
		emit_expression(enc, &RHS);
	}
	else
	{
		emit_integral_expression(enc, nd);
	}
}

/**
 *	Emit ternary expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_ternary_expression(encoder *const enc, const node *const nd)
{
	const node condition = expression_ternary_get_condition(nd);
	emit_expression(enc, &condition);

	mem_add(enc, IC_BE0);
	size_t addr = mem_reserve(enc);

	const node LHS = expression_ternary_get_LHS(nd);
	emit_expression(enc, &LHS);

	mem_set(enc, addr, (item_t)mem_size(enc) + 2);
	mem_add(enc, IC_B);
	addr = mem_reserve(enc);

	const node RHS = expression_ternary_get_RHS(nd);
	emit_expression(enc, &RHS);

	mem_set(enc, addr, (item_t)mem_size(enc));
}

/**
 *	Emit expression list
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_expression_list(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const size_t size = expression_list_get_size(nd);

	if (type_is_array(enc->sx, type))
	{
		mem_add(enc, IC_BEG_INIT);
		mem_add(enc, (item_t)size);
	}

	for (size_t i = 0; i < size; i++)
	{
		const node subexpr = expression_list_get_subexpr(nd, i);
		emit_expression(enc, &subexpr);
	}
}

/**
 *	Emit expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_expression(encoder *const enc, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		const lvalue value = emit_lvalue(enc, nd);
		emit_load_of_lvalue(enc, value);
		return;
	}

	switch (expression_get_class(nd))
	{
		case EXPR_LITERAL:
			emit_literal_expression(enc, nd);
			return;

		case EXPR_CALL:
			emit_call_expression(enc, nd);
			return;

		case EXPR_MEMBER:
			emit_member_rvalue(enc, nd);
			return;

		case EXPR_CAST:
			emit_cast_expression(enc, nd);
			return;

		case EXPR_UNARY:
			emit_unary_expression(enc, nd);
			return;

		case EXPR_BINARY:
			emit_binary_expression(enc, nd);
			return;

		case EXPR_TERNARY:
			emit_ternary_expression(enc, nd);
			return;

		case EXPR_LIST:
			emit_expression_list(enc, nd);
			return;

		default:
			// cannot be rvalue
			return;
	}
}

/**
 *	Emit expression which will be evaluated as a void expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_void_expression(encoder *const enc, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		emit_lvalue(enc, nd);
	}
	else
	{
		emit_expression(enc, nd);

		const size_t index = mem_get(enc, mem_size(enc) - 1) < 9000 ? mem_size(enc) - 2 : mem_size(enc) - 1;
		const instruction_t operation = (instruction_t)mem_get(enc, index);
		mem_set(enc, index, instruction_to_void_ver(operation));
	}
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */

/**
 *	Check that there are only strings in array initializer
 *
 *	@param	enc			Encoder
 *	@param	nd			Initializer
 *
 *	@return @c 1 on true, @c 0 on false
 */
static bool only_strings(const encoder *const enc, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_LITERAL:
			return type_is_string(enc->sx, expression_get_type(nd));

		case EXPR_LIST:
		{
			const size_t size = expression_list_get_size(nd);
			for (size_t i = 0; i < size; i++)
			{
				const node subexpr = expression_list_get_subexpr(nd, i);
				if (!only_strings(enc, &subexpr))
				{
					return false;
				}
			}

			return true;
		}

		default:
			return false;
	}
}

/**
 *	Emit array declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_array_declaration(encoder *const enc, const node *const nd)
{
	const size_t ident = declaration_variable_get_id(nd);
	const item_t displ = ident_get_displ(enc->sx, ident);

	const size_t bounds = declaration_variable_get_dim_amount(nd);
	for (size_t i = 0; i < bounds; i++)
	{
		const node bound = declaration_variable_get_dim_expr(nd, i);
		emit_expression(enc, &bound);
	}

	item_t type = ident_get_type(enc->sx, ident);
	size_t dimensions = 0;
	while (type_is_array(enc->sx, type))
	{
		type = type_array_get_element_type(enc->sx, type);
		dimensions++;
	}

	const item_t length = (item_t)type_size(enc->sx, type);
	const bool has_initializer = declaration_variable_has_initializer(nd);
	const item_t iniproc = proc_get(enc, (size_t)type);

	mem_add(enc, IC_DEFARR); 		// DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
	mem_add(enc, (item_t)dimensions - (has_initializer ? 1 : 0));
	mem_add(enc, length);
	mem_add(enc, displ);
	mem_add(enc, iniproc && iniproc != ITEM_MAX ? iniproc : 0);

	const size_t usual_addr = mem_size(enc);
	int usual = (dimensions == bounds ? 1 : 0);
	mem_add(enc, usual);

	mem_add(enc, has_initializer);
	mem_add(enc, 0);				// is in structure

	if (has_initializer)
	{
		const node initializer = declaration_variable_get_initializer(nd);
		emit_expression(enc, &initializer);

		if (only_strings(enc, &initializer))
		{
			usual += 2;
			mem_set(enc, usual_addr, usual);
		}

		mem_add(enc, IC_ARR_INIT);
		mem_add(enc, (item_t)dimensions);
		mem_add(enc, length);
		mem_add(enc, displ);
		mem_add(enc, usual);
	}
}

/**
 *	Emit variable declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_variable_declaration(encoder *const enc, const node *const nd)
{
	const size_t identifier = declaration_variable_get_id(nd);
	const item_t type = ident_get_type(enc->sx, identifier);
	const item_t displ = ident_get_displ(enc->sx, identifier);

	if (type_is_array(enc->sx, type))
	{
		emit_array_declaration(enc, nd);
		return;
	}

	const item_t iniproc = proc_get(enc, (size_t)type);
	if (iniproc != ITEM_MAX && iniproc != 0)
	{
		mem_add(enc, IC_STRUCT_WITH_ARR);
		mem_add(enc, displ);
		mem_add(enc, iniproc);
	}

	if (declaration_variable_has_initializer(nd))
	{
		const node initializer = declaration_variable_get_initializer(nd);
		emit_expression(enc, &initializer);

		if (type_is_structure(enc->sx, type))
		{
			mem_add(enc, IC_COPY0ST_ASSIGN);
			mem_add(enc, displ);
			mem_add(enc, (item_t)type_size(enc->sx, type));
		}
		else
		{
			mem_add(enc, type_is_floating(type) ? IC_ASSIGN_R_V : IC_ASSIGN_V);
			mem_add(enc, displ);
		}
	}
}

/**
 *	Emit type declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_type_declaration(encoder *const enc, const node *const nd)
{
	const size_t size = node_get_amount(nd);
	if (size == 0)
	{
		return;
	}

	mem_add(enc, IC_B);
	const size_t addr = mem_reserve(enc);

	const item_t type = node_get_arg(nd, 0);
	proc_set(enc, (size_t)type, (item_t)addr + 1);

	for (size_t i = 0; i < size; i++)
	{
		const node member = node_get_child(nd, i);

		item_t member_type = node_get_arg(&member, 0);
		size_t dimensions = 0;
		while (type_is_array(enc->sx, member_type))
		{
			member_type = type_array_get_element_type(enc->sx, member_type);
			dimensions++;
		}

		for (size_t j = 0; j < dimensions; j++)
		{
			const node bound = node_get_child(&member, j);
			emit_expression(enc, &bound);
		}

		const item_t length = (item_t)type_size(enc->sx, member_type);
		size_t displ = 0;
		for (size_t j = 0; j < (size_t)node_get_arg(&member, 1); j++)
		{
			displ += type_size(enc->sx, type_structure_get_member_type(enc->sx, type, j));
		}

		const item_t iniproc = proc_get(enc, (size_t)member_type);

		mem_add(enc, IC_DEFARR); 		// DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
		mem_add(enc, (item_t)dimensions);
		mem_add(enc, length);
		mem_add(enc, (item_t)displ);
		mem_add(enc, iniproc && iniproc != ITEM_MAX ? iniproc : 0);
		mem_add(enc, 1);				// usual
		mem_add(enc, 0);				// has initializer
		mem_add(enc, 1);				// is in structure
	}

	mem_add(enc, IC_STOP);
	mem_set(enc, addr, (item_t)mem_size(enc));
}

static void emit_function_definition(encoder *const enc, const node *const nd)
{
	const size_t function_id = (size_t)node_get_arg(nd, 0);
	const size_t max_displacement = (size_t)node_get_arg(nd, 1);
	const node function_body = declaration_function_get_body(nd);

	const size_t ref_func = (size_t)ident_get_displ(enc->sx, function_id);
	func_set(enc->sx, ref_func, (item_t)mem_size(enc));

	mem_add(enc, IC_FUNC_BEG);
	mem_add(enc, (item_t)max_displacement);

	const size_t old_pc = mem_reserve(enc);

	emit_statement(enc, &function_body);
	mem_add(enc, IC_RETURN_VOID);

	mem_set(enc, old_pc, (item_t)mem_size(enc));
}

/**
 *	Emit declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_declaration(encoder *const enc, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			emit_variable_declaration(enc, nd);
			return;

		case DECL_TYPE:
			emit_type_declaration(enc, nd);
			return;

		case DECL_FUNC:
			emit_function_definition(enc, nd);
			return;
	}
}


/*
 *	 ______     ______   ______     ______   ______     __    __     ______     __   __     ______   ______
 *	/\  ___\   /\__  _\ /\  __ \   /\__  _\ /\  ___\   /\ "-./  \   /\  ___\   /\ "-.\ \   /\__  _\ /\  ___\
 *	\ \___  \  \/_/\ \/ \ \  __ \  \/_/\ \/ \ \  __\   \ \ \-./\ \  \ \  __\   \ \ \-.  \  \/_/\ \/ \ \___  \
 *	 \/\_____\    \ \_\  \ \_\ \_\    \ \_\  \ \_____\  \ \_\ \ \_\  \ \_____\  \ \_\\"\_\    \ \_\  \/\_____\
 *	  \/_____/     \/_/   \/_/\/_/     \/_/   \/_____/   \/_/  \/_/   \/_____/   \/_/ \/_/     \/_/   \/_____/
 */


/**
 *	Emit labeled statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_labeled_statement(encoder *const enc, const node *const nd)
{
	const size_t label_id = statement_labeled_get_label(nd);
	item_t addr = ident_get_displ(enc->sx, label_id);

	if (addr < 0)
	{
		// Были переходы на метку
		while (addr != 0)
		{
			// Проставить ссылку на метку во всех ранних переходах
			const item_t ref = mem_get(enc, (size_t)(-addr));
			mem_set(enc, (size_t)(-addr), (item_t)mem_size(enc));
			addr = ref;
		}
	}

	ident_set_displ(enc->sx, label_id, (item_t)mem_size(enc));

	const node substatement = statement_labeled_get_substmt(nd);
	emit_statement(enc, &substatement);
}

/**
 *	Emit case statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_case_statement(encoder *const enc, const node *const nd)
{
	if (enc->addr_case != 0)
	{
		mem_set(enc, enc->addr_case, (item_t)mem_size(enc));
	}

	mem_add(enc, IC_DUPLICATE);

	const node expression = statement_case_get_expression(nd);
	emit_expression(enc, &expression);

	mem_add(enc, IC_EQ);
	mem_add(enc, IC_BE0);
	enc->addr_case = mem_reserve(enc);

	const node substmt = statement_case_get_substmt(nd);
	emit_statement(enc, &substmt);
}

/**
 *	Emit default statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_default_statement(encoder *const enc, const node *const nd)
{
	if (enc->addr_case != 0)
	{
		mem_set(enc, enc->addr_case, (item_t)mem_size(enc));
	}

	enc->addr_case = 0;

	const node substmt = statement_default_get_substmt(nd);
	emit_statement(enc, &substmt);
}

/**
 *	Emit compound statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_compound_statement(encoder *const enc, const node *const nd)
{
	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}
}

/**
 *	Emit if statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_if_statement(encoder *const enc, const node *const nd)
{
	const node condition = statement_if_get_condition(nd);
	emit_expression(enc, &condition);

	mem_add(enc, IC_BE0);
	size_t addr = mem_reserve(enc);

	const node then_substmt = statement_if_get_then_substmt(nd);
	emit_statement(enc, &then_substmt);

	if (statement_if_has_else_substmt(nd))
	{
		mem_set(enc, addr, (item_t)mem_size(enc) + 2);
		mem_add(enc, IC_B);
		addr = mem_reserve(enc);

		const node else_substmt = statement_if_get_else_substmt(nd);
		emit_statement(enc, &else_substmt);
	}

	mem_set(enc, addr, (item_t)mem_size(enc));
}

/**
 *	Emit switch statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_switch_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_case = enc->addr_case;
	enc->addr_break = 0;
	enc->addr_case = 0;

	const node condition = statement_switch_get_condition(nd);
	emit_expression(enc, &condition);

	const node body = statement_switch_get_body(nd);
	emit_statement(enc, &body);

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
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_while_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	const size_t addr = mem_size(enc);
	enc->addr_cond = addr;

	const node condition = statement_while_get_condition(nd);
	emit_expression(enc, &condition);

	mem_add(enc, IC_BE0);
	enc->addr_break = mem_size(enc);
	mem_add(enc, 0);

	const node body = statement_while_get_body(nd);
	emit_statement(enc, &body);

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
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_do_statement(encoder *const enc, const node *const nd)
{
	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	const item_t addr = (item_t)mem_size(enc);

	enc->addr_cond = 0;
	enc->addr_break = 0;

	const node body = statement_do_get_body(nd);
	emit_statement(enc, &body);
	addr_end_condition(enc);

	const node condition = statement_do_get_condition(nd);
	emit_expression(enc, &condition);

	mem_add(enc, IC_BNE0);
	mem_add(enc, addr);
	addr_end_break(enc);

	enc->addr_break = old_addr_break;
	enc->addr_cond = old_addr_cond;
}

/**
 *	Emit for statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_for_statement(encoder *const enc, const node *const nd)
{
	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		emit_statement(enc, &inition);
	}

	const size_t old_addr_break = enc->addr_break;
	const size_t old_addr_cond = enc->addr_cond;
	enc->addr_cond = 0;
	enc->addr_break = 0;

	const size_t addr_init = mem_size(enc);
	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		emit_expression(enc, &condition);

		mem_add(enc, IC_BE0);
		enc->addr_break = mem_size(enc);
		mem_add(enc, 0);
	}

	const node body = statement_for_get_body(nd);
	emit_statement(enc, &body);
	addr_end_condition(enc);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		emit_void_expression(enc, &increment);
	}

	mem_add(enc, IC_B);
	mem_add(enc, (item_t)addr_init);
	addr_end_break(enc);

	enc->addr_break = old_addr_break;
	enc->addr_cond = old_addr_cond;
}

/**
 *	Emit goto statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_goto_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_B);

	const size_t id = statement_goto_get_label(nd);
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

		// Ставим адрес предыдущего перехода
		mem_add(enc, addr);
	}
}

/**
 *	Emit continue statement
 *
 *	@param	enc			Encoder
 */
static void emit_continue_statement(encoder *const enc)
{
	mem_add(enc, IC_B);
	mem_add(enc, (item_t)enc->addr_cond);
	enc->addr_cond = mem_size(enc) - 1;
}

/**
 *	Emit break statement
 *
 *	@param	enc			Encoder
 */
static void emit_break_statement(encoder *const enc)
{
	mem_add(enc, IC_B);
	mem_add(enc, (item_t)enc->addr_break);
	enc->addr_break = mem_size(enc) - 1;
}

/**
 *	Emit return statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_return_statement(encoder *const enc, const node *const nd)
{
	if (statement_return_has_expression(nd))
	{
		const node expr = statement_return_get_expression(nd);
		emit_expression(enc, &expr);

		mem_add(enc, IC_RETURN_VAL);
		mem_add(enc, (item_t)type_size(enc->sx, expression_get_type(&expr)));
	}
	else
	{
		mem_add(enc, IC_RETURN_VOID);
	}
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
 *	@param	enc			Encoder
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
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_getid_statement(encoder *const enc, const node *const nd)
{
	mem_add(enc, IC_GETID);
	compress_ident(enc, (size_t)node_get_arg(nd, 0)); // Cсылка в identtab
}

/**
 *	Emit printf statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_printf_statement(encoder *const enc, const node *const nd)
{
	size_t sum_size = 0;
	const size_t argc = statement_printf_get_argc(nd);
	for (size_t i = 0; i < argc; i++)
	{
		const node arg = statement_printf_get_argument(nd, i);
		emit_expression(enc, &arg);

		sum_size += type_size(enc->sx, expression_get_type(&arg));
	}

	const node format_string = statement_printf_get_format_str(nd);
	emit_expression(enc, &format_string);

	mem_add(enc, IC_PRINTF);
	mem_add(enc, (item_t)sum_size);
}

/**
 *	Emit print statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_print_statement(encoder *const enc, const node *const nd)
{
	const node subexpr = node_get_child(nd, 0);
	emit_expression(enc, &subexpr);

	mem_add(enc, IC_PRINT);
	mem_add(enc, expression_get_type(&subexpr));
}

/**
 *	Emit statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_statement(encoder *const enc, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			emit_declaration(enc, nd);
			return;

		case STMT_LABEL:
			emit_labeled_statement(enc, nd);
			return;

		case STMT_CASE:
			emit_case_statement(enc, nd);
			return;

		case STMT_DEFAULT:
			emit_default_statement(enc, nd);
			return;

		case STMT_COMPOUND:
			emit_compound_statement(enc, nd);
			return;

		case STMT_EXPR:
			emit_void_expression(enc, nd);
			return;

		case STMT_NULL:
			return;

		case STMT_IF:
			emit_if_statement(enc, nd);
			return;

		case STMT_SWITCH:
			emit_switch_statement(enc, nd);
			return;

		case STMT_WHILE:
			emit_while_statement(enc, nd);
			return;

		case STMT_DO:
			emit_do_statement(enc, nd);
			return;

		case STMT_FOR:
			emit_for_statement(enc, nd);
			return;

		case STMT_GOTO:
			emit_goto_statement(enc, nd);
			return;

		case STMT_CONTINUE:
			emit_continue_statement(enc);
			return;

		case STMT_BREAK:
			emit_break_statement(enc);
			return;

		case STMT_RETURN:
			emit_return_statement(enc, nd);
			return;

		case STMT_PRINTF:
			emit_printf_statement(enc, nd);
			return;
		case STMT_PRINT:
			emit_print_statement(enc, nd);
			return;
		case STMT_PRINTID:
			emit_printid_statement(enc, nd);
			return;
		case STMT_GETID:
			emit_getid_statement(enc, nd);
			return;
	}
}

/**
 *	Emit translation unit
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_translation_unit(encoder *const enc, const node *const nd)
{
	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = translation_unit_get_declaration(nd, i);
		emit_declaration(enc, &decl);
	}

	mem_add(enc, IC_CALL1);
	mem_add(enc, IC_CALL2);
	mem_add(enc, ident_get_displ(enc->sx, enc->sx->ref_main));
	mem_add(enc, IC_STOP);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_vm(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	encoder enc = enc_create(ws, sx);

	const node root = node_get_root(&sx->tree);
	emit_translation_unit(&enc, &root);

#ifndef NDEBUG
	write_codes(DEFAULT_CODES, &enc.memory);
#endif

	int ret = enc.sx->was_error || enc_export(&enc);

	enc_clear(&enc);
	return ret;
}
