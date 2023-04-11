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
#include "AST.h"
#include "errors.h"
#include "instructions.h"
#include "item.h"
#include "string.h"
#include "tree.h"
#include "uniprinter.h"
#include "utf8.h"
#include "writer.h"


#define DISPL_START 3

#ifndef abs
	#define abs(a) ((a) > 0 ? (a) : -(a))
#endif


#ifndef max
	#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


static const char *const DEFAULT_CODES = "codes.txt";
static const size_t MAX_MEM_SIZE = 100000;


/** Kinds of lvalue */
typedef enum OPERAND
{
	VARIABLE,		/**< Variable operand */
	ADDRESS,		/**< Address operand */
} operand_t;


/** Allocated value designator */
typedef struct lvalue
{
	const item_t type;				/**< Value type */
	const operand_t kind;			/**< Value kind */
	const item_t displ;				/**< Value displacement */
} lvalue;

/** RuC-VM Intermediate Representation encoder */
typedef struct encoder
{
	syntax *const sx;				/**< Syntax structure */

	vector memory;					/**< Memory table */
	vector iniprocs;				/**< Init procedures */

	vector identifiers;				/**< Local identifiers table */
	vector representations;			/**< Local representations table */
	vector displacements;			/**< Displacements table */
	vector functions;				/**< Functions table */

	size_t addr_cond;				/**< Condition address */
	size_t addr_case;				/**< Case operator address */
	size_t addr_break;				/**< Break operator address */

	item_t displ;					/**< Current stack displacement */

	item_t max_local_displ;			/**< Maximal local displacement */
	item_t max_global_displ;		/**< Maximal global displacement */

	const node *curr_func;			/**< Currently emitted function */
	const item_status target;		/**< Target tables item type */
} encoder;


static void emit_void_expression(encoder *const enc, const node *const nd);
static lvalue emit_lvalue(encoder *const enc, const node *const nd);
static void emit_expression(encoder *const enc, const node *const nd);
static void emit_statement(encoder *const enc, const node *const nd);
static void emit_declaration(encoder *const enc, const node *const nd);


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

static inline size_t mem_size(const encoder *const enc)
{
	return vector_size(&enc->memory);
}

static inline size_t mem_add(encoder *const enc, const item_t value)
{
	return vector_add(&enc->memory, value);
}

static inline void mem_add_double(encoder *const enc, const double value)
{
	item_t buffer[8];
	const size_t size = item_store_double_for_target(enc->target, value, buffer);

	for (size_t i = 0; i < size; i++)
	{
		mem_add(enc, buffer[i]);
	}
}

static inline int mem_set(encoder *const enc, const size_t index, const item_t value)
{
	return vector_set(&enc->memory, index, value);
}

static inline item_t mem_get(const encoder *const enc, const size_t index)
{
	return vector_get(&enc->memory, index);
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


/**
 *	Allocate variable
 *
 *	@param	enc			Encoder
 *	@param	identifier	Variable identifier
 *
 *	@return	Allocated variable displacement
 */
static inline item_t displacements_add(encoder *const enc, const size_t identifier)
{
	const item_t type = ident_get_type(enc->sx, identifier);
	const item_t size = (item_t)type_size(enc->sx, type);
	item_t result_displ = enc->displ;

	if (enc->curr_func)
	{
		if (type_is_function(enc->sx, type))
		{
			result_displ *= -1;
		}

		enc->displ += size;
		enc->max_local_displ = max(enc->displ, enc->max_local_displ);
	}
	else
	{
		result_displ = -enc->max_global_displ;
		enc->max_global_displ += size;
	}

	vector_set(&enc->displacements, identifier, result_displ);
	return result_displ;
}

/**
 *	Get variable displacement
 *
 *	@param	enc			Encoder
 *	@param	identifier	Variable identifier
 *
 *	@return	Variable displacement
 */
static inline item_t displacements_get(const encoder *const enc, const size_t identifier)
{
	return vector_get(&enc->displacements, identifier);
}


/**
 *	Add function to a functions table
 *
 *	@param	enc			Encoder
 *	@param	identifier	Function identifier
 *	@param	address		Function address
 */
static inline void functions_add(encoder *const enc, const size_t identifier, const size_t address)
{
	const size_t func_number = vector_add(&enc->functions, (item_t)address);
	vector_set(&enc->displacements, identifier, (item_t)func_number);

	// If the function is defined after its calls, the callee is a predecl id
	const size_t predecl_identifier = ident_get_prev(enc->sx, identifier);
	item_t predecl_displ = displacements_get(enc, predecl_identifier);

	if (predecl_displ < 0)
	{
		size_t call_address = (size_t)abs(predecl_displ);
		while (call_address != 0)
		{
			const size_t ref = (size_t)mem_get(enc, call_address);
			mem_set(enc, call_address, (item_t)func_number);
			call_address = ref;
		}
	}
}

/**
 *	Get function displacement
 *
 *	@param	enc			Encoder
 *	@param	identifier	Function identifier
 *
 *	@return	Funciton displacement
 */
static inline item_t functions_get(encoder *const enc, const size_t identifier)
{
	const item_t displ = vector_get(&enc->displacements, identifier);

	if (enc->curr_func)
	{
		const size_t parameters_amount = declaration_function_get_parameters_amount(enc->curr_func);
		for (size_t i = 0; i < parameters_amount; i++)
		{
			const size_t parameter = declaration_function_get_parameter(enc->curr_func, i);
			if (identifier == parameter)
			{
				return displ;
			}
		}
	}

	if (displ <= 0)
	{
		vector_set(&enc->displacements, identifier, -(item_t)mem_size(enc));
	}

	return abs(displ);
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
	encoder enc = { .sx = sx, .target = item_get_status(ws) };

	enc.memory = vector_create(MAX_MEM_SIZE);
	enc.iniprocs = vector_create(0);

	const size_t records = vector_size(&sx->identifiers) / 4;
	enc.identifiers = vector_create(records * 3);
	enc.representations = vector_create(records * 8);
	enc.displacements = vector_create(records);
	enc.functions = vector_create(records);

	vector_increase(&enc.memory, 4);
	vector_increase(&enc.iniprocs, vector_size(&enc.sx->types));
	vector_increase(&enc.displacements, vector_size(&sx->identifiers));
	vector_increase(&enc.functions, 2);

	enc.max_global_displ = 3;
	enc.curr_func = NULL;

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
		, vector_size(&enc->functions)
		, vector_size(&enc->identifiers)
		, vector_size(&enc->representations)
		, vector_size(&enc->sx->types)
		, enc->max_global_displ);

	return print_table(enc, &enc->memory)
		|| print_table(enc, &enc->functions)
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
	vector_clear(&enc->displacements);
	vector_clear(&enc->functions);
}

/**
 *	Emit an error from encoder
 *
 *	@param	enc			Encoder
 *	@param	num			Error code
 */
static void encoder_error(encoder *const enc, const location loc, err_t num, ...)
{
	va_list args;
	va_start(args, num);

	report_error(&enc->sx->rprt, enc->sx->io, loc, num, args);

	va_end(args);
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
				mem_add(enc, type_is_floating(enc->sx, value.type) ? IC_LOADD : IC_LOAD);
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
				mem_add(enc, type_is_integer(enc->sx, value.type) ? IC_LAT : IC_LATD);
			}

			return;
		}
	}
}

/**
 *	Emit identifier lvalue
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Identifier designator
 */
static lvalue emit_identifier_lvalue(encoder *const enc, const node *const nd)
{
	const size_t identifier = expression_identifier_get_id(nd);
	const item_t type = ident_get_type(enc->sx, identifier);
	const item_t unqualified_type = type_is_const(enc->sx, type) ? type_const_get_unqualified_type(enc->sx, type) : type;
	const item_t displ = displacements_get(enc, identifier);

	return (lvalue){ .kind = VARIABLE, .type = unqualified_type, .displ = displ };
}

/**
 *	Emit subscript lvalue
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return Subscript designator
 */
static lvalue emit_subscript_lvalue(encoder *const enc, const node *const nd)
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
 *	Emit member lvalue
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Member designator
 */
static lvalue emit_member_lvalue(encoder *const enc, const node *const nd)
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
		const item_t member_type = type_structure_get_member_type(enc->sx, struct_type, i);
		member_displ += (item_t)type_size(enc->sx, member_type);
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
			const item_t displ = value.displ > 0 ? value.displ + member_displ : value.displ - member_displ;
			return (lvalue){ .kind = VARIABLE, .type = type, .displ = displ };
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
 *	Emit indirection lvalue
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Indirection designator
 */
static lvalue emit_indirection_lvalue(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const item_t unqualified_type = type_is_const(enc->sx, type) ? type_const_get_unqualified_type(enc->sx, type) : type;
	const node operand = expression_unary_get_operand(nd);
	const lvalue value = emit_lvalue(enc, &operand);
	if (value.kind == VARIABLE)
	{
		mem_add(enc, IC_LOAD);
		mem_add(enc, value.displ);
	}

	return (lvalue){ .kind = ADDRESS, .type = unqualified_type };
}

/**
 *	Emit lvalue
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *
 *	@return	Lvalue designator
 */
static lvalue emit_lvalue(encoder *const enc, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return emit_identifier_lvalue(enc, nd);

		case EXPR_SUBSCRIPT:
			return emit_subscript_lvalue(enc, nd);

		case EXPR_MEMBER:
			return emit_member_lvalue(enc, nd);

		case EXPR_UNARY:
			// There can be only indirection
			return emit_indirection_lvalue(enc, nd);

		default:
			// Cannot be lvalue
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
	const item_t unqualified_type = type_is_const(enc->sx, type) ? type_const_get_unqualified_type(enc->sx, type) : type;
	switch (type_get_class(enc->sx, unqualified_type))
	{
		case TYPE_NULL_POINTER:
		{
			mem_add(enc, IC_LI);
			mem_add(enc, 0);
			return;
		}

		case TYPE_BOOLEAN:
		{
			const bool value = expression_literal_get_boolean(nd);

			mem_add(enc, IC_LI);
			mem_add(enc, value ? 1 : 0);
			return;
		}

		case TYPE_CHARACTER:
		{
			const char32_t value = expression_literal_get_character(nd);

			mem_add(enc, IC_LI);
			mem_add(enc, value);
			return;
		}

		case TYPE_INTEGER:
		case TYPE_ENUM:
		{
			const item_t value = expression_literal_get_integer(nd);

			mem_add(enc, IC_LI);
			mem_add(enc, value);
			return;
		}

		case TYPE_FLOATING:
		{
			const double value = expression_literal_get_floating(nd);

			mem_add(enc, IC_LID);
			mem_add_double(enc, value);
			return;
		}

		case TYPE_ARRAY:
		{
			// Это может быть только строка
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
			return;
		}

		default:
			// Таких литералов не бывает
			return;
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
		const size_t identifier = expression_identifier_get_id(nd);
		const item_t displ = displacements_get(enc, identifier);
		mem_add(enc, displ >= 0 ? IC_LI : IC_LOAD);
		mem_add(enc, abs(displ));
	}
	else if (expression_get_class(nd) == EXPR_INITIALIZER)
	{
		mem_add(enc, IC_LI);
		const size_t reserved = mem_size(enc) + 4;
		mem_add(enc, (item_t)reserved);
		mem_add(enc, IC_B);
		mem_increase(enc, 2);

		const node fst = expression_initializer_get_subexpr(nd, 0);
		const size_t size = expression_initializer_get_size(nd);
		for (size_t i = 0; i < size; i++)
		{
			const node subexpr = expression_initializer_get_subexpr(nd, i);
			if (expression_get_class(&subexpr) != EXPR_LITERAL)
			{
				encoder_error(enc, node_get_location(&subexpr), wrong_init_in_actparam);
			}

			if (type_is_integer(enc->sx, expression_get_type(&fst)))
			{
				mem_add(enc, expression_literal_get_integer(&subexpr));
			}
			else
			{
				mem_add_double(enc, expression_literal_get_floating(&subexpr));
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

static void compress_ident(encoder *const enc, const size_t ref)
{
	if (vector_get(&enc->sx->identifiers, ref) == ITEM_MAX)
	{
		mem_add(enc, ident_get_repr(enc->sx, ref));
		return;
	}

	const item_t new_ref = (item_t)vector_size(&enc->identifiers) - 1;
	const item_t type = ident_get_type(enc->sx, ref);
	const item_t unqualified_type = type_is_const(enc->sx, type) ? type_const_get_unqualified_type(enc->sx, type) : type;
	vector_add(&enc->identifiers, (item_t)vector_size(&enc->representations) - 2);
	vector_add(&enc->identifiers, unqualified_type);
	vector_add(&enc->identifiers, displacements_get(enc, ref));

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
 *	Emit printid expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_printid_expression(encoder *const enc, const node *const nd)
{
	const size_t argc = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < argc; i++)
	{
		mem_add(enc, IC_PRINTID);

		const node arg = expression_call_get_argument(nd, i);
		compress_ident(enc, expression_identifier_get_id(&arg)); // Ссылка в identtab
	}
}

/**
 *	Emit getid expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_getid_expression(encoder *const enc, const node *const nd)
{
	const size_t argc = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < argc; i++)
	{
		mem_add(enc, IC_GETID);

		const node arg = expression_call_get_argument(nd, i);
		compress_ident(enc, expression_identifier_get_id(&arg)); // Ссылка в identtab
	}
}

/**
 *	Emit printf expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_printf_expression(encoder *const enc, const node *const nd)
{
	size_t sum_size = 0;
	const size_t argc = expression_call_get_arguments_amount(nd);
	for (size_t i = 1; i < argc; i++)
	{
		const node arg = expression_call_get_argument(nd, i);
		emit_expression(enc, &arg);

		sum_size += type_size(enc->sx, expression_get_type(&arg));
	}

	const node format_string = expression_call_get_argument(nd, 0);
	emit_expression(enc, &format_string);

	mem_add(enc, IC_PRINTF);
	mem_add(enc, (item_t)sum_size);
}

/**
 *	Emit print expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_print_expression(encoder *const enc, const node *const nd)
{
	const size_t argc = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < argc; i++)
	{
		const node arg = expression_call_get_argument(nd, i);
		emit_expression(enc, &arg);

		mem_add(enc, IC_PRINT);
		const item_t arg_type = expression_get_type(&arg);
		const item_t unqualified_arg_type =  type_is_const(enc->sx, arg_type) ? type_const_get_unqualified_type(enc->sx, arg_type) : arg_type;
		mem_add(enc, unqualified_arg_type);
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
	const size_t func = expression_identifier_get_id(&callee);

	switch (func)
	{
		case BI_PRINTF:
			emit_printf_expression(enc, nd);
			return;
		case BI_PRINT:
			emit_print_expression(enc, nd);
			return;
		case BI_PRINTID:
			emit_printid_expression(enc, nd);
			return;
		case BI_GETID:
			emit_getid_expression(enc, nd);
			return;
	}

	if (func >= BEGIN_USER_FUNC)
	{
		mem_add(enc, IC_CALL1);
	}

	const size_t args = expression_call_get_arguments_amount(nd);
	for (size_t i = 0; i < args; i++)
	{
		const node argument = expression_call_get_argument(nd, i);
		emit_argument(enc, &argument);
	}

	if (func >= BEGIN_USER_FUNC)
	{
		mem_add(enc, IC_CALL2);
		mem_add(enc, functions_get(enc, func));
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
static void emit_member_expression(encoder *const enc, const node *const nd)
{
	// Member expression может выдать rvalue только в одном случае: слева rvalue и оператор '.'
	const node base = expression_member_get_base(nd);
	emit_expression(enc, &base);

	const item_t base_type = expression_get_type(&base);
	const size_t member_index = expression_member_get_member_index(nd);

	size_t member_displ = 0;
	for (size_t i = 0; i < member_index; i++)
	{
		const item_t member_type = type_structure_get_member_type(enc->sx, base_type, i);
		member_displ += type_size(enc->sx, member_type);
	}

	mem_add(enc, IC_COPYST);
	mem_add(enc, (item_t)member_displ);
	mem_add(enc, (item_t)type_size(enc->sx, expression_get_type(nd)));
	mem_add(enc, (item_t)type_size(enc->sx, base_type));
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

	const item_t target_type = expression_get_type(nd);
	const item_t source_type = expression_get_type(&subexpr);

	// Необходимо только преобразование 'int' -> 'float'
	if (type_is_integer(enc->sx, source_type) && type_is_floating(enc->sx, target_type))
	{
		mem_add(enc, IC_WIDEN);
	}
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

	const unary_t op = expression_unary_get_operator(nd);
	instruction_t instruction = unary_to_instruction(op);

	if (value.kind == ADDRESS)
	{
		instruction = instruction_to_address_ver(instruction);
	}

	if (type_is_floating(enc->sx, expression_get_type(nd)))
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
			// Unreachable
			return;

		case UN_MINUS:
			emit_expression(enc, &operand);
			mem_add(enc, type_is_integer(enc->sx, type) ? IC_UNMINUS : IC_UNMINUS_R);
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
			mem_add(enc, type_is_integer(enc->sx, type) ? IC_ABSI : IC_ABS);
			return;

		case UN_UPB:
		{
			mem_add(enc, IC_LI);
			mem_add(enc, 0);

			emit_expression(enc, &operand);
			mem_add(enc, IC_UPB);
			return;
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
	const node LHS = expression_binary_get_LHS(nd);
	const node RHS = expression_binary_get_RHS(nd);
	if (operator == BIN_COMMA)
	{
		emit_void_expression(enc, &LHS);
		emit_expression(enc, &RHS);
	}
	else
	{
		emit_expression(enc, &LHS);

		size_t addr = SIZE_MAX;
		const bool is_logical = operator == BIN_LOG_AND || operator == BIN_LOG_OR;
		if (is_logical)
		{
			mem_add(enc, IC_DUPLICATE);
			mem_add(enc, operator == BIN_LOG_AND ? IC_BE0 : IC_BNE0);
			addr = mem_reserve(enc);
		}

		emit_expression(enc, &RHS);

		if (is_logical)
		{
			mem_set(enc, addr, (item_t)mem_size(enc) + 1);
		}

		const instruction_t instruction = binary_to_instruction(operator);
		if (type_is_floating(enc->sx, expression_get_type(&LHS)))
		{
			mem_add(enc, instruction_to_floating_ver(instruction));
		}
		else
		{
			mem_add(enc, instruction);
		}
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
 *	Emit assignment expression
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_assignment_expression(encoder *const enc, const node *const nd)
{
	const node LHS = expression_assignment_get_LHS(nd);
	const lvalue value = emit_lvalue(enc, &LHS);

	const node RHS = expression_assignment_get_RHS(nd);
	emit_expression(enc, &RHS);

	const item_t type = expression_get_type(nd);
	if (type_is_structure(enc->sx, type))
	{
		if (value.kind == VARIABLE)
		{
			mem_add(enc, IC_COPY0ST_ASSIGN);
			mem_add(enc, value.displ);
		}
		else
		{
			mem_add(enc, IC_COPY1ST_ASSIGN);
		}

		mem_add(enc, (item_t)type_size(enc->sx, type));
	}
	else // Скалярное присваивание
	{
		const binary_t op = expression_assignment_get_operator(nd);
		instruction_t instruction = binary_to_instruction(op);
		if (value.kind == ADDRESS)
		{
			instruction = instruction_to_address_ver(instruction);
		}

		if (type_is_floating(enc->sx, type))
		{
			instruction = instruction_to_floating_ver(instruction);
		}

		mem_add(enc, instruction);

		if (value.kind == VARIABLE)
		{
			mem_add(enc, value.displ);
		}
	}
}

/**
 *	Emit initializer
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_initializer(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	const size_t size = expression_initializer_get_size(nd);

	if (type_is_array(enc->sx, type))
	{
		mem_add(enc, IC_BEG_INIT);
		mem_add(enc, (item_t)size);
	}

	for (size_t i = 0; i < size; i++)
	{
		const node subexpr = expression_initializer_get_subexpr(nd, i);
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
			emit_member_expression(enc, nd);
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

		case EXPR_ASSIGNMENT:
			emit_assignment_expression(enc, nd);
			return;

		case EXPR_INITIALIZER:
			emit_initializer(enc, nd);
			return;

		default:
			// Cannot be rvalue
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

		const size_t index = mem_get(enc, mem_size(enc) - 1) < MIN_INSTRUCTION_CODE
			? mem_size(enc) - 2
			: mem_size(enc) - 1;

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

		case EXPR_INITIALIZER:
		{
			const size_t size = expression_initializer_get_size(nd);
			for (size_t i = 0; i < size; i++)
			{
				const node subexpr = expression_initializer_get_subexpr(nd, i);
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
	const size_t identifier = declaration_variable_get_id(nd);
	item_t type = ident_get_type(enc->sx, identifier);
	item_t dimensions = 0;
	bool has_empty_bounds = false;

	while (type_is_array(enc->sx, type))
	{
		type = type_array_get_element_type(enc->sx, type);
		const node bound = declaration_variable_get_bound(nd, (size_t)dimensions);
		if (expression_get_class(&bound) == EXPR_EMPTY_BOUND)
		{
			if (type_is_array(enc->sx, type))
			{
				encoder_error(enc, node_get_location(&bound), empty_init);
			}

			has_empty_bounds = true;
		}
		else
		{
			emit_expression(enc, &bound);
		}

		dimensions++;
	}

	if (type_is_const(enc->sx, type))
	{
		type = type_const_get_unqualified_type(enc->sx, type);
	}

	const bool has_initializer = declaration_variable_has_initializer(nd);
	const item_t length = (item_t)type_size(enc->sx, type);
	const item_t displ = displacements_add(enc, identifier);
	const item_t iniproc = proc_get(enc, (size_t)type);

	mem_add(enc, IC_DEFARR); 		// DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
	mem_add(enc, has_initializer ? dimensions - 1 : dimensions);
	mem_add(enc, length);
	mem_add(enc, displ);
	mem_add(enc, iniproc && iniproc != ITEM_MAX ? iniproc : 0);

	const size_t usual_addr = mem_size(enc);
	item_t usual = has_empty_bounds ? 0 : 1;
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
		mem_add(enc, dimensions);
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
	if (type_is_array(enc->sx, type))
	{
		emit_array_declaration(enc, nd);
		return;
	}

	const item_t displ = displacements_add(enc, identifier);
	const item_t unqualified_type = type_is_const(enc->sx, type) ? type_const_get_unqualified_type(enc->sx, type) : type;
	const item_t iniproc = proc_get(enc, (size_t)unqualified_type);
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

		if (type_is_structure(enc->sx, unqualified_type))
		{
			mem_add(enc, IC_COPY0ST_ASSIGN);
			mem_add(enc, displ);
			mem_add(enc, (item_t)type_size(enc->sx, unqualified_type));
		}
		else
		{
			mem_add(enc, type_is_floating(enc->sx, unqualified_type) ? IC_ASSIGN_R_V : IC_ASSIGN_V);
			mem_add(enc, displ);
		}
	}
}

/**
 *	Emit member declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 *	@param	displ		Member displacement
 */
static void emit_member_declaration(encoder *const enc, const node *const nd, const item_t displ)
{
	item_t member_type = declaration_member_get_type(nd);
	if (!type_is_array(enc->sx, member_type))
	{
		return;
	}

	while (type_is_array(enc->sx, member_type))
	{
		member_type = type_array_get_element_type(enc->sx, member_type);
	}

	const size_t bounds = declaration_member_get_bounds_amount(nd);
	for (size_t i = 0; i < bounds; i++)
	{
		const node bound = declaration_member_get_bound(nd, i);
		emit_expression(enc, &bound);
	}

	const size_t length = type_size(enc->sx, member_type);
	const item_t iniproc = proc_get(enc, (size_t)member_type);

	mem_add(enc, IC_DEFARR); 		// DEFARR N, d, displ, iniproc, usual N1...NN, уже лежат на стеке
	mem_add(enc, (item_t)bounds);
	mem_add(enc, (item_t)length);
	mem_add(enc, (item_t)displ);
	mem_add(enc, iniproc && iniproc != ITEM_MAX ? iniproc : 0);
	mem_add(enc, 1);				// usual
	mem_add(enc, 0);				// has initializer
	mem_add(enc, 1);				// is in structure
}

/**
 *	Emit struct declaration
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_struct_declaration(encoder *const enc, const node *const nd)
{
	const item_t type = declaration_struct_get_type(nd);
	const size_t members = type_structure_get_member_amount(enc->sx, type);

	bool has_arrays = false;
	for (size_t i = 0; i < members; i++)
	{
		const item_t member_type = type_structure_get_member_type(enc->sx, type, i);
		if (type_is_array(enc->sx, member_type))
		{
			has_arrays = true;
		}
	}

	if (!has_arrays)
	{
		return;
	}

	mem_add(enc, IC_B);
	const size_t addr = mem_reserve(enc);

	proc_set(enc, (size_t)type, (item_t)addr + 1);

	item_t displ = 0;
	const size_t size = declaration_struct_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node member = declaration_struct_get_member(nd, i);
		if (declaration_get_class(&member) == DECL_MEMBER)
		{
			emit_member_declaration(enc, &member, displ);
			displ += type_size(enc->sx, declaration_member_get_type(&member));
		}
		else
		{
			emit_declaration(enc, &member);
		}
	}

	mem_add(enc, IC_STOP);
	mem_set(enc, addr, (item_t)mem_size(enc));
}

/**
 *	Emit function definition
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_function_definition(encoder *const enc, const node *const nd)
{
	const size_t identifier = declaration_function_get_id(nd);
	functions_add(enc, identifier, mem_size(enc));

	enc->curr_func = nd;
	enc->displ = DISPL_START;
	enc->max_local_displ = enc->displ;

	const size_t parameters_amount = declaration_function_get_parameters_amount(nd);
	for (size_t i = 0; i < parameters_amount; i++)
	{
		const size_t parameter = declaration_function_get_parameter(nd, i);
		displacements_add(enc, parameter);
	}

	mem_add(enc, IC_FUNC_BEG);

	const size_t displ_addr = mem_reserve(enc);
	const size_t jump_addr = mem_reserve(enc);

	const node function_body = declaration_function_get_body(nd);
	emit_statement(enc, &function_body);
	mem_add(enc, IC_RETURN_VOID);

	mem_set(enc, displ_addr, enc->max_local_displ);
	mem_set(enc, jump_addr, (item_t)mem_size(enc));
	enc->curr_func = NULL;
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

		case DECL_STRUCT:
			emit_struct_declaration(enc, nd);
			return;

		case DECL_FUNC:
			emit_function_definition(enc, nd);
			return;

		default:
			// Unreachable
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
 *	Emit declaration statement
 *
 *	@param	enc			Encoder
 *	@param	nd			Node in AST
 */
static void emit_declaration_statement(encoder *const enc, const node *const nd)
{
	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		emit_declaration(enc, &decl);
	}
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

	const node expr = statement_case_get_expression(nd);
	emit_expression(enc, &expr);

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
	const item_t scope_displacement = enc->displ;
	const size_t size = statement_compound_get_size(nd);

	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}

	enc->displ = scope_displacement;
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
	const item_t scope_displacement = enc->displ;
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
	enc->displ = scope_displacement;
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
		const item_t type = expression_get_type(&expr);

		emit_expression(enc, &expr);

		mem_add(enc, IC_RETURN_VAL);
		mem_add(enc, (item_t)type_size(enc->sx, type));
	}
	else
	{
		mem_add(enc, IC_RETURN_VOID);
	}
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
			emit_declaration_statement(enc, nd);
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

		case STMT_CONTINUE:
			emit_continue_statement(enc);
			return;

		case STMT_BREAK:
			emit_break_statement(enc);
			return;

		case STMT_RETURN:
			emit_return_statement(enc, nd);
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

	const item_t main_displ = displacements_get(enc, enc->sx->ref_main);

	mem_add(enc, IC_CALL1);
	mem_add(enc, IC_CALL2);
	mem_add(enc, main_displ);
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

	int ret = reporter_get_errors_number(&enc.sx->rprt) != 0 ? 1 : 0;
	if (!ret)
	{
		ret = enc_export(&enc);
	}

	enc_clear(&enc);
	return ret;
}
