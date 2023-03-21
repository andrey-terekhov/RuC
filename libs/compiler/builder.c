/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "builder.h"
#include "AST.h"


#define MAX_PRINTF_ARGS 20


/**
 *	Emit a semantic error
 *
 *	@param	bldr		AST builder
 *	@param	loc			Error location
 *	@param	num			Error code
 */
static void semantic_error(builder *const bldr, const location loc, err_t num, ...)
{
	va_list args;
	va_start(args, num);

	report_error(&bldr->sx->rprt, bldr->sx->io, loc, num, args);

	va_end(args);
}

/**
 *	Emit a semantic warning
 *
 *	@param	bldr		AST builder
 *	@param	loc			Warning location
 *	@param	num			Warning code
 */
static void semantic_warning(builder *const bldr, const location loc, warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	report_warning(&bldr->sx->rprt, bldr->sx->io, loc, num, args);

	va_end(args);
}


static item_t usual_arithmetic_conversions(const syntax *const sx, node *const LHS, node *const RHS)
{
	const item_t LHS_type = expression_get_type(LHS);
	const item_t RHS_type = expression_get_type(RHS);

	if (type_is_floating(sx, LHS_type) || type_is_floating(sx, RHS_type))
	{
		*LHS = build_cast_expression(TYPE_FLOATING, LHS);
		*RHS = build_cast_expression(TYPE_FLOATING, RHS);

		return TYPE_FLOATING;
	}

	return TYPE_INTEGER;
}

static node fold_unary_expression(builder *const bldr, const item_t type, const category_t ctg
	, node *const expr, const unary_t op, const location loc)
{
	if (expression_get_class(expr) != EXPR_LITERAL)
	{
		return expression_unary(type, ctg, expr, op, loc);
	}

	switch (type_get_class(bldr->sx, expression_get_type(expr)))
	{
		case TYPE_NULL_POINTER:
			node_remove(expr);
			return build_boolean_literal_expression(bldr, true, loc);

		case TYPE_BOOLEAN:
		{
			const bool value = expression_literal_get_boolean(expr);
			node_remove(expr);
			return build_boolean_literal_expression(bldr, !value, loc);
		}

		case TYPE_ENUM:
		case TYPE_INTEGER:
		{
			const item_t value = expression_literal_get_integer(expr);
			node_remove(expr);

			switch (op)
			{
				case UN_MINUS:
					return build_integer_literal_expression(bldr, -value, loc);
				case UN_NOT:
					return build_integer_literal_expression(bldr, ~value, loc);
				case UN_LOGNOT:
					return build_boolean_literal_expression(bldr, value == 0, loc);
				case UN_ABS:
					return build_integer_literal_expression(bldr, value >= 0 ? value : -value, loc);
				default:
					return node_broken();
			}
		}

		case TYPE_FLOATING:
		{
			const double value = expression_literal_get_floating(expr);
			node_remove(expr);

			switch (op)
			{
				case UN_MINUS:
					return build_floating_literal_expression(bldr, -value, loc);
				case UN_ABS:
					return build_floating_literal_expression(bldr, value >= 0 ? value : -value, loc);
				default:
					return node_broken();
			}
		}

		default:
			return expression_unary(type, ctg, expr, op, loc);
	}
}

static node fold_binary_expression(builder *const bldr, const item_t type
	, node *const LHS, node *const RHS, const binary_t op, const location loc)
{
	if (expression_get_class(LHS) != EXPR_LITERAL || expression_get_class(RHS) != EXPR_LITERAL)
	{
		return expression_binary(type, LHS, RHS, op, loc);
	}

	switch (type_get_class(bldr->sx, expression_get_type(LHS)))
	{
		case TYPE_ENUM:
		case TYPE_INTEGER:
		case TYPE_BOOLEAN:
		{
			const item_t left_value = expression_literal_get_integer(LHS);
			const item_t right_value = expression_literal_get_integer(RHS);
			node_remove(LHS);
			node_remove(RHS);

			switch (op)
			{
				case BIN_MUL:
					return build_integer_literal_expression(bldr, left_value * right_value, loc);
				case BIN_DIV:
					return build_integer_literal_expression(bldr, left_value / right_value, loc);
				case BIN_REM:
					return build_integer_literal_expression(bldr, left_value % right_value, loc);
				case BIN_ADD:
					return build_integer_literal_expression(bldr, left_value + right_value, loc);
				case BIN_SUB:
					return build_integer_literal_expression(bldr, left_value - right_value, loc);
				case BIN_SHL:
					return build_integer_literal_expression(bldr, left_value << right_value, loc);
				case BIN_SHR:
					return build_integer_literal_expression(bldr, left_value >> right_value, loc);
				case BIN_LT:
					return build_boolean_literal_expression(bldr, left_value < right_value, loc);
				case BIN_GT:
					return build_boolean_literal_expression(bldr, left_value > right_value, loc);
				case BIN_LE:
					return build_boolean_literal_expression(bldr, left_value <= right_value, loc);
				case BIN_GE:
					return build_boolean_literal_expression(bldr, left_value >= right_value, loc);
				case BIN_EQ:
					return build_boolean_literal_expression(bldr, left_value == right_value, loc);
				case BIN_NE:
					return build_boolean_literal_expression(bldr, left_value != right_value, loc);
				case BIN_AND:
					return build_integer_literal_expression(bldr, left_value & right_value, loc);
				case BIN_XOR:
					return build_integer_literal_expression(bldr, left_value ^ right_value, loc);
				case BIN_OR:
					return build_integer_literal_expression(bldr, left_value | right_value, loc);
				case BIN_LOG_AND:
					return build_boolean_literal_expression(bldr, left_value && right_value, loc);
				case BIN_LOG_OR:
					return build_boolean_literal_expression(bldr, left_value || right_value, loc);
				default:
					return node_broken();
			}
		}

		case TYPE_FLOATING:
		{
			const double left_value = expression_literal_get_floating(LHS);
			const double right_value = expression_literal_get_floating(RHS);

			node_remove(LHS);
			node_remove(RHS);

			switch (op)
			{
				case BIN_MUL:
					return build_floating_literal_expression(bldr, left_value * right_value, loc);
				case BIN_DIV:
					return build_floating_literal_expression(bldr, left_value / right_value, loc);
				case BIN_ADD:
					return build_floating_literal_expression(bldr, left_value + right_value, loc);
				case BIN_SUB:
					return build_floating_literal_expression(bldr, left_value - right_value, loc);
				case BIN_LT:
					return build_boolean_literal_expression(bldr, left_value < right_value, loc);
				case BIN_GT:
					return build_boolean_literal_expression(bldr, left_value > right_value, loc);
				case BIN_LE:
					return build_boolean_literal_expression(bldr, left_value <= right_value, loc);
				case BIN_GE:
					return build_boolean_literal_expression(bldr, left_value >= right_value, loc);
				case BIN_EQ:
					return build_boolean_literal_expression(bldr, left_value == right_value, loc);
				case BIN_NE:
					return build_boolean_literal_expression(bldr, left_value != right_value, loc);
				default:
					return node_broken();
			}
		}

		default:
			return expression_binary(type, LHS, RHS, op, loc);
	}
}


static size_t evaluate_args(builder *const bldr, const node *const format_str
	, item_t *const format_types, char32_t *const placeholders)
{
	const size_t str_index = expression_literal_get_string(format_str);
	const char *const string = string_get(bldr->sx, str_index);

	size_t args = 0;
	for (size_t i = 0; string[i] != '\0'; i += utf8_symbol_size(string[i]))
	{
		if (utf8_convert(&string[i]) == '%')
		{
			i += utf8_symbol_size(string[i]);
			const char32_t specifier = utf8_convert(&string[i]);
			if (specifier != '%')
			{
				if (args == MAX_PRINTF_ARGS)
				{
					semantic_error(bldr, node_get_location(format_str), too_many_printf_args, (size_t)MAX_PRINTF_ARGS);
					return 0;
				}

				placeholders[args] = specifier;
			}

			switch (specifier)
			{
				case 'i':
				case U'ц':
				case 'c':
				case U'л':
					format_types[args++] = TYPE_INTEGER;
					break;

				case 'f':
				case U'в':
					format_types[args++] = TYPE_FLOATING;
					break;

				case 's':
				case U'с':
					format_types[args++] = type_string(bldr->sx);
					break;

				case '%':
					break;

				case '\0':
					semantic_error(bldr, node_get_location(format_str), expected_format_specifier);
					return 0;

				default:
					semantic_error(bldr, node_get_location(format_str), unknown_format_specifier, specifier);
					return 0;
			}
		}
	}

	return args;
}

static node build_printf_expression(builder *const bldr, node *const callee, node_vector *const args, const location r_loc)
{
	const size_t argc = node_vector_size(args);
	if (args == NULL || argc == 0)
	{
		semantic_error(bldr, r_loc, printf_fst_not_string);
		return node_broken();
	}

	if (argc >= MAX_PRINTF_ARGS)
	{
		semantic_error(bldr, r_loc, too_many_printf_args);
		return node_broken();
	}

	const node fst = node_vector_get(args, 0);
	if (expression_get_class(&fst) != EXPR_LITERAL || !type_is_string(bldr->sx, expression_get_type(&fst)))
	{
		semantic_error(bldr, node_get_location(&fst), printf_fst_not_string);
		return node_broken();
	}

	char32_t placeholders[MAX_PRINTF_ARGS];
	item_t format_types[MAX_PRINTF_ARGS];
	const size_t expected_args = evaluate_args(bldr, &fst, format_types, placeholders);

	if (expected_args != argc - 1)
	{
		semantic_error(bldr, r_loc, wrong_printf_argument_amount);
		return node_broken();
	}

	for (size_t i = 1; i < argc; i++)
	{
		node argument = node_vector_get(args, i);
		if (!check_assignment_operands(bldr, format_types[i - 1], &argument, /*is_declaration:*/ true))
		{
			// FIXME: кинуть другую ошибку
			return node_broken();
		}

		node_vector_set(args, i, &argument);
	}

	const location loc = { node_get_location(callee).begin, r_loc.end };
	return expression_call(TYPE_INTEGER, callee, args, loc);
}

static node build_print_expression(builder *const bldr, node *const callee, node_vector *const args, const location r_loc)
{
	size_t argc;
	if (args == NULL || (argc = node_vector_size(args)) == 0)
	{
		semantic_error(bldr, r_loc, expected_expression);
		return node_broken();
	}

	for (size_t i = 0; i < argc; i++)
	{
		const node argument = node_vector_get(args, i);
		if (type_is_pointer(bldr->sx, expression_get_type(&argument)))
		{
			semantic_error(bldr, node_get_location(&argument), pointer_in_print);
			return node_broken();
		}
	}

	const location loc = { node_get_location(callee).begin, r_loc.end };
	return expression_call(TYPE_VOID, callee, args, loc);
}

static node build_printid_expression(builder *const bldr, node *const callee, node_vector *const args, const location r_loc)
{
	const size_t argc = node_vector_size(args);
	if (args == NULL || argc == 0)
	{
		semantic_error(bldr, r_loc, expected_identifier_in_printid);
		return node_broken();
	}

	for (size_t i = 0; i < argc; i++)
	{
		const node argument = node_vector_get(args, i);
		if (node_is_correct(&argument) && expression_get_class(&argument) != EXPR_IDENTIFIER)
		{
			semantic_error(bldr, node_get_location(&argument), expected_identifier_in_printid);
			return node_broken();
		}
	}

	const location loc = { node_get_location(callee).begin, r_loc.end };
	return expression_call(TYPE_VOID, callee, args, loc);
}

static node build_getid_expression(builder *const bldr, node *const callee, node_vector *const args, const location r_loc)
{
	const size_t argc = node_vector_size(args);
	if (args == NULL || argc == 0)
	{
		semantic_error(bldr, r_loc, expected_identifier_in_getid);
		return node_broken();
	}

	for (size_t i = 0; i < argc; i++)
	{
		const node argument = node_vector_get(args, i);
		if (node_is_correct(&argument) && expression_get_class(&argument) != EXPR_IDENTIFIER)
		{
			semantic_error(bldr, node_get_location(&argument), expected_identifier_in_getid);
			return node_broken();
		}
	}

	const location loc = { node_get_location(callee).begin, r_loc.end };
	return expression_call(TYPE_VOID, callee, args, loc);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


builder builder_create(syntax *const sx)
{
	builder bldr;
	bldr.sx = sx;

	node root = node_get_root(&bldr.sx->tree);
	node_copy(&bldr.context, &root);

	return bldr;
}


bool check_assignment_operands(builder *const bldr, const item_t expected_type, node *const init, const bool is_declaration)
{
	if (!node_is_correct(init))
	{
		return true;
	}

	syntax *const sx = bldr->sx;
	const location loc = node_get_location(init);

	const item_t expected_type_unqualified = type_is_const(sx, expected_type) 
		? type_const_get_unqualified_type(sx, expected_type)
		: expected_type;
	if (type_is_const(sx, expected_type) && !is_declaration)
	{
		semantic_error(bldr, loc, assign_to_const);
		return false;
	}

	if (expression_get_class(init) == EXPR_INITIALIZER)
	{
		const size_t actual_inits = expression_initializer_get_size(init);
		if (type_is_structure(sx, expected_type_unqualified))
		{
			const size_t expected_inits = type_structure_get_member_amount(sx, expected_type_unqualified);
			if (expected_inits != actual_inits)
			{
				semantic_error(bldr, loc, wrong_init_in_actparam, expected_inits, actual_inits);
				return false;
			}

			for (size_t i = 0; i < actual_inits; i++)
			{
				const item_t type = type_structure_get_member_type(sx, expected_type_unqualified, i);
				node subexpr = expression_initializer_get_subexpr(init, i);
				if (!check_assignment_operands(bldr, type, &subexpr, is_declaration))
				{
					return false;
				}
			}

			expression_initializer_set_type(init, expected_type_unqualified);
			return true;
		}
		else if (type_is_array(sx, expected_type_unqualified))
		{
			const item_t type = type_array_get_element_type(sx, expected_type_unqualified);
			for (size_t i = 0; i < actual_inits; i++)
			{
				node subexpr = expression_initializer_get_subexpr(init, i);
				if (!check_assignment_operands(bldr, type, &subexpr, is_declaration))
				{
					return false;
				}
			}

			expression_initializer_set_type(init, expected_type_unqualified);
			return true;
		}
		else
		{
			semantic_error(bldr, loc, wrong_init);
			return false;
		}
	}

	const item_t expr_type = expression_get_type(init);
	const item_t actual_type = type_is_const(sx, expr_type) ? type_const_get_unqualified_type(sx, expr_type) : expr_type;
	if (type_is_floating(sx, expected_type_unqualified) && type_is_integer(sx, actual_type))
	{
		*init = build_cast_expression(expected_type_unqualified, init);
		return true;
	}

	if (type_is_enum(sx, expected_type_unqualified) && type_is_enum_field(sx, actual_type))
	{
		return true; // check enum initializer
	}

	if (type_is_integer(sx, expected_type_unqualified) && (type_is_enum(sx, actual_type) || type_is_enum_field(sx, actual_type)))
	{
		return true; // check int initializer
	}

	if (type_is_integer(sx, expected_type_unqualified) && type_is_integer(sx, actual_type))
	{
		return true;
	}

	if (type_is_pointer(sx, expected_type_unqualified) && type_is_null_pointer(actual_type))
	{
		return true;
	}

	if (type_is_pointer(sx, expected_type_unqualified) && type_is_pointer(sx, actual_type))
	{
		const item_t expected_element_type = type_pointer_get_element_type(sx, expected_type_unqualified);
		const item_t actual_element_type = type_pointer_get_element_type(sx, actual_type);
		if (!type_is_const(sx, expected_element_type) && type_is_const(sx, actual_element_type))
		{
			semantic_error(bldr, loc, invalid_const_pointer_cast);
			return false;
		}
		if (type_is_const(sx, expected_element_type) && !type_is_const(sx, actual_element_type)
			&& actual_element_type == type_const_get_unqualified_type(sx, expected_element_type))
		{
			return true;
		}
	}

	if (expected_type_unqualified == actual_type)
	{
		return true;
	}

	semantic_error(bldr, loc, wrong_init);
	return false;
}

node build_identifier_expression(builder *const bldr, const size_t name, const location loc)
{
	const item_t identifier = repr_get_reference(bldr->sx, name);

	if (identifier == ITEM_MAX)
	{
		semantic_error(bldr, loc, use_of_undeclared_identifier, repr_get_name(bldr->sx, name));
		return node_broken();
	}

	const item_t type = ident_get_type(bldr->sx, (size_t)identifier);
	if (type_is_enum_field(bldr->sx, type))
	{
		const item_t enum_type = get_enum_field_type(bldr->sx, type);
		const item_t value = ident_get_displ(bldr->sx, (size_t)identifier);
		return expression_integer_literal(&bldr->context, enum_type, value, loc);
	}

	return expression_identifier(&bldr->context, type, (size_t)identifier, loc);
}

node build_null_literal_expression(builder *const bldr, const location loc)
{
	return expression_null_literal(&bldr->context, TYPE_NULL_POINTER, loc);
}

node build_boolean_literal_expression(builder *const bldr, const bool value, const location loc)
{
	return expression_boolean_literal(&bldr->context, TYPE_BOOLEAN, value, loc);
}

node build_character_literal_expression(builder *const bldr, const char32_t value, const location loc)
{
	return expression_character_literal(&bldr->context, TYPE_CHARACTER, value, loc);
}

node build_integer_literal_expression(builder *const bldr, const item_t value, const location loc)
{
	return expression_integer_literal(&bldr->context, TYPE_INTEGER, value, loc);
}

node build_floating_literal_expression(builder *const bldr, const double value, const location loc)
{
	return expression_floating_literal(&bldr->context, TYPE_FLOATING, value, loc);
}

node build_string_literal_expression(builder *const bldr, const size_t index, const location loc)
{
	return expression_string_literal(&bldr->context, type_string(bldr->sx), index, loc);
}

node build_subscript_expression(builder *const bldr, node *const base, node *const index
	, const location l_loc, const location r_loc)
{
	if (!node_is_correct(base) || !node_is_correct(index))
	{
		return node_broken();
	}

	const item_t base_type = expression_get_type(base);
	if (!type_is_array(bldr->sx, base_type))
	{
		semantic_error(bldr, l_loc, subscripted_expr_not_array);
		return node_broken();
	}

	const item_t index_type = expression_get_type(index);
	if (!type_is_integer(bldr->sx, index_type))
	{
		semantic_error(bldr, node_get_location(index), array_subscript_not_integer);
		return node_broken();
	}

	const item_t element_type = type_array_get_element_type(bldr->sx, base_type);

	const location loc = { node_get_location(base).begin, r_loc.end };
	return expression_subscript(element_type, base, index, loc);
}

node build_call_expression(builder *const bldr, node *const callee
	, node_vector *const args, const location l_loc, const location r_loc)
{
	if (!node_is_correct(callee))
	{
		return node_broken();
	}

	const item_t callee_type = expression_get_type(callee);
	if (!type_is_function(bldr->sx, callee_type))
	{
		semantic_error(bldr, l_loc, called_expr_not_function);
		return node_broken();
	}

	if (expression_get_class(callee) == EXPR_IDENTIFIER)
	{
		switch (expression_identifier_get_id(callee))
		{
			case BI_PRINTF:
				return build_printf_expression(bldr, callee, args, r_loc);
			case BI_PRINT:
				return build_print_expression(bldr, callee, args, r_loc);
			case BI_PRINTID:
				return build_printid_expression(bldr, callee, args, r_loc);
			case BI_GETID:
				return build_getid_expression(bldr, callee, args, r_loc);
			default:
				break;
		}
	}

	const size_t expected_args = type_function_get_parameter_amount(bldr->sx, callee_type);
	const size_t actual_args = args != NULL ? node_vector_size(args) : 0;

	if (expected_args != actual_args)
	{
		semantic_error(bldr, r_loc, wrong_argument_amount, expected_args, actual_args);
		return node_broken();
	}

	for (size_t i = 0; i < actual_args; i++)
	{
		const item_t expected_type = type_function_get_parameter_type(bldr->sx, callee_type, i);
		node argument = node_vector_get(args, i);
		if (!check_assignment_operands(bldr, expected_type, &argument, /*is_declaration:*/ true))
		{
			return node_broken();
		}

		node_vector_set(args, i, &argument);
	}

	const item_t return_type = type_function_get_return_type(bldr->sx, callee_type);
	const location loc = { node_get_location(callee).begin, r_loc.end };
	return expression_call(return_type, callee, args, loc);
}

node build_member_expression(builder *const bldr, node *const base, const size_t name
	, const bool is_arrow, const location op_loc, const location id_loc)
{
	if (!node_is_correct(base))
	{
		return node_broken();
	}

	const item_t base_type = expression_get_type(base);
	item_t struct_type;
	category_t category;

	if (!is_arrow)
	{
		if (!type_is_structure(bldr->sx, base_type))
		{
			semantic_error(bldr, op_loc, member_reference_not_struct);
			return node_broken();
		}

		struct_type = base_type;
		category = expression_is_lvalue(base) ? LVALUE : RVALUE;
	}
	else
	{
		if (!type_is_struct_pointer(bldr->sx, base_type))
		{
			semantic_error(bldr, op_loc, member_reference_not_struct_pointer);
			return node_broken();
		}

		struct_type = type_pointer_get_element_type(bldr->sx, base_type);
		category = LVALUE;
	}

	const size_t member_amount = type_structure_get_member_amount(bldr->sx, struct_type);
	for (size_t i = 0; i < member_amount; i++)
	{
		if (name == type_structure_get_member_name(bldr->sx, struct_type, i))
		{
			const item_t type = type_structure_get_member_type(bldr->sx, struct_type, i);
			const location loc = { node_get_location(base).begin, id_loc.end };

			return expression_member(type, category, i, is_arrow, base, loc);
		}
	}

	semantic_error(bldr, id_loc, no_such_member, repr_get_name(bldr->sx, name));
	return node_broken();
}

node build_cast_expression(const item_t target_type, node *const expr)
{
	if (!node_is_correct(expr))
	{
		return node_broken();
	}

	const item_t source_type = expression_get_type(expr);
	const location loc = node_get_location(expr);

	if (target_type != source_type)
	{
		if (expression_get_class(expr) == EXPR_LITERAL)
		{
			// Пока тут только int -> float
			const item_t value = expression_literal_get_integer(expr);
			const node result = node_insert(expr, OP_LITERAL, DOUBLE_SIZE + 4);

			node_set_arg(&result, 0, TYPE_FLOATING);
			node_set_arg(&result, 1, RVALUE);
			node_set_arg_double(&result, 2, (double)value);
			node_set_arg(&result, DOUBLE_SIZE + 2, (item_t)loc.begin);
			node_set_arg(&result, DOUBLE_SIZE + 3, (item_t)loc.end);

			node_remove(expr);
			return result;
		}

		return expression_cast(target_type, source_type, expr, loc);
	}

	return *expr;
}

node build_unary_expression(builder *const bldr, node *const operand, const unary_t op_kind, const location op_loc)
{
	if (!node_is_correct(operand))
	{
		return node_broken();
	}

	const item_t operand_type = expression_get_type(operand);

	const location loc = op_kind == UN_POSTINC || op_kind == UN_POSTDEC
		? (location){ node_get_location(operand).begin, op_loc.end }
		: (location){ op_loc.begin, node_get_location(operand).end };

	switch (op_kind)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
		{
			if (!expression_is_lvalue(operand))
			{
				semantic_error(bldr, op_loc, unassignable_expression);
				return node_broken();
			}

			if (!type_is_arithmetic(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, increment_operand_not_arithmetic, op_kind);
				return node_broken();
			}

			return expression_unary(operand_type, RVALUE, operand, op_kind, loc);
		}

		case UN_ADDRESS:
		{
			if (!expression_is_lvalue(operand))
			{
				semantic_error(bldr, op_loc, addrof_operand_not_lvalue);
				return node_broken();
			}

			const item_t type = type_pointer(bldr->sx, operand_type);
			return expression_unary(type, RVALUE, operand, op_kind, loc);
		}

		case UN_INDIRECTION:
		{
			if (!type_is_pointer(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, indirection_operand_not_pointer);
				return node_broken();
			}

			const item_t type = type_pointer_get_element_type(bldr->sx, operand_type);
			return expression_unary(type, LVALUE, operand, op_kind, loc);
		}

		case UN_ABS:
		case UN_MINUS:
		{
			if (!type_is_arithmetic(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, unary_operand_not_arithmetic, operand_type);
				return node_broken();
			}

			return fold_unary_expression(bldr, operand_type, RVALUE, operand, op_kind, loc);
		}

		case UN_NOT:
		{
			if (!type_is_integer(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, unnot_operand_not_integer, operand_type);
				return node_broken();
			}

			return fold_unary_expression(bldr, TYPE_INTEGER, RVALUE, operand, op_kind, loc);
		}

		case UN_LOGNOT:
		{
			if (!type_is_scalar(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, lognot_operand_not_scalar, operand_type);
				return node_broken();
			}

			return fold_unary_expression(bldr, TYPE_BOOLEAN, RVALUE, operand, op_kind, loc);
		}

		case UN_UPB:
		{
			if (!type_is_array(bldr->sx, operand_type))
			{
				semantic_error(bldr, op_loc, upb_operand_not_array, operand_type);
				return node_broken();
			}

			return fold_unary_expression(bldr, TYPE_INTEGER, RVALUE, operand, op_kind, loc);
		}

		default:
			// Unknown unary operator
			return node_broken();
	}
}

node build_binary_expression(builder *const bldr, node *const LHS, node *const RHS
	, const binary_t op_kind, const location op_loc)
{
	if (!node_is_correct(LHS) || !node_is_correct(RHS))
	{
		return node_broken();
	}

	const item_t left_type = expression_get_type(LHS);
	const item_t right_type = expression_get_type(RHS);

	if (operation_is_assignment(op_kind))
	{
		if (!expression_is_lvalue(LHS))
		{
			semantic_error(bldr, op_loc, unassignable_expression);
			return node_broken();
		}

		if (!check_assignment_operands(bldr, left_type, RHS, /*is_declaration:*/ false))
		{
			return node_broken();
		}
	}

	const location loc = { node_get_location(LHS).begin, node_get_location(RHS).end };

	switch (op_kind)
	{
		case BIN_REM:
		case BIN_SHL:
		case BIN_SHR:
		case BIN_AND:
		case BIN_XOR:
		case BIN_OR:
		{
			if (!type_is_integer(bldr->sx, left_type) || !type_is_integer(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			return fold_binary_expression(bldr, TYPE_INTEGER, LHS, RHS, op_kind, loc);
		}

		case BIN_MUL:
		case BIN_DIV:
		case BIN_ADD:
		case BIN_SUB:
		{
			if (!type_is_arithmetic(bldr->sx, left_type) || !type_is_arithmetic(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			const item_t type = usual_arithmetic_conversions(bldr->sx, LHS, RHS);
			return fold_binary_expression(bldr, type, LHS, RHS, op_kind, loc);
		}

		case BIN_LT:
		case BIN_GT:
		case BIN_LE:
		case BIN_GE:
		{
			if (!type_is_arithmetic(bldr->sx, left_type) || !type_is_arithmetic(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			usual_arithmetic_conversions(bldr->sx, LHS, RHS);
			return fold_binary_expression(bldr, TYPE_BOOLEAN, LHS, RHS, op_kind, loc);
		}

		case BIN_EQ:
		case BIN_NE:
		{
			if (type_is_floating(bldr->sx, left_type) || type_is_floating(bldr->sx, right_type))
			{
				semantic_warning(bldr, op_loc, variable_deviation);
			}

			if (type_is_arithmetic(bldr->sx, left_type) && type_is_arithmetic(bldr->sx, right_type))
			{
				usual_arithmetic_conversions(bldr->sx, LHS, RHS);
				return fold_binary_expression(bldr, TYPE_BOOLEAN, LHS, RHS, op_kind, loc);
			}

			if ((type_is_pointer(bldr->sx, left_type) && type_is_null_pointer(right_type))
				|| (type_is_null_pointer(left_type) && type_is_pointer(bldr->sx, right_type))
				|| left_type == right_type)
			{
				return fold_binary_expression(bldr, TYPE_BOOLEAN, LHS, RHS, op_kind, loc);
			}

			semantic_error(bldr, op_loc, typecheck_binary_expr);
			return node_broken();
		}

		case BIN_LOG_AND:
		case BIN_LOG_OR:
		{
			if (!type_is_scalar(bldr->sx, left_type) || !type_is_scalar(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			return fold_binary_expression(bldr, TYPE_BOOLEAN, LHS, RHS, op_kind, loc);
		}

		case BIN_ASSIGN:
			return expression_assignment(left_type, LHS, RHS, op_kind, loc);

		case BIN_REM_ASSIGN:
		case BIN_SHL_ASSIGN:
		case BIN_SHR_ASSIGN:
		case BIN_AND_ASSIGN:
		case BIN_XOR_ASSIGN:
		case BIN_OR_ASSIGN:
		{
			if (!type_is_integer(bldr->sx, left_type) || !type_is_integer(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			return expression_assignment(left_type, LHS, RHS, op_kind, loc);
		}

		case BIN_MUL_ASSIGN:
		case BIN_DIV_ASSIGN:
		case BIN_ADD_ASSIGN:
		case BIN_SUB_ASSIGN:
		{
			if (!type_is_arithmetic(bldr->sx, left_type) || !type_is_arithmetic(bldr->sx, right_type))
			{
				semantic_error(bldr, op_loc, typecheck_binary_expr);
				return node_broken();
			}

			return expression_assignment(left_type, LHS, RHS, op_kind, loc);
		}

		case BIN_COMMA:
			return expression_binary(right_type, LHS, RHS, op_kind, loc);

		default:
			// Unknown binary operator
			return node_broken();
	}
}

node build_ternary_expression(builder *const bldr, node *const cond, node *const LHS, node *const RHS, location op_loc)
{
	if (!node_is_correct(cond) || !node_is_correct(LHS) || !node_is_correct(RHS))
	{
		return node_broken();
	}

	if (expression_get_class(LHS) == EXPR_INITIALIZER)
	{
		semantic_error(bldr, node_get_location(LHS), expected_expression);
		return node_broken();
	}

	if (expression_get_class(RHS) == EXPR_INITIALIZER)
	{
		semantic_error(bldr, node_get_location(RHS), expected_expression);
		return node_broken();
	}

	const item_t cond_type = expression_get_type(cond);
	if (!type_is_scalar(bldr->sx, cond_type))
	{
		semantic_error(bldr, node_get_location(cond), condition_must_be_scalar);
		return node_broken();
	}

	const location loc = { node_get_location(cond).begin, node_get_location(RHS).end };

	const item_t LHS_type = expression_get_type(LHS);
	const item_t RHS_type = expression_get_type(RHS);
	if (type_is_arithmetic(bldr->sx, LHS_type) && type_is_arithmetic(bldr->sx, RHS_type))
	{
		const item_t type = usual_arithmetic_conversions(bldr->sx, LHS, RHS);
		return expression_ternary(type, cond, LHS, RHS, loc);
	}

	if (type_is_pointer(bldr->sx, LHS_type) && type_is_null_pointer(RHS_type))
	{
		return expression_ternary(LHS_type, cond, LHS, RHS, loc);
	}

	if ((type_is_null_pointer(LHS_type) && type_is_pointer(bldr->sx, RHS_type))
		|| (LHS_type == RHS_type))
	{
		return expression_ternary(RHS_type, cond, LHS, RHS, loc);
	}

	semantic_error(bldr, op_loc, incompatible_cond_operands);
	return node_broken();
}

node build_initializer(builder *const bldr, node_vector *const exprs, const location l_loc, const location r_loc)
{
	const size_t actual_inits = node_vector_size(exprs);
	if (actual_inits == 0)
	{
		semantic_error(bldr, l_loc, empty_init);
		return node_broken();
	}

	const location loc = { l_loc.begin, r_loc.end };
	return expression_initializer(exprs, loc);
}

node build_constant_expression(builder *const bldr, node *const expr)
{
	if (expression_get_class(expr) != EXPR_LITERAL)
	{
		semantic_error(bldr, node_get_location(expr), expected_constant_expression);
		return node_broken();
	}

	return *expr;
}

node build_condition(builder *const bldr, node *const expr)
{
	if (expression_get_class(expr) == EXPR_ASSIGNMENT)
	{
		semantic_warning(bldr, node_get_location(expr), result_of_assignment_as_condition);
	}

	return *expr;
}

node build_empty_bound_expression(builder *const bldr, const location loc)
{
	return expression_empty_bound(&bldr->context, loc);
}


node build_member_declaration(builder *const bldr, const item_t type, const size_t name, const bool was_star
	, const bool was_const, node_vector *const bounds, const location loc)
{
	if (type_is_void(type))
	{
		semantic_error(bldr, loc, only_functions_may_have_type_VOID);
	}

	item_t member_type = was_star ? type_pointer(bldr->sx, type) : type;
	if (was_const)
	{
		member_type = type_const(bldr->sx, member_type);
	}
	const size_t bounds_amount = node_vector_size(bounds);
	for (size_t i = 0; i < bounds_amount; i++)
	{
		const node bound = node_vector_get(bounds, i);
		member_type = type_array(bldr->sx, member_type);
		if (!type_is_integer(bldr->sx, expression_get_type(&bound)))
		{
			semantic_error(bldr, node_get_location(&bound), array_size_must_be_int);
		}
	}

	return declaration_member(&bldr->context, member_type, name, bounds, loc);
}

node build_empty_struct_declaration(builder *const bldr, const size_t name, const location struct_loc)
{
	return declaration_struct(&bldr->context, name, struct_loc);
}

node build_struct_declaration(builder *const bldr, node *const declaration, node_vector *const members)
{
	const size_t members_amount = node_vector_size(members);
	vector types = vector_create(members_amount);
	vector names = vector_create(members_amount);

	for (size_t i = 0; i < members_amount; i++)
	{
		node member = node_vector_get(members, i);
		if (node_is_correct(&member))
		{
			vector_add(&types, declaration_member_get_type(&member));
			vector_add(&names, (item_t)declaration_member_get_name(&member));

			declaration_struct_add_declarator(declaration, &member);
		}
	}

	const size_t name = declaration_struct_get_name(declaration);
	const item_t type = type_structure(bldr->sx, &types, &names);
	const size_t id = ident_add(bldr->sx, name, 1000, type, 0);

	vector_clear(&types);
	vector_clear(&names);
	declaration_struct_set_type(declaration, type);

	const location struct_loc = node_get_location(declaration);
	if (id == SIZE_MAX - 1)
	{
		semantic_error(bldr, struct_loc, repeated_decl, repr_get_name(bldr->sx, name));
	}

	const node last_member = node_vector_get(members, members_amount - 1);
	const location loc = { struct_loc.begin, node_get_location(&last_member).end };
	return declaration_struct_set_location(declaration, loc);
}

node build_declarator(builder *const bldr, const item_t type, const size_t name
   , const bool was_star, const bool was_const, node_vector *const bounds, node *const initializer, const location ident_loc)
{
	if (type_is_void(type))
	{
		semantic_error(bldr, ident_loc, only_functions_may_have_type_VOID);
	}

	item_t variable_type = was_star ? type_pointer(bldr->sx, type) : type;
	if (was_const)
	{
		variable_type = type_const(bldr->sx, variable_type);
	}
	const size_t bounds_amount = node_vector_size(bounds);

	bool has_empty_bounds = false;
	for (size_t i = 0; i < bounds_amount; i++)
	{
		const node bound = node_vector_get(bounds, i);
		variable_type = type_array(bldr->sx, variable_type);
		if (!type_is_integer(bldr->sx, expression_get_type(&bound)))
		{
			semantic_error(bldr, node_get_location(&bound), array_size_must_be_int);
		}
		if (expression_get_class(&bound) == EXPR_EMPTY_BOUND)
		{
			has_empty_bounds = true;
		}
	}

	if (initializer)
	{
		check_assignment_operands(bldr, variable_type, initializer, /*is_declaration:*/ true);
	}
	else if (has_empty_bounds)
	{
		semantic_error(bldr, ident_loc, empty_bound_without_init);
	}

	// Magic numbers, maybe we need identifiers interface?
	const size_t id = ident_add(bldr->sx, name, 0, variable_type, 3);
	if (id == SIZE_MAX)
	{
		semantic_error(bldr, ident_loc, redefinition_of_main);
	}
	else if (id == SIZE_MAX - 1)
	{
		semantic_error(bldr, ident_loc, repeated_decl, repr_get_name(bldr->sx, name));
	}


	return declaration_variable(&bldr->context, id, bounds, initializer, ident_loc);
}

node build_empty_declaration(builder *const bldr)
{
	return statement_declaration(&bldr->context);
}

node build_declaration(builder *const bldr, node *const declaration, node_vector *const declarators, const location loc)
{
	const size_t amount = node_vector_size(declarators);
	if (amount == 0 && statement_declaration_get_size(declaration) == 0)
	{
		semantic_error(bldr, loc, declaration_does_not_declare_anything);
	}
	else
	{
		for (size_t i = 0; i < amount; i++)
		{
			node declarator = node_vector_get(declarators, i);
			if (node_is_correct(&declarator))
			{
				statement_declaration_add_declarator(declaration, &declarator);
			}
		}
	}

	return statement_declaration_set_location(declaration, loc);
}


node build_case_statement(builder *const bldr, node *const expr, node *const substmt, const location case_loc)
{
	if (!node_is_correct(expr) || !node_is_correct(substmt))
	{
		return node_broken();
	}

	if (!type_is_integer(bldr->sx, expression_get_type(expr)))
	{
		semantic_error(bldr, node_get_location(expr), case_expr_not_integer);
		return node_broken();
	}

	const location loc = { case_loc.begin, node_get_location(substmt).end };
	return statement_case(expr, substmt, loc);
}

node build_default_statement(builder *const bldr, node *const substmt, const location default_loc)
{
	if (!node_is_correct(substmt))
	{
		return node_broken();
	}

	(void)bldr;
	const location loc = { default_loc.begin, node_get_location(substmt).end };
	return statement_default(substmt, loc);
}

node build_compound_statement(builder *const bldr, node_vector *const stmts, location l_loc, location r_loc)
{
	if (stmts)
	{
		const size_t amount = node_vector_size(stmts);
		for (size_t i = 0; i < amount; i++)
		{
			node item = node_vector_get(stmts, i);
			if (!node_is_correct(&item))
			{
				return node_broken();
			}
		}
	}

	const location loc = { l_loc.begin, r_loc.end };
	return statement_compound(&bldr->context, stmts, loc);
}

node build_null_statement(builder *const bldr, const location semi_loc)
{
	return statement_null(&bldr->context, semi_loc);
}

node build_if_statement(builder *const bldr, node *const cond
	, node *const then_stmt, node *const else_stmt, const location if_loc)
{
	if (!node_is_correct(cond) || !node_is_correct(then_stmt) || (else_stmt && !node_is_correct(else_stmt)))
	{
		return node_broken();
	}

	if (!type_is_scalar(bldr->sx, expression_get_type(cond)))
	{
		semantic_error(bldr, node_get_location(cond), condition_must_be_scalar);
		return node_broken();
	}

	const location loc = { if_loc.begin, node_get_location(else_stmt ? else_stmt : then_stmt).end };
	return statement_if(cond, then_stmt, else_stmt, loc);
}

node build_switch_statement(builder *const bldr, node *const cond, node *const body, const location switch_loc)
{
	if (!node_is_correct(cond) || !node_is_correct(body))
	{
		return node_broken();
	}

	if (!type_is_integer(bldr->sx, expression_get_type(cond)))
	{
		semantic_error(bldr, node_get_location(cond), switch_expr_not_integer);
		return node_broken();
	}

	const location loc = { switch_loc.begin, node_get_location(body).end };
	return statement_switch(cond, body, loc);
}

node build_while_statement(builder *const bldr, node *const cond, node *const body, const location while_loc)
{
	if (!node_is_correct(cond) || !node_is_correct(body))
	{
		return node_broken();
	}

	if (!type_is_scalar(bldr->sx, expression_get_type(cond)))
	{
		semantic_error(bldr, node_get_location(cond), condition_must_be_scalar);
		return node_broken();
	}

	const location loc = { while_loc.begin, node_get_location(body).end };
	return statement_while(cond, body, loc);
}

node build_do_statement(builder *const bldr, node *const body, node *const cond, const location do_loc)
{
	if (!node_is_correct(body) || !node_is_correct(cond))
	{
		return node_broken();
	}

	if (!type_is_scalar(bldr->sx, expression_get_type(cond)))
	{
		semantic_error(bldr, node_get_location(cond), condition_must_be_scalar);
		return node_broken();
	}

	const location loc = { do_loc.begin, node_get_location(cond).end };
	return statement_do(body, cond, loc);
}

node build_for_statement(builder *const bldr, node *const init
	, node *const cond, node *const incr, node *const body, const location for_loc)
{
	if ((init && !node_is_correct(init)) || (cond && !node_is_correct(cond))
		|| (incr && !node_is_correct(incr)) || !node_is_correct(body))
	{
		return node_broken();
	}

	if (cond && !type_is_scalar(bldr->sx, expression_get_type(cond)))
	{
		semantic_error(bldr, node_get_location(cond), condition_must_be_scalar);
		return node_broken();
	}

	const location loc = { for_loc.begin, node_get_location(body).end };
	return statement_for(init, cond, incr, body, loc);
}

node build_continue_statement(builder *const bldr, const location continue_loc)
{
	return statement_continue(&bldr->context, continue_loc);
}

node build_break_statement(builder *const bldr, const location break_loc)
{
	return statement_break(&bldr->context, break_loc);
}

node build_return_statement(builder *const bldr, node *const expr, const location return_loc)
{
	location loc = return_loc;
	const item_t return_type = type_function_get_return_type(bldr->sx, bldr->func_type);
	if (expr != NULL)
	{
		if (!node_is_correct(expr))
		{
			return node_broken();
		}

		if (type_is_void(return_type))
		{
			semantic_error(bldr, node_get_location(expr), void_func_valued_return);
			return node_broken();
		}

		// TODO: void*?
		if (return_type != type_pointer(bldr->sx, TYPE_VOID))
		{
			check_assignment_operands(bldr, return_type, expr, /*is_declaration:*/ true);
		}

		loc.end = node_get_location(expr).end;
	}
	else
	{
		if (!type_is_void(return_type))
		{
			semantic_error(bldr, return_loc, nonvoid_func_void_return);
			return node_broken();
		}
	}

	return statement_return(&bldr->context, expr, loc);
}
