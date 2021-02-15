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


item_t parse_struct_or_union_specifier(parser *const prs);
item_t parse_struct_declaration_list(parser *const prs);


/**
 *	Parse type specifier [C99 6.7.2]
 *
 *	type-specifier:
 *		void
 *		char
 *		short
 *		int
 *		long
 *		float
 *		double
 *		struct-or-union-specifier
 *		enum-specifier [TODO]
 *		typedef-name [TODO]
 *
 *	@param	prs			Parser structure
 *
 *	@return	Standard type or index of the modes table
 */
item_t parse_type_specifier(parser *const prs)
{
	prs->flag_array_in_struct = 0;
	switch (prs->next_token)
	{
		case kw_void:
			consume_token(prs);
			return mode_void;

		case kw_char:
			consume_token(prs);
			return mode_character;

		// case kw_short:
		case kw_int:
		case kw_long:
			consume_token(prs);
			return mode_integer;

		case kw_float:
		case kw_double:
			consume_token(prs);
			return mode_float;

		case identifier:
		{
			const size_t id = (size_t)repr_get_reference(prs->sx, prs->lxr->repr);
			consume_token(prs);

			if (ident_get_displ(prs->sx, id) < 1000)
			{
				parser_error(prs, ident_not_type);
				return mode_undefined;
			}

			prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, id) - 1000;
			return ident_get_mode(prs->sx, id);
		}

		// case kw_union:
		case kw_struct:
			consume_token(prs);
			return parse_struct_or_union_specifier(prs);

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
 *
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_struct_or_union_specifier(parser *const prs)
{
	switch (prs->next_token)
	{
		case l_brace:
			return parse_struct_declaration_list(prs);

		case identifier:
		{
			const size_t repr = prs->lxr->repr;
			consume_token(prs);

			if (prs->next_token == l_brace)
			{
				const item_t mode = parse_struct_declaration_list(prs);
				const size_t id = to_identab(prs, repr, 1000, mode);
				ident_set_displ(prs->sx, id, 1000 + prs->flag_array_in_struct);
				prs->flag_was_type_def = 1;

				return ident_get_mode(prs->sx, id);
			}
			else // if (parser->next_token != l_brace)
			{
				const size_t id = (size_t)repr_get_reference(prs->sx, repr);

				if (id == 1)
				{
					char buffer[MAXSTRINGL];
					repr_get_ident(prs->sx, repr, buffer);
					parser_error(prs, ident_is_not_declared, buffer);
					return mode_undefined;
				}

				// TODO: what if it was not a struct name?
				prs->flag_array_in_struct = (int)ident_get_displ(prs->sx, id) - 1000;
				return ident_get_mode(prs->sx, id);
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
 *	@param	type		Type of variable in declaration
 *
 *	@return	Index of the modes table
 */
item_t parse_array_definition(parser *const prs, item_t type)
{
	prs->array_dimensions = 0;
	prs->flag_empty_bounds = 1;

	if (mode_is_pointer(prs->sx, type))
	{
		parser_error(prs, pnt_before_array);
	}

	while (try_consume_token(prs, l_square))
	{
		prs->array_dimensions++;
		if (try_consume_token(prs, r_square))
		{
			if (prs->next_token == l_square)
			{
				// int a[][] = {{ 1, 2, 3 }, { 4, 5, 6 }};	// нельзя
				parser_error(prs, empty_init);
			}
			prs->flag_empty_bounds = 0;
		}
		else
		{
			const item_t size_type = parse_constant_expression(prs);
			if (!mode_is_int(size_type))
			{
				parser_error(prs, array_size_must_be_int);
			}

			if (!try_consume_token(prs, r_square))
			{
				parser_error(prs, wait_right_sq_br);
				skip_until(prs, r_square | comma | semicolon);
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
 *
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_struct_declaration_list(parser *const prs)
{
	const size_t ref_struct_begin = tree_size(prs->sx);
	tree_add(prs->sx, TStructbeg);
	tree_add(prs->sx, 0);	// Тут будет номер инициализирующей процедуры

	consume_token(prs);
	if (prs->next_token == r_brace)
	{
		// Что делать с пустой структурой?
		// parser_error(parser, empty_struct);
		consume_token(prs);
		return mode_undefined;
	}

	item_t local_modetab[100];
	size_t local_md = 3;
	size_t fields = 0;
	size_t displ = 0;
	int was_array = 0;

	do
	{
		item_t element_type = parse_type_specifier(prs);
		if (element_type == mode_void)
		{
			parser_error(prs, only_functions_may_have_type_VOID);
			element_type = mode_undefined;
		}

		item_t type = element_type;
		if (prs->next_token == star)
		{
			consume_token(prs);
			type = to_modetab(prs, mode_pointer, element_type);
		}

		const size_t repr = prs->lxr->repr;
		if (prs->next_token == identifier)
		{
			consume_token(prs);
			if (prs->next_token == l_square)
			{
				tree_add(prs->sx, TDeclarr);
				const size_t ref_array_dim = tree_reserve(prs->sx);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(prs, element_type);
				tree_set(prs->sx, ref_array_dim, (item_t)prs->array_dimensions);

				tree_add(prs->sx, TDeclid);
				tree_add(prs->sx, (item_t)displ);
				tree_add(prs->sx, element_type);
				tree_add(prs->sx, (item_t)prs->array_dimensions);

				// all - место в дереве, где будет общее количество выражений в инициализации,
				const size_t all = tree_reserve(prs->sx);		// для массивов - только признак (1) наличия инициализации
				tree_add(prs->sx, prs->flag_array_in_struct);	// proc
				tree_add(prs->sx, prs->flag_empty_bounds);	// usual
				tree_add(prs->sx, 1);							// Признак, что массив в структуре
				was_array = 1;

				if (try_consume_token(prs, equal))
				{
					consume_token(prs);
					if (mode_is_array(prs->sx, type))
					{
						prs->flag_strings_only = 2;
						tree_set(prs->sx, all, 1);
						if (!prs->flag_empty_bounds)
						{
							tree_set(prs->sx, ref_array_dim, tree_get(prs->sx, ref_array_dim) - 1);
						}

						parse_initializer(prs, type);

						if (prs->flag_strings_only == 1)
						{
							tree_set(prs->sx, all + 2, prs->flag_empty_bounds + 2);
						}
					}
				}
			}
		}
		else
		{
			parser_error(prs, wait_ident_after_semicolon_in_struct);
			skip_until(prs, semicolon | r_brace);
		}

		local_modetab[local_md++] = type;
		local_modetab[local_md++] = (item_t)repr;
		fields++;
		displ += size_of(prs->sx, type);

		expect_and_consume_token(prs, semicolon, no_semicolon_in_struct);
	} while (!try_consume_token(prs, r_brace));

	if (was_array)
	{
		tree_add(prs->sx, TStructend);
		// TODO: сюда бы интерфейс для processes
		const size_t procd = vector_size(&prs->sx->processes);
		vector_increase(&prs->sx->processes, 1);

		tree_add(prs->sx, procd);
		tree_set(prs->sx, ref_struct_begin + 1, procd);
		prs->flag_array_in_struct = procd;
	}
	else
	{
		tree_set(prs->sx, ref_struct_begin, NOP);
		tree_set(prs->sx, ref_struct_begin + 1, NOP);
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
 *	@param	type		Index of the modes table
 */
void parse_struct_initializer(parser *const prs, const item_t type)
{
	if (prs->curr_token != l_brace)
	{
		parser_error(prs, struct_init_must_start_from_BEGIN);
		skip_until(prs, comma | semicolon);
		return;
	}

	const size_t expected_fields = (size_t)(mode_get(prs->sx, (size_t)type + 2) / 2);
	size_t actual_fields = 0;
	size_t ref_next_field = (size_t)type + 3;

	tree_add(prs->sx, TStructinit);
	tree_add(prs->sx, (item_t)expected_fields);

	do
	{
		consume_token(prs);
		parse_initializer(prs, mode_get(prs->sx, ref_next_field));
		ref_next_field += 2;
		actual_fields++;

		if (prs->next_token == r_brace)
		{
			break;
		}
		else if (!try_consume_token(prs, comma))
		{
			parser_error(prs, no_comma_in_init_list);
			skip_until(prs, comma | r_brace | semicolon);
		}
	} while (actual_fields != expected_fields && prs->next_token != semicolon);

	expect_and_consume_token(prs, r_brace, wait_end);
	tree_add(prs->sx, TExprend);
}

/**
 *	Parse array initializer
 *
 *	@param	prs			Parser structure
 *	@param	type		Index of the modes table
 */
void parse_array_initializer(parser *const prs, const item_t type)
{
	if (prs->curr_token == string_literal)
	{
		if (prs->flag_strings_only == 0)
		{
			parser_error(prs, string_and_notstring);
		}
		else if (prs->flag_strings_only == 2)
		{
			prs->flag_strings_only = 1;
		}
		parse_string_literal_expression(prs);
		tree_add(prs->sx, TExprend);
		return;
	}

	if (prs->curr_token != l_brace)
	{
		parser_error(prs, arr_init_must_start_from_BEGIN);
		skip_until(prs, comma | semicolon);
		return;
	}

	tree_add(prs->sx, TBeginit);
	const size_t ref_list_length = tree_reserve(prs->sx);
	size_t list_length = 0;

	do
	{
		list_length++;
		consume_token(prs);
		parse_initializer(prs, mode_get(prs->sx, (size_t)type + 1));

		if (prs->next_token == r_brace)
		{
			break;
		}
		else if (!try_consume_token(prs, comma))
		{
			parser_error(prs, no_comma_in_init_list);
			skip_until(prs, comma | r_brace | semicolon);
		}
	} while (prs->next_token != semicolon);

	expect_and_consume_token(prs, r_brace, wait_end);
	tree_set(prs->sx, ref_list_length, (item_t)list_length);
	tree_add(prs->sx, TExprend);
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
 *	@param	type		Type of variable in declaration
 */
void parse_init_declarator(parser *const prs, item_t type)
{
	const size_t old_id = to_identab(prs, prs->lxr->repr, 0, type);
	size_t ref_array_dim = 0;

	prs->flag_empty_bounds = 1;
	prs->array_dimensions = 0;
	const item_t element_type = type;

	if (prs->next_token == l_square)
	{
		tree_add(prs->sx, TDeclarr);
		ref_array_dim = tree_reserve(prs->sx);
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(prs, type);
		ident_set_mode(prs->sx, old_id, type);
		tree_set(prs->sx, ref_array_dim, (item_t)prs->array_dimensions);
		if (!prs->flag_empty_bounds && prs->next_token != equal)
		{
			parser_error(prs, empty_bound_without_init);
		}
	}

	tree_add(prs->sx, TDeclid);
	tree_add(prs->sx, ident_get_displ(prs->sx, old_id));
	tree_add(prs->sx, element_type);
	tree_add(prs->sx, (item_t)prs->array_dimensions);

	// all - место в дереве, где будет общее количество выражений в инициализации,
	const size_t all = tree_reserve(prs->sx);	// для массивов - только признак наличия инициализации
	tree_set(prs->sx, all, 0);
	tree_add(prs->sx, mode_is_pointer(prs->sx, type) ? 0 : prs->flag_array_in_struct);
	tree_add(prs->sx, prs->flag_empty_bounds);
	tree_add(prs->sx, 0);	// Признак того, что массив не в структуре

	if (try_consume_token(prs, equal))
	{
		consume_token(prs);
		tree_set(prs->sx, all, (item_t)size_of(prs->sx, type));
		if (mode_is_array(prs->sx, type))
		{
			if (!prs->flag_empty_bounds)
			{
				tree_set(prs->sx, ref_array_dim, tree_get(prs->sx, ref_array_dim) - 1);
			}

			prs->flag_strings_only = 2;
			parse_array_initializer(prs, type);
			if (prs->flag_strings_only == 1)
			{
				tree_set(prs->sx, all + 2, prs->flag_empty_bounds + 2);
			}
		}
		else
		{
			parse_initializer(prs, type);
		}
	}
}

/**
 *	Parse function declarator
 *
 *	@return	Index of modes table, @c mode_undefined on failure
 */
item_t parse_function_declarator(parser *const prs, const int level, int func_d, const item_t return_type)
{
	item_t local_modetab[100];
	size_t local_md = 3;
	size_t args = 0;

	if (try_consume_token(prs, r_paren))
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
			item_t type = parse_type_specifier(prs);

			if (prs->next_token == star)
			{
				arg_func = 1;
				consume_token(prs);
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
				if (try_consume_token(prs, identifier))
				{
					flag_was_ident = 1;
					func_add(prs->sx, (item_t)prs->lxr->repr);
				}
			}
			else if (prs->next_token == identifier)
			{
				parser_error(prs, ident_in_declarator);
				skip_until(prs, r_paren | semicolon);
				return mode_undefined;
			}

			if (type == mode_void && prs->next_token != l_paren)
			{
				parser_error(prs, par_type_void_with_nofun);
			}

			if (prs->next_token == l_square)
			{
				arg_func = 2;
				if (mode_is_pointer(prs->sx, type) && flag_was_ident == 0)
				{
					parser_error(prs, aster_with_row);
				}

				while (try_consume_token(prs, l_square))
				{
					type = to_modetab(prs, mode_array, type);
					if (!try_consume_token(prs, r_square))
					{
						parser_error(prs, wait_right_sq_br);
						skip_until(prs, r_square | comma | r_paren | semicolon);
					}
				}
			}

			if (try_consume_token(prs, l_paren))
			{
				expect_and_consume_token(prs, star, wrong_fun_as_param);
				if (prs->next_token == identifier)
				{
					if (level)
					{
						consume_token(prs);
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

				expect_and_consume_token(prs, r_paren, no_right_br_in_paramfun);
				expect_and_consume_token(prs, l_paren, wrong_fun_as_param);
				if (arg_func == 1)
				{
					parser_error(prs, aster_before_func);
					skip_until(prs, comma | r_paren | semicolon);
				}
				else if (arg_func == 2)
				{
					parser_error(prs, array_before_func);
					skip_until(prs, comma | r_paren | semicolon);
				}

				const int old_func_def = prs->func_def;
				type = parse_function_declarator(prs, 0, 2, type);
				prs->func_def = old_func_def;
			}
			if (func_d == 3)
			{
				func_d = flag_was_ident > 0 ? 1 : 2;
			}
			else if (func_d == 2 && flag_was_ident > 0)
			{
				parser_error(prs, wait_declarator);
				skip_until(prs, r_paren | semicolon);
				// На случай, если после этого заголовка стоит тело функции
				if (try_consume_token(prs, l_brace))
				{
					skip_until(prs, r_brace);
				}
				return mode_undefined;
			}
			else if (func_d == 1 && flag_was_ident == 0)
			{
				parser_error(prs, wait_definition);
				skip_until(prs, r_paren | semicolon);
				return mode_undefined;
			}

			args++;
			local_modetab[local_md++] = type;
		} while (try_consume_token(prs, comma));

		expect_and_consume_token(prs, r_paren, wrong_param_list);
		prs->func_def = func_d;
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
 *	@param	function_id	Function number
 */
void parse_function_body(parser *const prs, const size_t function_id)
{
	prs->function_mode = (size_t)ident_get_mode(prs->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(prs->sx, function_id);
	const size_t param_number = (size_t)mode_get(prs->sx, prs->function_mode + 2);

	prs->pgotost = 0;
	prs->flag_was_return = 0;

	const item_t prev = ident_get_prev(prs->sx, function_id);
	if (prev > 1) // Был прототип
	{
		if (prs->function_mode != (size_t)ident_get_mode(prs->sx, (size_t)prev))
		{
			parser_error(prs, decl_and_def_have_diff_type);
			skip_until(prs, r_brace);
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

	func_set(prs->sx, function_number, (item_t)tree_size(prs->sx));
	tree_add(prs->sx, TFuncdef);
	tree_add(prs->sx, (item_t)function_id);

	const size_t ref_maxdispl = tree_reserve(prs->sx);

	consume_token(prs);
	parse_compound_statement(prs, FUNCBODY);

	vector_remove(&prs->sx->tree);
	tree_add(prs->sx, TReturnvoid);
	tree_add(prs->sx, TEnd);

	if (mode_get(prs->sx, prs->function_mode + 1) != mode_void && !prs->flag_was_return)
	{
		parser_error(prs, no_ret_in_func);
	}

	scope_func_exit(prs->sx, ref_maxdispl, old_displ);

	for (size_t i = 0; i < prs->pgotost; i += 2)
	{
		const size_t repr = (size_t)ident_get_repr(prs->sx, (size_t)prs->gotost[i]);
		const size_t line_number = (size_t)llabs(prs->gotost[i + 1]);
		if (!ident_get_mode(prs->sx, (size_t)prs->gotost[i]))
		{
			char buffer[MAXSTRINGL];
			repr_get_ident(prs->sx, repr, buffer);
			parser_error(prs, label_not_declared, line_number, buffer);
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
 *	@param	type		Return type of a function
 */
void parse_function_definition(parser *const prs, const item_t type)
{
	const size_t function_num = func_reserve(prs->sx);
	const size_t function_repr = prs->lxr->repr;

	consume_token(prs);
	const item_t function_mode = parse_function_declarator(prs, 1, 3, type);

	if (prs->func_def == 0 && prs->next_token == l_brace)
	{
		prs->func_def = 1;
	}
	else if (prs->func_def == 0)
	{
		prs->func_def = 2;
	}

	const size_t function_id = to_identab(prs, function_repr, (item_t)function_num, function_mode);

	if (prs->next_token == l_brace)
	{
		if (prs->func_def == 1)
		{
			parse_function_body(prs, function_id);
		}
		else
		{
			parser_error(prs, func_decl_req_params);
			skip_until(prs, r_brace);
		}
	}
	else if (prs->func_def == 1)
	{
		parser_error(prs, function_has_no_body);
		// На тот случай, если после неправильного декларатора стоит ';'
		try_consume_token(prs, semicolon);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_inner_declaration(parser *const prs)
{
	prs->flag_was_type_def = 0;
	item_t group_type = parse_type_specifier(prs);

	if (group_type == mode_void)
	{
		parser_error(prs, only_functions_may_have_type_VOID);
		group_type = mode_undefined;
	}
	else if (prs->flag_was_type_def && try_consume_token(prs, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (prs->next_token == star)
		{
			consume_token(prs);
			type = to_modetab(prs, mode_pointer, group_type);
		}

		if (prs->next_token == identifier)
		{
			consume_token(prs);
			parse_init_declarator(prs, type);
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			skip_until(prs, comma | semicolon);
		}
	} while (try_consume_token(prs, comma));

	expect_and_consume_token(prs, semicolon, expected_semi_after_decl);
}

void parse_external_declaration(parser *const prs)
{
	prs->flag_was_type_def = 0;
	prs->func_def = 3;
	const item_t group_type = parse_type_specifier(prs);

	if (prs->flag_was_type_def && try_consume_token(prs, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (prs->next_token == star)
		{
			consume_token(prs);
			if (group_type == mode_void)
			{
				type = mode_void_pointer;
			}
			else
			{
				type = to_modetab(prs, mode_pointer, group_type);
			}
		}

		if (try_consume_token(prs, identifier))
		{
			if (prs->next_token == l_paren)
			{
				parse_function_definition(prs, type);
			}
			else if (group_type == mode_void)
			{
				parser_error(prs, only_functions_may_have_type_VOID);
			}
			else
			{
				parse_init_declarator(prs, type);
			}
		}
		else
		{
			parser_error(prs, after_type_must_be_ident);
			skip_until(prs, comma | semicolon);
		}
	} while (try_consume_token(prs, comma));

	if (prs->func_def != 1)
	{
		expect_and_consume_token(prs, semicolon, expected_semi_after_decl);
	}
}

void parse_initializer(parser *const parser, const item_t type)
{
	if (parser->curr_token != l_brace)
	{
		const item_t expr_type = parse_assignment_expression(parser);
		if (!mode_is_undefined(expr_type) && !mode_is_undefined(type))
		{
			if (mode_is_int(type) && mode_is_float(expr_type))
			{
				parser_error(parser, init_int_by_float);
			}
			else if (mode_is_float(type) && mode_is_int(expr_type))
			{
				insert_widen(parser);
			}
			else if (type != expr_type)
			{
				parser_error(parser, error_in_initialization);
			}
		}
	}
	else if (mode_is_struct(parser->sx, type))
	{
		parse_struct_initializer(parser, type);
	}
	else if (mode_is_array(parser->sx, type))
	{
		parse_array_initializer(parser, type);
	}
	else
	{
		parser_error(parser, wrong_init);
		skip_until(parser, comma | semicolon);
	}
}
