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
			const item_t id = repr_get_reference(prs->sx, prs->lxr.repr);
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
				const size_t repr = prs->lxr.repr;
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
			const size_t repr = prs->lxr.repr;
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
			node_copy(&prs->sx->nd, parent);
			const node size = parse_constant_expression(prs);
			const item_t size_type = expression_get_type(&size);
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

	node nd = node_add_child(parent, OP_DECL_TYPE);
	node_add_arg(&nd, 0);	// Тут будет индекс для таблицы types

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

		const size_t repr = prs->lxr.repr;
		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			if (prs->token == TK_L_SQUARE)
			{
				node nd_decl = node_add_child(&nd, OP_DECL_VAR);
				node_add_arg(&nd_decl, 0);	// Вместо id подставим тип
				node_add_arg(&nd_decl, 0);	// Тут будет размерность
				node_add_arg(&nd_decl, 0);	// Тут будет флаг наличия инициализатора

				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, &nd_decl, element_type);

				node_set_arg(&nd_decl, 0, type);
				node_set_arg(&nd_decl, 1, (item_t)prs->array_dimensions);

				if (token_try_consume(prs, TK_EQUAL) && type_is_array(prs->sx, type))
				{
					node_set_arg(&nd_decl, 2, true);
					node_copy(&prs->sx->nd, &nd_decl);

					const node initializer = parse_initializer(prs, type);
					if (!node_is_correct(&initializer))
					{
						token_skip_until(prs, TK_SEMICOLON);
						continue;
					}

					check_assignment_operands(prs->sx, type, &initializer);
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

	local_modetab[0] = TYPE_STRUCTURE;
	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)fields * 2;

	const item_t result = type_add(prs->sx, local_modetab, local_md);
	node_set_arg(&nd, 0, result);
	return result;
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
	const size_t id = to_identab(prs, prs->lxr.repr, 0, type);

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;

	node nd = node_add_child(parent, OP_DECL_VAR);
	node_add_arg(&nd, (item_t)id);
	node_add_arg(&nd, 0);	// Тут будет размерность
	node_add_arg(&nd, 0);	// Тут будет флаг наличия инициализатора

	if (prs->token == TK_L_SQUARE)
	{
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, &nd, type);
		ident_set_type(prs->sx, id, type);
		node_set_arg(&nd, 1, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && prs->token != TK_EQUAL)
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	if (token_try_consume(prs, TK_EQUAL))
	{
		node_set_arg(&nd, 2, true);
		node_copy(&prs->sx->nd, &nd);

		const node initializer = parse_initializer(prs, type);
		if (!node_is_correct(&initializer))
		{
			token_skip_until(prs, TK_SEMICOLON);
			return;
		}

		check_assignment_operands(prs->sx, type, &initializer);
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
					func_add(prs->sx, (item_t)prs->lxr.repr);
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
						func_add(prs->sx, -((item_t)prs->lxr.repr));
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

	node nd = node_add_child(parent, OP_FUNC_DEF);
	node_add_arg(&nd, (item_t)function_id);
	node_add_arg(&nd, 0); // for max_displ

	const item_t old_displ = scope_func_enter(prs->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		item_t type = type_get(prs->sx, prs->function_mode + i + 3);
		const item_t repr = func_get(prs->sx, function_number + i + 1);

		const size_t id = to_identab(prs, (size_t)llabs(repr), repr > 0 ? 0 : -1, type);

		size_t dim = 0;
		while (type_is_array(prs->sx, type))
		{
			dim++;
			type = type_array_get_element_type(prs->sx, type);
		}

		node nd_param = node_add_child(&nd, OP_DECL_VAR);
		node_add_arg(&nd_param, (item_t)id);	// id
		node_add_arg(&nd_param, (item_t)dim);	// dim
		node_add_arg(&nd_param, false);			// has init
	}

	func_set(prs->sx, function_number, (item_t)node_save(&nd)); // Ссылка на расположение в дереве

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
	const size_t function_repr = prs->lxr.repr;

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
