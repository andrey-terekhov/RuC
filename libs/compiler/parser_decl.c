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


item_t parse_struct_or_union_specifier(parser *const prs, node *const parent);
item_t parse_struct_declaration_list(parser *const prs, node *const parent);


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
 *	@return	Standard type or index of the modes table
 */
item_t parse_type_specifier(parser *const prs, node *const parent)
{
	prs->flag_array_in_struct = 0;
	switch (prs->token)
	{
		case kw_void:
			token_consume(prs);
			return mode_void;

		case kw_char:
			token_consume(prs);
			return mode_character;

		// case kw_short:
		case kw_int:
		case kw_long:
			token_consume(prs);
			return mode_integer;

		case kw_float:
		case kw_double:
			token_consume(prs);
			return mode_float;

		case identifier:
		{
			const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
			token_consume(prs);

			if (id == ITEM_MAX || ident_get_displ(prs->sx, (size_t)id) < 1000)
			{
				parser_error(prs, ident_not_type);
				return mode_undefined;
			}

			prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
			return ident_get_mode(prs->sx, (size_t)id);
		}

		// case kw_union:
		case kw_struct:
			token_consume(prs);
			return parse_struct_or_union_specifier(prs, parent);

		default:
			parser_error(prs, not_decl);
			return mode_undefined;
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
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_struct_or_union_specifier(parser *const prs, node *const parent)
{
	switch (prs->token)
	{
		case l_brace:
			return parse_struct_declaration_list(prs, parent);

		case identifier:
		{
			const size_t repr = prs->lxr->repr;
			token_consume(prs);

			if (prs->token == l_brace)
			{
				const item_t mode = parse_struct_declaration_list(prs, parent);
				const size_t id = to_identab(prs, repr, 1000, mode);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
				prs->flag_was_type_def = 1;

				return ident_get_mode(prs->sx, id);
			}
			else // if (parser->next_token != l_brace)
			{
				const item_t id = repr_get_reference(prs->sx, repr);

				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
					return mode_undefined;
				}

				// TODO: what if it was not a struct name?
				prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, (size_t)id) - 1000;
				return ident_get_mode(prs->sx, (size_t)id);
			}
		}

		default:
			parser_error(prs, wrong_struct);
			return mode_undefined;
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
 *	@return	Index of the modes table
 */
item_t parse_array_definition(parser *const prs, node *const parent, item_t type)
{
	prs->array_dimensions = 0;
	prs->flag_empty_bounds = 1;

	if (mode_is_pointer(prs->sx, type))
	{
		parser_error(prs, pnt_before_array);
	}

	while (token_try_consume(prs, l_square))
	{
		prs->array_dimensions++;
		if (token_try_consume(prs, r_square))
		{
			if (prs->token == l_square)
			{
				// int a[][] = {{ 1, 2, 3 }, { 4, 5, 6 }};	// нельзя
				parser_error(prs, empty_init);
			}
			prs->flag_empty_bounds = 0;
		}
		else
		{
			const item_t size_type = parse_constant_expression(prs, parent);
			if (!mode_is_int(size_type))
			{
				parser_error(prs, array_size_must_be_int);
			}

			if (!token_try_consume(prs, r_square))
			{
				parser_error(prs, wait_right_sq_br);
				token_skip_until(prs, r_square | comma | semicolon);
			}
		}
		type = to_modetab(prs, mode_array, type);
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
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_struct_declaration_list(parser *const prs, node *const parent)
{
	token_consume(prs);
	if (token_try_consume(prs, r_brace))
	{
		parser_error(prs, empty_struct);
		return mode_undefined;
	}

	item_t local_modetab[100];
	size_t local_md = 3;
	size_t fields = 0;
	size_t displ = 0;

	node nd;
	int was_array = 0;

	do
	{
		item_t element_type = parse_type_specifier(prs, parent);
		if (element_type == mode_void)
		{
			parser_error(prs, only_functions_may_have_type_VOID);
			element_type = mode_undefined;
		}

		item_t type = element_type;
		if (token_try_consume(prs, star))
		{
			type = to_modetab(prs, mode_pointer, element_type);
		}

		const size_t repr = prs->lxr->repr;
		if (token_try_consume(prs, identifier))
		{
			if (prs->token == l_square)
			{
				if (!was_array)
				{
					nd = node_add_child(parent, TStructbeg);
					node_add_arg(&nd, 0); // Тут будет номер инициализирующей процедуры
					was_array = 1;
				}

				node nd_decl_arr = node_add_child(&nd, TDeclarr);
				node_add_arg(&nd_decl_arr, 0);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, &nd_decl_arr, element_type);
				node_set_arg(&nd_decl_arr, 0, prs->flag_empty_bounds
							 ? (item_t)prs->array_dimensions
							 : (item_t)prs->array_dimensions - 1);
				node nd_decl_id = node_add_child(&nd_decl_arr, TDeclid);
				node_add_arg(&nd_decl_id, (item_t)displ);
				node_add_arg(&nd_decl_id, element_type);
				node_add_arg(&nd_decl_id, (item_t)prs->array_dimensions);
				node_add_arg(&nd_decl_id, 0);
				node_add_arg(&nd_decl_id, prs->flag_array_in_struct);	// proc
				node_add_arg(&nd_decl_id, prs->flag_empty_bounds);		// usual
				node_add_arg(&nd_decl_id, 1);							// Признак, что массив в структуре

				if (token_try_consume(prs, equal))
				{
					if (mode_is_array(prs->sx, type))
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
			token_skip_until(prs, semicolon | r_brace);
		}

		local_modetab[local_md++] = type;
		local_modetab[local_md++] = (item_t)repr;
		fields++;
		displ += size_of(prs->sx, type);

		token_expect_and_consume(prs, semicolon, no_semicolon_in_struct);
	} while (!token_try_consume(prs, r_brace));

	if (was_array)
	{
		node nd_struct_end = node_add_child(parent, TStructend);
		node_add_arg(&nd_struct_end, (item_t)prs->sx->procd);
		node_set_arg(&nd, 0, (item_t)prs->sx->procd);
		prs->flag_array_in_struct = (int)prs->sx->procd++;
	}

	local_modetab[0] = mode_struct;
	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)fields * 2;

	return (item_t)mode_add(prs->sx, local_modetab, local_md);
}

/**
 *	Parse struct initializer
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Index of the modes table
 */
void parse_struct_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (!token_try_consume(prs, l_brace))
	{
		parser_error(prs, struct_init_must_start_from_BEGIN);
		token_skip_until(prs, comma | semicolon);
		return;
	}

	node_copy(&prs->nd, parent);
	const size_t expected_fields = (size_t)(mode_get(prs->sx, (size_t)type + 2) / 2);
	size_t actual_fields = 0;
	size_t ref_next_field = (size_t)type + 3;

	to_tree(prs, TStructinit);
	node_add_arg(&prs->nd, (item_t)expected_fields);

	do
	{
		parse_initializer(prs, &prs->nd, mode_get(prs->sx, ref_next_field));
		ref_next_field += 2;
		actual_fields++;

		if (prs->token == r_brace)
		{
			break;
		}
		else if (!token_try_consume(prs, comma))
		{
			parser_error(prs, no_comma_in_init_list);
			token_skip_until(prs, comma | r_brace | semicolon);
		}
	} while (actual_fields != expected_fields && prs->token != semicolon);

	token_expect_and_consume(prs, r_brace, wait_end);
	to_tree(prs, TExprend);
}

/**
 *	Parse array initializer
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	type		Index of the modes table
 */
void parse_array_initializer(parser *const prs, node *const parent, const item_t type)
{
	node_copy(&prs->nd, parent);
	if (prs->token == string_literal)
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
		to_tree(prs, TExprend);
		return;
	}

	if (!token_try_consume(prs, l_brace))
	{
		parser_error(prs, arr_init_must_start_from_BEGIN);
		token_skip_until(prs, comma | semicolon);
		return;
	}

	to_tree(prs, TBeginit);
	node_add_arg(&prs->nd, 0);
	size_t list_length = 0;

	node beginit;
	node_copy(&beginit, &prs->nd);

	do
	{
		list_length++;
		parse_initializer(prs, &prs->nd, mode_get(prs->sx, (size_t)type + 1));

		if (prs->token == r_brace)
		{
			break;
		}
		else if (!token_try_consume(prs, comma))
		{
			parser_error(prs, no_comma_in_init_list);
			token_skip_until(prs, comma | r_brace | semicolon);
		}
	} while (prs->token != semicolon);

	token_expect_and_consume(prs, r_brace, wait_end);
	node_set_arg(&beginit, 0, (item_t)list_length);
	to_tree(prs, TExprend);
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
void parse_init_declarator(parser *const prs, node *const parent, item_t type)
{
	const size_t old_id = to_identab(prs, prs->lxr->repr, 0, type);

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;
	const item_t element_type = type;
	
	node nd_decl_arr;
	int is_array = 0;

	if (prs->token == l_square)
	{
		nd_decl_arr = node_add_child(parent, TDeclarr);
		node_add_arg(&nd_decl_arr, 0); // Здесь будет размерность
		is_array = 1;

		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, &nd_decl_arr, type);
		ident_set_mode(prs->sx, old_id, type);
		node_set_arg(&nd_decl_arr, 0, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && prs->token != equal)
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	node nd = node_add_child(is_array ? &nd_decl_arr : parent, TDeclid);
	node_add_arg(&nd, ident_get_displ(prs->sx, old_id));
	node_add_arg(&nd, element_type);
	node_add_arg(&nd, (item_t)prs->array_dimensions);
	node_add_arg(&nd, 0);
	node_add_arg(&nd, mode_is_pointer(prs->sx, type) ? 0 : prs->flag_array_in_struct);
	node_add_arg(&nd, prs->flag_empty_bounds);
	node_add_arg(&nd, 0);	// Признак того, что массив не в структуре

	if (token_try_consume(prs, equal))
	{
		node_set_arg(&nd, 3, (item_t)size_of(prs->sx, type));
		if (mode_is_array(prs->sx, type))
		{
			if (!prs->flag_empty_bounds)
			{
				node_set_arg(&nd_decl_arr, 0, node_get_arg(&nd, 2) - 1);
			}

			prs->flag_strings_only = 2;
			parse_array_initializer(prs, &nd_decl_arr, type);
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
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_function_declarator(parser *const prs, const int level, int func_def, const item_t return_type)
{
	item_t local_modetab[100];
	size_t local_md = 3;
	size_t args = 0;

	if (token_try_consume(prs, r_paren))
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

			if (token_try_consume(prs, star))
			{
				arg_func = 1;
				if (type == mode_void)
				{
					type = mode_void_pointer;
				}
				else
				{
					type = to_modetab(prs, mode_pointer, type);
				}
			}

			// На 1 уровне это может быть определением функции или предописанием;
			// На остальных уровнях - только декларатором (без идентов)
			int flag_was_ident = 0;
			if (level)
			{
				if (token_try_consume(prs, identifier))
				{
					flag_was_ident = 1;
					func_add(prs->sx, (item_t)prs->lxr->repr);
				}
			}
			else if (prs->token == identifier)
			{
				parser_error(prs, ident_in_declarator);
				token_skip_until(prs, r_paren | semicolon);
				return mode_undefined;
			}

			if (type == mode_void && prs->token != l_paren)
			{
				parser_error(prs, par_type_void_with_nofun);
			}

			if (prs->token == l_square)
			{
				arg_func = 2;
				if (mode_is_pointer(prs->sx, type) && flag_was_ident == 0)
				{
					parser_error(prs, aster_with_row);
				}

				while (token_try_consume(prs, l_square))
				{
					type = to_modetab(prs, mode_array, type);
					if (!token_try_consume(prs, r_square))
					{
						parser_error(prs, wait_right_sq_br);
						token_skip_until(prs, r_square | comma | r_paren | semicolon);
					}
				}
			}

			if (token_try_consume(prs, l_paren))
			{
				token_expect_and_consume(prs, star, wrong_func_as_arg);
				if (prs->token == identifier)
				{
					if (level)
					{
						token_consume(prs);
						if (flag_was_ident == 0)
						{
							flag_was_ident = 2;
						}
						else
						{
							parser_error(prs, two_idents_for_1_declarer);
							return mode_undefined;
						}
						func_add(prs->sx, -((item_t)prs->lxr->repr));
					}
					else
					{
						parser_error(prs, ident_in_declarator);
						return mode_undefined;
					}
				}

				token_expect_and_consume(prs, r_paren, no_right_br_in_arg_func);
				token_expect_and_consume(prs, l_paren, wrong_func_as_arg);
				if (arg_func == 1)
				{
					parser_error(prs, aster_before_func);
					token_skip_until(prs, comma | r_paren | semicolon);
				}
				else if (arg_func == 2)
				{
					parser_error(prs, array_before_func);
					token_skip_until(prs, comma | r_paren | semicolon);
				}

				const int old_func_def = prs->func_def;
				type = parse_function_declarator(prs, 0, 2, type);
				prs->func_def = old_func_def;
			}
			if (func_def == 3)
			{
				func_def = flag_was_ident > 0 ? 1 : 2;
			}
			else if (func_def == 2 && flag_was_ident > 0)
			{
				parser_error(prs, wait_declarator);
				token_skip_until(prs, r_paren | semicolon);
				// На случай, если после этого заголовка стоит тело функции
				if (token_try_consume(prs, l_brace))
				{
					token_skip_until(prs, r_brace);
				}
				return mode_undefined;
			}
			else if (func_def == 1 && flag_was_ident == 0)
			{
				parser_error(prs, wait_definition);
				token_skip_until(prs, r_paren | semicolon);
				return mode_undefined;
			}

			args++;
			local_modetab[local_md++] = type;
		} while (token_try_consume(prs, comma));

		token_expect_and_consume(prs, r_paren, wrong_param_list);
		prs->func_def = func_def;
	}

	local_modetab[0] = mode_function;
	local_modetab[1] = return_type;
	local_modetab[2] = (item_t)args;

	return (item_t)mode_add(prs->sx, local_modetab, local_md);
}

/**
 *	Parse function body
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 *	@param	function_id	Function number
 */
void parse_function_body(parser *const prs, node *const parent, const size_t function_id)
{
	prs->function_mode = (size_t)ident_get_mode(prs->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(prs->sx, function_id);
	const size_t param_number = (size_t)mode_get(prs->sx, prs->function_mode + 2);

	vector_resize(&prs->labels, 0);
	prs->flag_was_return = 0;

	const item_t prev = ident_get_prev(prs->sx, function_id);
	if (prev > 1 && prev != ITEM_MAX - 1) // Был прототип
	{
		if (prs->function_mode != (size_t)ident_get_mode(prs->sx, (size_t)prev))
		{
			parser_error(prs, decl_and_def_have_diff_type);
			token_skip_until(prs, r_brace);
			return;
		}
		ident_set_displ(prs->sx, (size_t)prev, (item_t)function_number);
	}

	const item_t old_displ = scope_func_enter(prs->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		const item_t type = mode_get(prs->sx, prs->function_mode + i + 3);
		const item_t repr = func_get(prs->sx, function_number + i + 1);

		to_identab(prs, (size_t)llabs(repr), repr > 0 ? 0 : -1, type);
	}

	node nd = node_add_child(parent, TFuncdef);
	node_add_arg(&nd, (item_t)function_id);
	node_add_arg(&nd, 0); // for max_displ

	func_set(prs->sx, function_number, node_save(&nd)); // Ссылка на расположение в дереве

	parse_statement_compound(prs, &nd, FUNCBODY);

	if (mode_get(prs->sx, prs->function_mode + 1) != mode_void && !prs->flag_was_return)
	{
		parser_error(prs, no_ret_in_func);
	}

	const item_t max_displ = scope_func_exit(prs->sx, old_displ);
	node_set_arg(&nd, 1, max_displ);

	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		const size_t repr = (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i));
		const size_t line_number = (size_t)llabs(vector_get(&prs->labels, i + 1));
		if (!ident_get_mode(prs->sx, (size_t)vector_get(&prs->labels, i)))
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
void parse_function_definition(parser *const prs, node *const parent, const item_t type)
{
	const size_t function_num = func_reserve(prs->sx);
	const size_t function_repr = prs->lxr->repr;

	token_consume(prs);
	const item_t function_mode = parse_function_declarator(prs, 1, 3, type);

	if (prs->func_def == 0 && prs->token == l_brace)
	{
		prs->func_def = 1;
	}
	else if (prs->func_def == 0)
	{
		prs->func_def = 2;
	}

	const size_t function_id = to_identab(prs, function_repr, (item_t)function_num, function_mode);

	if (prs->token == l_brace)
	{
		if (prs->func_def == 1)
		{
			parse_function_body(prs, parent, function_id);
		}
		else
		{
			parser_error(prs, func_decl_req_params);
			token_skip_until(prs, r_brace);
		}
	}
	else if (prs->func_def == 1)
	{
		parser_error(prs, function_has_no_body);
		// На тот случай, если после неправильного декларатора стоит ';'
		token_try_consume(prs, semicolon);
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
	prs->flag_was_type_def = 0;
	item_t group_type = parse_type_specifier(prs, parent);

	if (group_type == mode_void)
	{
		parser_error(prs, only_functions_may_have_type_VOID);
		group_type = mode_undefined;
	}
	else if (prs->flag_was_type_def && token_try_consume(prs, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (token_try_consume(prs, star))
		{
			type = to_modetab(prs, mode_pointer, group_type);
		}

		if (token_try_consume(prs, identifier))
		{
			parse_init_declarator(prs, parent, type);
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			token_skip_until(prs, comma | semicolon);
		}
	} while (token_try_consume(prs, comma));

	token_expect_and_consume(prs, semicolon, expected_semi_after_decl);
}

void parse_declaration_external(parser *const prs, node *const root)
{
	prs->flag_was_type_def = 0;
	prs->func_def = 3;
	const item_t group_type = parse_type_specifier(prs, root);

	if (prs->flag_was_type_def && token_try_consume(prs, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (prs->token == star)
		{
			token_consume(prs);
			if (group_type == mode_void)
			{
				type = mode_void_pointer;
			}
			else
			{
				type = to_modetab(prs, mode_pointer, group_type);
			}
		}

		if (token_try_consume(prs, identifier))
		{
			if (prs->token == l_paren)
			{
				parse_function_definition(prs, root, type);
			}
			else if (group_type == mode_void)
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
			token_skip_until(prs, comma | semicolon);
		}
	} while (token_try_consume(prs, comma));

	if (prs->func_def != 1)
	{
		token_expect_and_consume(prs, semicolon, expected_semi_after_decl);
	}
}

void parse_initializer(parser *const prs, node *const parent, const item_t type)
{
	if (prs->token != l_brace)
	{
		const item_t expr_type = parse_assignment_expression(prs, parent);
		if (!mode_is_undefined(expr_type) && !mode_is_undefined(type))
		{
			if (mode_is_int(type) && mode_is_float(expr_type))
			{
				parser_error(prs, init_int_by_float);
			}
			else if (mode_is_float(type) && mode_is_int(expr_type))
			{
				parse_insert_widen(prs);
			}
			else if (type != expr_type)
			{
				parser_error(prs, error_in_initialization);
			}
		}
	}
	else if (mode_is_struct(prs->sx, type))
	{
		parse_struct_initializer(prs, parent, type);
	}
	else if (mode_is_array(prs->sx, type))
	{
		parse_array_initializer(prs, parent, type);
	}
	else
	{
		parser_error(prs, wrong_init);
		token_skip_until(prs, comma | semicolon);
	}
}
