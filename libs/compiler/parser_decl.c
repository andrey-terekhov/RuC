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

#include "parser.h"
#include <stdlib.h>


static item_t parse_struct_or_union_specifier(parser *const prs, node *const parent);
static item_t parse_struct_declaration_list(parser *const prs, node *const parent);
static void parse_array_initializer(parser *const prs, node *const parent, const item_t type);
static item_t parse_enum_specifier(parser *const prs, node *const parent);


/**
 *	Parse type specifier [C99 6.7.2]
 *
 *	type-specifier:
 *		'void'
 *		'char'
 *		'short'
 *		'int'
 *		'long'
 *		'float'
 *		'double'
 *		struct-or-union-specifier
 *		enum-specifier [TODO]
 *		typedef-name [TODO]
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Standard type or index of the types table
 */
static item_t parse_type_specifier(parser *const prs, node *const parent)
{
	prs->flag_array_in_struct = 0;
	switch (prs->token)
	{
		case TK_VOID:
			token_consume(prs);
			return TYPE_VOID;

		case TK_CHAR:
			token_consume(prs);
			return TYPE_CHARACTER;

		case TK_INT:
		case TK_LONG:
			token_consume(prs);
			return TYPE_INTEGER;

		case TK_FLOAT:
		case TK_DOUBLE:
			token_consume(prs);
			return TYPE_FLOATING;

		case TK_FILE:
			token_consume(prs);
			return TYPE_FILE;

		case TK_IDENTIFIER:
		{
			const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
			token_consume(prs);

			if (id == ITEM_MAX || ident_get_displ(prs->sx, (size_t)id) < 1000)
			{
				parser_error(prs, ident_not_type);
				return TYPE_UNDEFINED;
			}

			prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
			return ident_get_type(prs->sx, (size_t)id);
		}

		case TK_STRUCT:
			token_consume(prs);
			return parse_struct_or_union_specifier(prs, parent);

		case TK_ENUM:
		{
			token_consume(prs);
			return parse_enum_specifier(prs, parent);
		}

		case TK_TYPEDEF:
		{
			token_consume(prs);
			item_t type = parse_type_specifier(prs, parent);
			if (type_is_undefined(type))
			{
				token_skip_until(prs, TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}
			if (prs->token == TK_STAR)
			{
				token_consume(prs);
				if (type_is_void(type))
				{
					type = TYPE_VOID_POINTER;
				}
				else
				{
					type = type_pointer(prs->sx, type);
				}
			}
			if (prs->token == TK_IDENTIFIER)
			{
				const size_t repr = prs->lxr->repr;
				token_consume(prs);
				to_identab(prs, repr, 1000, type);
				prs->was_type_def = true;
				if (prs->token != TK_SEMICOLON)
				{
					parser_error(prs, expected_semi_after_decl);
					return TYPE_UNDEFINED;

				}
				return type;
			}
			else
			{
				parser_error(prs, typedef_requires_a_name);
				token_skip_until(prs, TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}
		}

		default:
			parser_error(prs, not_decl);
			return TYPE_UNDEFINED;
	}
}

/**
 *	Parse struct or union specifier [C99 6.7.2.1p1]
 *
 *	struct-or-union-specifier:
 *		struct-or-union identifier[opt] '{' struct-contents '}'
 *		struct-or-union identifier
 *
 *	struct-or-union:
 *		'struct'
 *		'union'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_struct_or_union_specifier(parser *const prs, node *const parent)
{
	switch (prs->token)
	{
		case TK_L_BRACE:
			return parse_struct_declaration_list(prs, parent);

		case TK_IDENTIFIER:
		{
			const size_t repr = prs->lxr->repr;
			token_consume(prs);

			if (prs->token == TK_L_BRACE)
			{
				const item_t type = parse_struct_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, type);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
				prs->was_type_def = true;

				return ident_get_type(prs->sx, id);
			}
			else // if (parser->next_token != l_brace)
			{
				const item_t id = repr_get_reference(prs->sx, repr);

				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
					return TYPE_UNDEFINED;
				}

				// TODO: what if it was not a struct name?
				prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
				return ident_get_type(prs->sx, (size_t)id);
			}
		}

		default:
			parser_error(prs, wrong_struct);
			return TYPE_UNDEFINED;
	}
}

/**
 *	Parse array definition
 *
 *	direct-abstract-declarator:
 *		direct-abstract-declarator[opt] '[' constant-expression[opt] ']'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Type of variable in declaration
 *
 *	@return	Index of the types table
 */
static item_t parse_array_definition(parser *const prs, node *const parent, item_t type)
{
	prs->array_dimensions = 0;
	prs->flag_empty_bounds = 1;

	if (type_is_pointer(prs->sx, type))
	{
		parser_error(prs, pnt_before_array);
	}

	while (token_try_consume(prs, TK_L_SQUARE))
	{
		prs->array_dimensions++;
		if (token_try_consume(prs, TK_R_SQUARE))
		{
			if (prs->token == TK_L_SQUARE)
			{
				// int a[][] = {{ 1, 2, 3 }, { 4, 5, 6 }};	// нельзя
				parser_error(prs, empty_init);
			}
			prs->flag_empty_bounds = 0;
		}
		else
		{
			const item_t size_type = parse_constant_expression(prs, parent);
			if (!type_is_integer(size_type))
			{
				parser_error(prs, array_size_must_be_int);
			}

			if (!token_try_consume(prs, TK_R_SQUARE))
			{
				parser_error(prs, wait_right_sq_br);
				token_skip_until(prs, TK_R_SQUARE | TK_COMMA | TK_SEMICOLON);
			}
		}
		type = type_array(prs->sx, type);
	}

	return type;
}

/**
 *	Parse struct declaration list [C99 6.7.2.1p2]
 *
 *	struct-declaration-list:
 *		struct-declaration
 *		struct-declaration-list struct-declaration
 *
 *	struct-declaration:
 *		type-specifier struct-declarator-list ';'
 *
 *	struct-declarator-list:
 *		declarator
 *		struct-declarator-list ',' declarator
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_struct_declaration_list(parser *const prs, node *const parent)
{
	token_consume(prs);
	if (token_try_consume(prs, TK_R_BRACE))
	{
		parser_error(prs, empty_struct);
		return TYPE_UNDEFINED;
	}

	item_t local_modetab[100];
	size_t local_md = 3;
	size_t fields = 0;
	size_t displ = 0;

	node nd;
	bool was_array = false;

	do
	{
		item_t element_type = parse_type_specifier(prs, parent);
		if (type_is_void(element_type))
		{
			parser_error(prs, only_functions_may_have_type_VOID);
			element_type = TYPE_UNDEFINED;
		}

		item_t type = element_type;
		if (token_try_consume(prs, TK_STAR))
		{
			type = type_pointer(prs->sx, element_type);
		}

		const size_t repr = prs->lxr->repr;
		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			if (prs->token == TK_L_SQUARE)
			{
				if (!was_array)
				{
					nd = node_add_child(parent, OP_DECL_STRUCT);
					node_add_arg(&nd, 0); // Тут будет номер инициализирующей процедуры
					was_array = true;
				}

				node nd_decl_arr = node_add_child(&nd, OP_DECL_ARR);
				node_add_arg(&nd_decl_arr, 0);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, &nd_decl_arr, element_type);
				node_set_arg(&nd_decl_arr, 0, prs->flag_empty_bounds
							 ? (item_t)prs->array_dimensions
							 : (item_t)prs->array_dimensions - 1);
				node nd_decl_id = node_add_child(&nd_decl_arr, OP_DECL_ID);
				node_add_arg(&nd_decl_id, (item_t)displ);
				node_add_arg(&nd_decl_id, element_type);
				node_add_arg(&nd_decl_id, (item_t)prs->array_dimensions);
				node_add_arg(&nd_decl_id, 0);
				node_add_arg(&nd_decl_id, prs->flag_array_in_struct);	// proc
				node_add_arg(&nd_decl_id, prs->flag_empty_bounds);		// usual
				node_add_arg(&nd_decl_id, 1);							// Признак, что массив в структуре

				if (token_try_consume(prs, TK_EQUAL))
				{
					if (type_is_array(prs->sx, type))
					{
						prs->flag_strings_only = 2;
						node_set_arg(&nd_decl_id, 3, 1);

						parse_initializer(prs, &nd_decl_arr, type);
						if (prs->flag_strings_only == 1)
						{
							node_set_arg(&nd_decl_id, 5, prs->flag_empty_bounds + 2);
						}
					}
				}
			}
		}
		else
		{
			parser_error(prs, wait_ident_after_semicolon_in_struct);
			token_skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		local_modetab[local_md++] = type;
		local_modetab[local_md++] = (item_t)repr;
		fields++;
		displ += type_size(prs->sx, type);

		token_expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_struct);
	} while (!token_try_consume(prs, TK_R_BRACE));

	if (was_array)
	{
		node nd_struct_end = node_add_child(&nd, OP_DECL_STRUCT_END);
		node_add_arg(&nd_struct_end, (item_t)prs->sx->procd);
		node_set_arg(&nd, 0, (item_t)prs->sx->procd);
		prs->flag_array_in_struct = (int)prs->sx->procd++;
	}

	local_modetab[0] = TYPE_STRUCTURE;
	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)fields * 2;

	return type_add(prs->sx, local_modetab, local_md);
}

/**
 *	Parse struct initializer
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Index of the types table
 */
static void parse_struct_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (!token_try_consume(prs, TK_L_BRACE))
	{
		parser_error(prs, struct_init_must_start_from_BEGIN);
		token_skip_until(prs, TK_COMMA | TK_SEMICOLON);
		return;
	}

	const size_t expected_fields = (size_t)(type_get(prs->sx, (size_t)type + 2) / 2);
	size_t actual_fields = 0;
	size_t ref_next_field = (size_t)type + 3;

	node nd_struct_init = node_add_child(parent, OP_STRUCT_INIT);
	node_add_arg(&nd_struct_init, (item_t)expected_fields);

	do
	{
		parse_initializer(prs, &nd_struct_init, type_get(prs->sx, ref_next_field));
		ref_next_field += 2;
		actual_fields++;

		if (prs->token == TK_R_BRACE)
		{
			break;
		}
		else if (!token_try_consume(prs, TK_COMMA))
		{
			parser_error(prs, no_comma_in_init_list);
			token_skip_until(prs, TK_COMMA | TK_R_BRACE | TK_SEMICOLON);
		}
	} while (actual_fields != expected_fields && prs->token != TK_SEMICOLON);

	token_expect_and_consume(prs, TK_R_BRACE, wait_end);

	// Это для продолжения выражений, если инициализатор был вызван не для объявления
	node_copy(&prs->nd, &nd_struct_init);
}

/**
 *	Parse array initializer
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Index of the types table
 */
static void parse_array_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (prs->token == TK_STRING)
	{
		if (prs->flag_strings_only == 0)
		{
			parser_error(prs, string_and_notstring);
		}
		else if (prs->flag_strings_only == 2)
		{
			prs->flag_strings_only = 1;
		}
		parse_string_literal(prs, parent);
		to_tree(prs, OP_EXPR_END);
		node_copy(&prs->nd, parent);
		return;
	}

	if (!token_try_consume(prs, TK_L_BRACE))
	{
		parser_error(prs, arr_init_must_start_from_BEGIN);
		token_skip_until(prs, TK_COMMA | TK_SEMICOLON);

		// Это для продолжения парсинга
		node_copy(&prs->nd, parent);
		return;
	}

	size_t list_length = 0;

	node nd_arr_init = node_add_child(parent, OP_ARRAY_INIT);
	node_add_arg(&nd_arr_init, 0);

	do
	{
		list_length++;
		parse_initializer(prs, &nd_arr_init, type_get(prs->sx, (size_t)type + 1));

		if (prs->token == TK_R_BRACE)
		{
			break;
		}
		else if (!token_try_consume(prs, TK_COMMA))
		{
			parser_error(prs, no_comma_in_init_list);
			token_skip_until(prs, TK_COMMA | TK_R_BRACE | TK_SEMICOLON);
		}
	} while (prs->token != TK_SEMICOLON);

	token_expect_and_consume(prs, TK_R_BRACE, wait_end);
	node_set_arg(&nd_arr_init, 0, (item_t)list_length);

	// Это для продолжения выражений, если инициализатор был вызван не для объявления
	node_copy(&prs->nd, &nd_arr_init);
}

/**
 *	Parse declarator with optional initializer
 *
 *	init-declarator:
 *		direct-declarator initializer[opt]
 *
 *	direct-declarator:
 *		identifier
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Type of variable in declaration
 */
static void parse_init_declarator(parser *const prs, node *const parent, item_t type)
{
	const size_t old_id = to_identab(prs, prs->lxr->repr, 0, type);

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;
	const item_t element_type = type;

	node nd_decl_arr;
	bool is_array = false;

	if (prs->token == TK_L_SQUARE)
	{
		nd_decl_arr = node_add_child(parent, OP_DECL_ARR);
		node_add_arg(&nd_decl_arr, 0); // Здесь будет размерность
		is_array = true;

		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, &nd_decl_arr, type);
		ident_set_type(prs->sx, old_id, type);
		node_set_arg(&nd_decl_arr, 0, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && prs->token != TK_EQUAL)
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	node nd = node_add_child(is_array ? &nd_decl_arr : parent, OP_DECL_ID);
	node_add_arg(&nd, ident_get_displ(prs->sx, old_id));
	node_add_arg(&nd, element_type);
	node_add_arg(&nd, (item_t)prs->array_dimensions);
	node_add_arg(&nd, 0);
	node_add_arg(&nd, type_is_pointer(prs->sx, type) ? 0 : prs->flag_array_in_struct);
	node_add_arg(&nd, prs->flag_empty_bounds);
	node_add_arg(&nd, 0);	// Признак того, что массив не в структуре

	if (token_try_consume(prs, TK_EQUAL))
	{
		node_set_arg(&nd, 3, (item_t)type_size(prs->sx, type));
		if (type_is_array(prs->sx, type))
		{
			if (!prs->flag_empty_bounds)
			{
				node_set_arg(&nd_decl_arr, 0, node_get_arg(&nd, 2) - 1);
			}

			prs->flag_strings_only = 2;
			parse_array_initializer(prs, &nd_decl_arr, type);
			node_add_child(&prs->nd, OP_EXPR_END);
			if (prs->flag_strings_only == 1)
			{
				node_set_arg(&nd, 5, prs->flag_empty_bounds + 2);
			}
		}
		else
		{
			parse_initializer(prs, &nd, type);
		}
	}
}

/**
 *	Parse function declarator
 *
 *	@param	prs			Parser structure
 *	@param	level		Level of declarator
 *	@param	func_def	@c 0 for function without arguments,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for others
 *	@param	return_type	Return type of declarated function
 *
 *	@return	Index of types table, @c type_undefined on failure
 */
static item_t parse_function_declarator(parser *const prs, const int level, int func_def, const item_t return_type)
{
	item_t local_modetab[100];
	size_t local_md = 3;
	size_t args = 0;

	if (token_try_consume(prs, TK_R_PAREN))
	{
		prs->func_def = 0;
	}
	else
	{
		do
		{
			int arg_func = 0;	/**< Тип возврата функции-параметра:
									 @c 0 - обычный тип,
									 @c 1 - была '*',
									 @c 2 - была '[' */
			item_t type = parse_type_specifier(prs, NULL);

			if (token_try_consume(prs, TK_STAR))
			{
				arg_func = 1;
				if (type_is_void(type))
				{
					type = TYPE_VOID_POINTER;
				}
				else
				{
					type = type_pointer(prs->sx, type);
				}
			}

			// На 1 уровне это может быть определением функции или предописанием;
			// На остальных уровнях - только декларатором (без идентов)
			bool was_ident = false;
			if (level)
			{
				if (token_try_consume(prs, TK_IDENTIFIER))
				{
					was_ident = true;
					func_add(prs->sx, (item_t)prs->lxr->repr);
				}
			}
			else if (prs->token == TK_IDENTIFIER)
			{
				parser_error(prs, ident_in_declarator);
				token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}

			if (type_is_void(type) && prs->token != TK_L_PAREN)
			{
				parser_error(prs, par_type_void_with_nofun);
			}

			if (prs->token == TK_L_SQUARE)
			{
				arg_func = 2;
				if (type_is_pointer(prs->sx, type) && !was_ident)
				{
					parser_error(prs, aster_with_row);
				}

				while (token_try_consume(prs, TK_L_SQUARE))
				{
					type = type_array(prs->sx, type);
					if (!token_try_consume(prs, TK_R_SQUARE))
					{
						parser_error(prs, wait_right_sq_br);
						token_skip_until(prs, TK_R_SQUARE | TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
					}
				}
			}

			if (token_try_consume(prs, TK_L_PAREN))
			{
				token_expect_and_consume(prs, TK_STAR, wrong_func_as_arg);
				if (prs->token == TK_IDENTIFIER)
				{
					if (level)
					{
						token_consume(prs);
						if (!was_ident)
						{
							was_ident = true;
						}
						else
						{
							parser_error(prs, two_idents_for_1_declarer);
							return TYPE_UNDEFINED;
						}
						func_add(prs->sx, -((item_t)prs->lxr->repr));
					}
					else
					{
						parser_error(prs, ident_in_declarator);
						return TYPE_UNDEFINED;
					}
				}

				token_expect_and_consume(prs, TK_R_PAREN, no_right_br_in_arg_func);
				token_expect_and_consume(prs, TK_L_PAREN, wrong_func_as_arg);
				if (arg_func == 1)
				{
					parser_error(prs, aster_before_func);
					token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
				}
				else if (arg_func == 2)
				{
					parser_error(prs, array_before_func);
					token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
				}

				const int old_func_def = prs->func_def;
				type = parse_function_declarator(prs, 0, 2, type);
				prs->func_def = old_func_def;
			}
			if (func_def == 3)
			{
				func_def = was_ident ? 1 : 2;
			}
			else if (func_def == 2 && was_ident)
			{
				parser_error(prs, wait_declarator);
				token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				// На случай, если после этого заголовка стоит тело функции
				if (token_try_consume(prs, TK_L_BRACE))
				{
					token_skip_until(prs, TK_R_BRACE);
				}
				return TYPE_UNDEFINED;
			}
			else if (func_def == 1 && !was_ident)
			{
				parser_error(prs, wait_definition);
				token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
				return TYPE_UNDEFINED;
			}

			args++;
			local_modetab[local_md++] = type;
		} while (token_try_consume(prs, TK_COMMA));

		token_expect_and_consume(prs, TK_R_PAREN, wrong_param_list);
		prs->func_def = func_def;
	}

	local_modetab[0] = TYPE_FUNCTION;
	local_modetab[1] = return_type;
	local_modetab[2] = (item_t)args;

	return type_add(prs->sx, local_modetab, local_md);
}

/**
 *	Parse function body
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	function_id	Function number
 */
static void parse_function_body(parser *const prs, node *const parent, const size_t function_id)
{
	prs->function_mode = (size_t)ident_get_type(prs->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(prs->sx, function_id);
	const size_t param_number = (size_t)type_get(prs->sx, prs->function_mode + 2);

	vector_resize(&prs->labels, 0);
	prs->was_return = 0;

	const item_t prev = ident_get_prev(prs->sx, function_id);
	if (prev > 1 && prev != ITEM_MAX - 1) // Был прототип
	{
		if (prs->function_mode != (size_t)ident_get_type(prs->sx, (size_t)prev))
		{
			parser_error(prs, decl_and_def_have_diff_type);
			token_skip_until(prs, TK_R_BRACE);
			return;
		}
		ident_set_displ(prs->sx, (size_t)prev, (item_t)function_number);
	}

	const item_t old_displ = scope_func_enter(prs->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		const item_t type = type_get(prs->sx, prs->function_mode + i + 3);
		const item_t repr = func_get(prs->sx, function_number + i + 1);

		to_identab(prs, (size_t)llabs(repr), repr > 0 ? 0 : -1, type);
	}

	node nd = node_add_child(parent, OP_FUNC_DEF);
	node_add_arg(&nd, (item_t)function_id);
	node_add_arg(&nd, 0); // for max_displ

	func_set(prs->sx, function_number, node_save(&nd)); // Ссылка на расположение в дереве

	parse_statement_compound(prs, &nd, FUNCBODY);

	if (type_get(prs->sx, prs->function_mode + 1) != TYPE_VOID && !prs->was_return)
	{
		parser_error(prs, no_ret_in_func);
	}

	const item_t max_displ = scope_func_exit(prs->sx, old_displ);
	node_set_arg(&nd, 1, max_displ);

	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		const size_t repr = (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i));
		const size_t line_number = (size_t)llabs(vector_get(&prs->labels, i + 1));
		if (!ident_get_type(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			parser_error(prs, label_not_declared, line_number, repr_get_name(prs->sx, repr));
		}
	}
}

/**
 *	Parse function definition [C99 6.9.1]
 *
 *	function-definition:
 *		declarator declaration-list[opt] compound-statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Return type of a function
 */
static void parse_function_definition(parser *const prs, node *const parent, const item_t type)
{
	const size_t function_num = func_reserve(prs->sx);
	const size_t function_repr = prs->lxr->repr;

	token_consume(prs);
	const item_t function_mode = parse_function_declarator(prs, 1, 3, type);

	if (prs->func_def == 0 && prs->token == TK_L_BRACE)
	{
		prs->func_def = 1;
	}
	else if (prs->func_def == 0)
	{
		prs->func_def = 2;
	}

	const size_t function_id = to_identab(prs, function_repr, (item_t)function_num, function_mode);

	if (prs->token == TK_L_BRACE)
	{
		if (prs->func_def == 1)
		{
			parse_function_body(prs, parent, function_id);
		}
		else
		{
			parser_error(prs, func_decl_req_params);
			token_skip_until(prs, TK_R_BRACE);
		}
	}
	else if (prs->func_def == 1)
	{
		parser_error(prs, function_has_no_body);
		// На тот случай, если после неправильного декларатора стоит ';'
		token_try_consume(prs, TK_SEMICOLON);
	}
}

static void parse_init_enum_field_declarator(parser *const prs, item_t type, item_t number)
{
	const size_t old_id = vector_size(&prs->sx->identifiers);
	const item_t ref = repr_get_reference(prs->sx, prs->lxr->repr);
	vector_add(&prs->sx->identifiers, ref == ITEM_MAX ? ITEM_MAX - 1 : ref);
	vector_increase(&prs->sx->identifiers, 3);

	ident_set_repr(prs->sx, old_id, (item_t)prs->lxr->repr);
	ident_set_type(prs->sx, old_id, type);

	repr_set_reference(prs->sx, prs->lxr->repr, (item_t)old_id);

	ident_set_displ(prs->sx, old_id, number);

	if (old_id == SIZE_MAX)
	{
		parser_error(prs, redefinition_of_main);
	}
	else if (old_id == SIZE_MAX - 1)
	{
		parser_error(prs, repeated_decl, repr_get_name(prs->sx, prs->lxr->repr));
	}
}

static item_t parse_enum_declaration_list(parser *const prs, node *const parent)
{
	token_consume(prs);
	if (token_try_consume(prs, TK_R_BRACE))
	{
		parser_error(prs, empty_enum);
		return TYPE_UNDEFINED;
	}

	size_t local_md = 3;
	item_t field_value = 0;
	item_t local_modetab[100];
	local_modetab[0] = TYPE_ENUM;

	do
	{

		if (!token_try_consume(prs, TK_IDENTIFIER))
		{
			parser_error(prs, wait_ident_after_comma_in_enum);
			token_skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		}

		if (prs->token == TK_EQUAL)
		{
			token_consume(prs);
			const size_t repr = prs->lxr->repr;
			field_value = parse_enum_field_expression(prs, parent);
			if (field_value == ITEM_MAX)
			{
				return TYPE_UNDEFINED;
			}
			prs->lxr->repr = repr;
			parse_init_enum_field_declarator(prs, TYPE_ENUM, field_value++);
		}
		else
		{
			parse_init_enum_field_declarator(prs, TYPE_ENUM, field_value++);
		}

		local_modetab[local_md++] = field_value - 1;
		local_modetab[local_md++] = (item_t)prs->lxr->repr;
		if (prs->token == TK_R_BRACE)
		{
			continue;
		}
		token_expect_and_consume(prs, TK_COMMA, no_comma_in_enum);
	} while (!token_try_consume(prs, TK_R_BRACE));

	local_modetab[2] = (item_t)(local_md - 3);
	local_modetab[1] = local_modetab[2] / 2;

	return type_add(prs->sx, local_modetab, local_md);
}

static item_t parse_enum_specifier(parser *const prs, node *const parent)
{
	switch (prs->token)
	{
		case TK_L_BRACE:
		{
			const item_t type = parse_enum_declaration_list(prs, parent);
			prs->was_type_def = true;
			return type;
		}
		case TK_IDENTIFIER:
		{
			const size_t repr = prs->lxr->repr;
			token_consume(prs);

			if (prs->token == TK_L_BRACE)
			{
				const item_t type = parse_enum_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, type);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
				prs->was_type_def = true;
				return ident_get_type(prs->sx, (size_t)id);
			}
			else // if (parser->next_token != l_brace)
			{
				const item_t id = repr_get_reference(prs->sx, repr);

				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
					return TYPE_UNDEFINED;
				}
				return ident_get_type(prs->sx, (size_t)id);
			}
		}

		default:
			parser_error(prs, wrong_struct);
			return TYPE_UNDEFINED;
	}
}

static bool check_int_initializer(const syntax *const sx, const item_t type, const item_t expr_type)
{
	return type_is_integer(type) && (type_is_enum(sx, expr_type) || type_is_enum_field(expr_type));
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_declaration_inner(parser *const prs, node *const parent)
{
	prs->was_type_def = 0;
	item_t group_type = parse_type_specifier(prs, parent);

	if (type_is_void(group_type))
	{
		parser_error(prs, only_functions_may_have_type_VOID);
		group_type = TYPE_UNDEFINED;
	}
	else if (prs->was_type_def && token_try_consume(prs, TK_SEMICOLON))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (token_try_consume(prs, TK_STAR))
		{
			type = type_pointer(prs->sx, group_type);
		}

		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			parse_init_declarator(prs, parent, type);
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			token_skip_until(prs, TK_COMMA | TK_SEMICOLON);
		}
	} while (token_try_consume(prs, TK_COMMA));

	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_decl);
}

void parse_declaration_external(parser *const prs, node *const root)
{
	prs->was_type_def = 0;
	prs->func_def = 3;
	const item_t group_type = parse_type_specifier(prs, root);

	if (prs->was_type_def && token_try_consume(prs, TK_SEMICOLON))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (prs->token == TK_STAR)
		{
			token_consume(prs);
			if (type_is_void(group_type))
			{
				type = TYPE_VOID_POINTER;
			}
			else
			{
				type = type_pointer(prs->sx, group_type);
			}
		}

		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			if (prs->token == TK_L_PAREN)
			{
				parse_function_definition(prs, root, type);
			}
			else if (type_is_void(group_type))
			{
				parser_error(prs, only_functions_may_have_type_VOID);
			}
			else
			{
				parse_init_declarator(prs, root, type);
			}
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			token_skip_until(prs, TK_COMMA | TK_SEMICOLON);
		}
	} while (token_try_consume(prs, TK_COMMA));

	if (prs->func_def != 1)
	{
		token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_decl);
	}
}

void parse_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (prs->token != TK_L_BRACE)
	{
		const item_t expr_type = parse_assignment_expression(prs, parent);
		if (!type_is_undefined(expr_type) && !type_is_undefined(type))
		{
			if (type_is_integer(type) && type_is_floating(expr_type))
			{
				parser_error(prs, init_int_by_float);
			}
			else if (type_is_floating(type) && type_is_integer(expr_type))
			{
				parse_insert_widen(prs);
			}
			else if (!check_enum_initializer(prs->sx, type, expr_type, prs->lxr->repr)
				&& !check_int_initializer(prs->sx, type, expr_type) && type != expr_type)
			{
				parser_error(prs, error_in_initialization);
			}
		}
	}
	else
	{
		parse_braced_initializer(prs, parent, type);
		// Инициализатор вызывается только для деклараций и аргументов, всегда нужен expr_end
		node_add_child(&prs->nd, OP_EXPR_END);
	}
}

void parse_braced_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (type_is_structure(prs->sx, type))
	{
		parse_struct_initializer(prs, parent, type);
	}
	else if (type_is_array(prs->sx, type))
	{
		parse_array_initializer(prs, parent, type);
	}
	else
	{
		node_copy(&prs->nd, parent);
		parser_error(prs, wrong_init);
		token_skip_until(prs, TK_COMMA | TK_SEMICOLON);
	}
}

bool check_enum_initializer(const syntax *const sx
		, const item_t type, const item_t expr_type, const size_t field_repr)
{
	if (!type_is_enum(sx, type) || !type_is_enum_field(expr_type))
	{
		return false;
	}

	const item_t fields = type_get(sx, (size_t)type + 1);
	for (item_t i = 0; i < fields; i++)
	{
		if (field_repr == (size_t)type_get(sx, (size_t)(type + 4 + 2 * i)))
		{
			return true;
		}
	}

	return false;
}
