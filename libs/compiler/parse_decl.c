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

item_t parse_struct_or_union_specifier(parser *const parser);
item_t parse_struct_declaration_list(parser *const parser);


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
 *	@param	parser		Parser structure
 *
 *	@return	Standart type or index for modes table
 */
item_t parse_type_specifier(parser *const parser)
{
	parser->was_struct_with_arr = 0;
	switch (parser->next_token)
	{
		case kw_void:
			consume_token(parser);
			return mode_void;

		case kw_char:
			consume_token(parser);
			return mode_character;

		//case kw_short:
		case kw_int:
		case kw_long:
			consume_token(parser);
			return mode_integer;

		case kw_float:
		case kw_double:
			consume_token(parser);
			return mode_float;

		case identifier:
		{
			const size_t id = (size_t)repr_get_reference(parser->sx, parser->lexer->repr);
			consume_token(parser);

			if (ident_get_displ(parser->sx, id) < 1000)
			{
				parser_error(parser, ident_not_type);
				return mode_undefined;
			}

			parser->was_struct_with_arr = ident_get_displ(parser->sx, id) - 1000;
			return ident_get_mode(parser->sx, id);
		}

		//case kw_union:
		case kw_struct:
			consume_token(parser);
			return parse_struct_or_union_specifier(parser);

		default:
			parser_error(parser, not_decl);
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
 *	@param	parser		Parser structure
 *
 *	@return	@c mode_undefined or index for modes table
 */
item_t parse_struct_or_union_specifier(parser *const parser)
{
	switch (parser->next_token)
	{
		case l_brace:
			return parse_struct_declaration_list(parser);

		case identifier:
		{
			const size_t repr = parser->lexer->repr;
			consume_token(parser);

			if (parser->next_token == l_brace)
			{
				const item_t mode = parse_struct_declaration_list(parser);
				const size_t id = to_identab(parser, repr, 1000, mode);
				ident_set_displ(parser->sx, id, 1000 + parser->was_struct_with_arr);

				parser->flag_was_type_def = 1;

				return ident_get_mode(parser->sx, id);
			}
			else // if (parser->next_token != l_brace)
			{
				const size_t id = (size_t)repr_get_reference(parser->sx, repr);

				if (id == 1)
				{
					char buffer[MAXSTRINGL];
					repr_get_ident(parser->sx, repr, buffer);
					parser_error(parser, ident_is_not_declared, buffer);
					return mode_undefined;
				}

				// TODO: what if it was not a struct name?
				parser->was_struct_with_arr = ident_get_displ(parser->sx, id) - 1000;
				return ident_get_mode(parser->sx, id);
			}
		}

		default:
			parser_error(parser, wrong_struct);
			return mode_undefined;
	}
}

/**
 *	Parse array definition
 *
 *	direct-abstract-declarator:
 *		direct-abstract-declarator[opt] '[' constant-expression[opt] ']'
 *
 *	@param	parser		Parser structure
 *	@param	type		Type of variable in declaration
 *
 *	@return	Index for modes table
 */
item_t parse_array_definition(parser *const parser, item_t type)
{
	parser->arrdim = 0;
	parser->usual = 1;

	if (is_pointer(parser->sx, type))
	{
		parser_error(parser, pnt_before_array);
	}

	while (try_consume_token(parser, l_square))
	{
		parser->arrdim++;
		if (try_consume_token(parser, r_square))
		{
			if (parser->next_token == l_square)
			{
				// int a[][] = {{1,2,3}, {4,5,6}}; - нельзя;
				parser_error(parser, empty_init);
			}
			parser->usual = 0;
		}
		else
		{
			const item_t size_type = parse_constant_expression(parser);
			if (!is_int(size_type))
			{
				parser_error(parser, array_size_must_be_int);
			}

			if (!try_consume_token(parser, r_square))
			{
				parser_error(parser, wait_right_sq_br);
				skip_until(parser, r_square | comma | semicolon);
			}
		}
		type = to_modetab(parser->sx, mode_array, type);
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
 *	@param	parser		Parser structure
 *
 *	@return	@c mode_undefined or index for modes table
 */
item_t parse_struct_declaration_list(parser *const parser)
{
	const size_t ref_struct_begin = tree_size(parser->sx);
	tree_add(parser->sx, TStructbeg);

	item_t local_modetab[100];
	size_t local_md = 3;

	local_modetab[0] = mode_struct;
	tree_add(parser->sx, 0);	// Тут будет номер иниц процедуры

	size_t field_number = 0;
	size_t displ = 0;
	int wasarr = 0;

	consume_token(parser);
	if (parser->next_token == r_brace)
	{
		//	Что делать с пустой структурой?
		//	parser_error(parser, empty_struct);
		consume_token(parser);
		return mode_undefined;
	}

	do
	{
		item_t element_type = parse_type_specifier(parser);
		if (element_type == mode_void)
		{
			parser_error(parser, only_functions_may_have_type_VOID);
			element_type = mode_undefined;
		}

		item_t type = element_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			type = to_modetab(parser->sx, mode_pointer, element_type);
		}

		const size_t repr = parser->lexer->repr;
		if (parser->next_token == identifier)
		{
			consume_token(parser);
			if (parser->next_token == l_square)
			{
				tree_add(parser->sx, TDeclarr);
				const size_t ref_array_dim = tree_reserve(parser->sx);
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(parser, element_type);
				tree_set(parser->sx, ref_array_dim, parser->arrdim);

				tree_add(parser->sx, TDeclid);
				tree_add(parser->sx, (item_t)displ);
				tree_add(parser->sx, element_type);
				tree_add(parser->sx, parser->arrdim);

				// all - место в дереве, где будет общее количество выражений в инициализации,
				const size_t all = tree_reserve(parser->sx);		// для массивов - только признак (1) наличия инициализации
				tree_set(parser->sx, all, 0);						// all
				tree_add(parser->sx, parser->was_struct_with_arr);	// proc
				tree_add(parser->sx, parser->usual);				// usual
				tree_add(parser->sx, 1);							// Признак, что массив в структуре
				wasarr = 1;

				if (try_consume_token(parser, equal))
				{
					consume_token(parser);
					if (is_array(parser->sx, type))
					{
						parser->onlystrings = 2;
						tree_set(parser->sx, all, 1);
						if (!parser->usual)
						{
							tree_set(parser->sx, ref_array_dim, tree_get(parser->sx, ref_array_dim) - 1);
						}

						parse_initializer(parser, type);

						if (parser->onlystrings == 1)
						{
							tree_set(parser->sx, all + 2, parser->usual + 2);
						}
					}
				}
			}
		}
		else
		{
			parser_error(parser, wait_ident_after_semicolon_in_struct);
			skip_until(parser, semicolon | r_brace);
		}

		local_modetab[local_md++] = type;
		local_modetab[local_md++] = (item_t)repr;
		field_number++;
		displ += size_of(parser->sx, type);

		expect_and_consume_token(parser, semicolon, no_semicolon_in_struct);
	} while (!try_consume_token(parser, r_brace));

	if (wasarr)
	{
		tree_add(parser->sx, TStructend);
		tree_add(parser->sx, (item_t)ref_struct_begin);
		// TODO: сюда бы интерфейс для processes
		const size_t procd = vector_size(&parser->sx->processes);
		vector_increase(&parser->sx->processes, 1);

		parser->was_struct_with_arr = procd;
		tree_set(parser->sx, ref_struct_begin + 1, parser->was_struct_with_arr);
	}
	else
	{
		tree_set(parser->sx, ref_struct_begin, NOP);
		tree_set(parser->sx, ref_struct_begin + 1, NOP);
	}

	local_modetab[1] = (item_t)displ;
	local_modetab[2] = (item_t)field_number * 2;

	return (item_t)mode_add(parser->sx, local_modetab, local_md);
}

/**
 *	Parse struct initializer
 *
 *	@param	parser		Parser structure
 *	@param	type		Index for modes table
 */
void parse_struct_initializer(parser *const parser, const item_t type)
{
	if (parser->curr_token != l_brace)
	{
		parser_error(parser, struct_init_must_start_from_BEGIN);
		skip_until(parser, comma | semicolon);
		return;
	}

	const size_t expected_field_number = (size_t)(mode_get(parser->sx, type + 2) / 2);
	size_t actual_field_number = 0;
	size_t ref_next_field = type + 3;

	tree_add(parser->sx, TStructinit);
	tree_add(parser->sx, (item_t)expected_field_number);

	do
	{
		consume_token(parser);
		parse_initializer(parser, mode_get(parser->sx, ref_next_field));
		ref_next_field += 2;
		actual_field_number++;

		if (parser->next_token == r_brace)
		{
			break;
		}
		else if (!try_consume_token(parser, comma))
		{
			parser_error(parser, no_comma_in_init_list);
			skip_until(parser, comma | r_brace | semicolon);
		}
	} while (actual_field_number != expected_field_number && parser->next_token != semicolon);

	expect_and_consume_token(parser, r_brace, wait_end);
	parser->leftansttype = (int)type;
	tree_add(parser->sx, TExprend);
}

/**
 *	Parse array initializer
 *
 *	@param	parser		Parser structure
 *	@param	type		Index for modes table
 */
void parse_array_initializer(parser *const parser, const item_t type)
{
	if (parser->curr_token == string_literal)
	{
		if (parser->onlystrings == 0)
		{
			parser_error(parser, string_and_notstring);
		}
		if (parser->onlystrings == 2)
		{
			parser->onlystrings = 1;
		}
		parse_string_literal_expression(parser);
		tree_add(parser->sx, TExprend);
	}
	else
	{
		if (parser->curr_token != l_brace)
		{
			parser_error(parser, arr_init_must_start_from_BEGIN);
			skip_until(parser, comma | semicolon);
			return;
		}

		tree_add(parser->sx, TBeginit);
		const size_t ref_list_length = tree_reserve(parser->sx);
		size_t list_length = 0;

		do
		{
			list_length++;
			consume_token(parser);
			parse_initializer(parser, mode_get(parser->sx, type + 1));

			if (parser->next_token == r_brace)
			{
				break;
			}
			else if (!try_consume_token(parser, comma))
			{
				parser_error(parser, no_comma_in_init_list);
				skip_until(parser, comma | r_brace | semicolon);
			}
		} while (parser->next_token != semicolon);

		expect_and_consume_token(parser, r_brace, wait_end);
		tree_set(parser->sx, ref_list_length, (item_t)list_length);
		parser->leftansttype = (int)type;
		tree_add(parser->sx, TExprend);
	}
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
 *	@param	parser		Parser structure
 *	@param	type		Type of variable in declaration
 */
void parse_init_declarator(parser *const parser, item_t type)
{
	const size_t old_id = to_identab(parser, parser->lexer->repr, 0, type);
	size_t ref_array_dim = 0;

	parser->usual = 1;
	parser->arrdim = 0;
	const item_t element_type = type;

	if (parser->next_token == l_square)
	{
		tree_add(parser->sx, TDeclarr);
		ref_array_dim = tree_reserve(parser->sx);
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(parser, type);
		ident_set_mode(parser->sx, old_id, type);
		tree_set(parser->sx, ref_array_dim, parser->arrdim);
		if ((!parser->usual && parser->next_token != equal))
		{
			parser_error(parser, empty_bound_without_init);
		}
	}

	tree_add(parser->sx, TDeclid);
	tree_add(parser->sx, ident_get_displ(parser->sx, old_id));
	tree_add(parser->sx, element_type);
	tree_add(parser->sx, parser->arrdim);

	// all - место в дереве, где будет общее количество выражений в инициализации,
	const size_t all = tree_reserve(parser->sx);	// для массивов - только признак (1) наличия инициализации
	tree_set(parser->sx, all, 0);
	tree_add(parser->sx, is_pointer(parser->sx, type) ? 0 : parser->was_struct_with_arr);
	tree_add(parser->sx, parser->usual);
	tree_add(parser->sx, 0);	// Признак того, массив не в структуре

	if (try_consume_token(parser, equal))
	{
		consume_token(parser);
		tree_set(parser->sx, all, (item_t)size_of(parser->sx, type));
		if (is_array(parser->sx, type))
		{
			if (!parser->usual)
			{
				tree_set(parser->sx, ref_array_dim, tree_get(parser->sx, ref_array_dim) - 1);
			}

			parser->onlystrings = 2;
			parse_array_initializer(parser, type);
			if (parser->onlystrings == 1)
			{
				tree_set(parser->sx, all + 2, parser->usual + 2);
			}
		}
		else
		{
			parse_initializer(parser, type);
		}
	}
}

/**
 *	Parse function declarator
 *
 *	@return	@c mode_undefined or index for modes table
 */
item_t parse_function_declarator(parser *const parser, const int level, int func_d, const item_t return_type)
{
	item_t local_modetab[100];
	size_t local_md = 3;
	local_modetab[0] = mode_function;
	local_modetab[1] = return_type;
	local_modetab[2] = 0;
	size_t param_number = 0;

	if (try_consume_token(parser, r_paren))
	{
		parser->func_def = 0;
	}
	else
	{
		do
		{
			int flag_was_ident = 0;
			int maybe_fun = 0;	// м.б. параметр-ф-ция?
								// 0 - ничего не было,
								// 1 - была *,
								// 2 - была [
			item_t type = parse_type_specifier(parser);

			if (parser->next_token == star)
			{
				maybe_fun = 1;
				consume_token(parser);
				if (type == mode_void)
				{
					type = mode_void_pointer;
				}
				else
				{
					type = to_modetab(parser->sx, mode_pointer, type);
				}
			}

			// На 1 уровне это может быть определением функции или предописанием;
			// На остальных уровнях - только декларатором (без идентов)
			if (level)
			{
				if (try_consume_token(parser, identifier))
				{
					flag_was_ident = 1;
					func_add(parser->sx, (int)parser->lexer->repr);
				}
			}
			else if (parser->next_token == identifier)
			{
				parser_error(parser, ident_in_declarator);
				skip_until(parser, r_paren | semicolon);
				return mode_undefined;
			}

			if (type == mode_void && parser->next_token != l_paren)
			{
				parser_error(parser, par_type_void_with_nofun);
			}

			if (parser->next_token == l_square)
			{
				maybe_fun = 2;
				if (is_pointer(parser->sx, type) && flag_was_ident == 0)
				{
					parser_error(parser, aster_with_row);
				}

				while (try_consume_token(parser, l_square))
				{
					type = to_modetab(parser->sx, mode_array, type);
					if (!try_consume_token(parser, r_square))
					{
						parser_error(parser, wait_right_sq_br);
						skip_until(parser, r_square | comma | r_paren | semicolon);
					}
				}
			}

			if (try_consume_token(parser, l_paren))
			{
				expect_and_consume_token(parser, star, wrong_fun_as_param);
				if (parser->next_token == identifier)
				{
					if (level)
					{
						consume_token(parser);
						if (flag_was_ident == 0)
						{
							flag_was_ident = 2;
						}
						else
						{
							parser_error(parser, two_idents_for_1_declarer);
							return mode_undefined;
						}
						func_add(parser->sx, -((int)parser->lexer->repr));
					}
					else
					{
						parser_error(parser, ident_in_declarator);
						return mode_undefined;
					}
				}
				expect_and_consume_token(parser, r_paren, no_right_br_in_paramfun);
				expect_and_consume_token(parser, l_paren, wrong_fun_as_param);
				if (maybe_fun == 1)
				{
					parser_error(parser, aster_before_func);
					skip_until(parser, comma | r_paren | semicolon);
				}
				else if (maybe_fun == 2)
				{
					parser_error(parser, array_before_func);
					skip_until(parser, comma | r_paren | semicolon);
				}

				const int old_func_def = parser->func_def;
				type = parse_function_declarator(parser, 0, 2, type);
				parser->func_def = old_func_def;
			}
			if (func_d == 3)
			{
				func_d = flag_was_ident > 0 ? 1 : 2;
			}
			else if (func_d == 2 && flag_was_ident > 0)
			{
				parser_error(parser, wait_declarator);
				skip_until(parser, r_paren | semicolon);
				// На случай, если после этого заголовка стоит тело функции
				if (try_consume_token(parser, l_brace))
				{
					skip_until(parser, r_brace);
				}
				return mode_undefined;
			}
			else if (func_d == 1 && flag_was_ident == 0)
			{
				parser_error(parser, wait_definition);
				skip_until(parser, r_paren | semicolon);
				return mode_undefined;
			}

			param_number++;
			local_modetab[local_md++] = type;
		} while (try_consume_token(parser, comma));

		expect_and_consume_token(parser, r_paren, wrong_param_list);
		parser->func_def = func_d;
	}

	local_modetab[2] = (item_t)param_number;
	return (item_t)mode_add(parser->sx, local_modetab, local_md);
}

/**
 *	Parse function body
 *
 *	@param	parser		Parser structure
 *	@param	function_id	Function number
 */
void parse_function_body(parser *const parser, const size_t function_id)
{
	parser->function_mode = ident_get_mode(parser->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(parser->sx, function_id);
	const size_t param_number = (size_t)mode_get(parser->sx, parser->function_mode + 2);

	parser->pgotost = 0;
	parser->flag_was_return = 0;

	const item_t prev = ident_get_prev(parser->sx, function_id);
	if (prev > 1) // Был прототип
	{
		if (parser->function_mode != ident_get_mode(parser->sx, prev))
		{
			parser_error(parser, decl_and_def_have_diff_type);
			skip_until(parser, r_brace);
			return;
		}
		ident_set_displ(parser->sx, prev, (item_t)function_number);
	}

	const item_t old_displ = scope_func_enter(parser->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		const item_t type = mode_get(parser->sx, parser->function_mode + i + 3);
		const item_t repr = func_get(parser->sx, function_number + i + 1);

		to_identab(parser, llabs(repr), repr > 0 ? 0 : -1, type);
	}

	func_set(parser->sx, function_number, (item_t)tree_size(parser->sx));
	tree_add(parser->sx, TFuncdef);
	tree_add(parser->sx, (item_t)function_id);

	const size_t ref_maxdispl = tree_reserve(parser->sx);

	consume_token(parser);
	parse_compound_statement(parser, FUNCBODY);

	tree_set(parser->sx, tree_size(parser->sx), TReturnvoid);
	tree_add(parser->sx, TEnd);

	if (mode_get(parser->sx, parser->function_mode + 1) != mode_void && !parser->flag_was_return)
	{
		parser_error(parser, no_ret_in_func);
	}

	scope_func_exit(parser->sx, ref_maxdispl, old_displ);

	for (size_t i = 0; i < parser->pgotost; i += 2)
	{
		const size_t repr = ident_get_repr(parser->sx, parser->gotost[i]);
		const size_t line_number = llabs(parser->gotost[i + 1]);
		if (!ident_get_mode(parser->sx, parser->gotost[i]))
		{
			char buffer[MAXSTRINGL];
			repr_get_ident(parser->sx, repr, buffer);
			parser_error(parser, label_not_declared, line_number, buffer);
		}
	}
}

/**
 *	Parse function definition [C99 6.9.1]
 *
 *	function-definition:
 *		declarator declaration-list[opt] compound-statement
 *
 *	@param	parser		Parser structure
 *	@param	type		Return type of a function
 */
void parse_function_definition(parser *const parser, const item_t type)
{
	// TODO: сюда бы интерфейс для functions
	const size_t function_num = vector_size(&parser->sx->functions);
	vector_increase(&parser->sx->functions, 1);
	const size_t function_repr = parser->lexer->repr;
	consume_token(parser);
	const item_t function_mode = parse_function_declarator(parser, 1, 3, type);

	if (parser->func_def == 0 && parser->next_token == l_brace)
	{
		parser->func_def = 1;
	}
	else if (parser->func_def == 0)
	{
		parser->func_def = 2;
	}

	const size_t function_id = to_identab(parser, function_repr, (item_t)function_num, function_mode);

	if (parser->next_token == l_brace)
	{
		if (parser->func_def == 1)
		{
			parse_function_body(parser, function_id);
			return;
		}
		else
		{
			parser_error(parser, func_decl_req_params);
			skip_until(parser, r_brace);
			return;
		}
	}
	else if (parser->func_def == 1)
	{
		parser_error(parser, function_has_no_body);
		// На случай, если после неправильного декларатора стоит ';'
		try_consume_token(parser, semicolon);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_inner_declaration(parser *const parser)
{
	parser->flag_was_type_def = 0;
	item_t group_type = parse_type_specifier(parser);

	if (group_type == mode_void)
	{
		parser_error(parser, only_functions_may_have_type_VOID);
		group_type = mode_undefined;
	}
	else if (parser->flag_was_type_def && try_consume_token(parser, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			type = to_modetab(parser->sx, mode_pointer, group_type);
		}

		if (parser->next_token == identifier)
		{
			consume_token(parser);
			parse_init_declarator(parser, type);
		}
		else
		{
			parser_error(parser, after_type_must_be_ident);
			skip_until(parser, comma | semicolon);
		}
	} while (try_consume_token(parser, comma));

	expect_and_consume_token(parser, semicolon, expected_semi_after_decl);
}

void parse_external_declaration(parser *const parser)
{
	parser->flag_was_type_def = 0;
	parser->func_def = 3;
	const item_t group_type = parse_type_specifier(parser);

	if (parser->flag_was_type_def && try_consume_token(parser, semicolon))
	{
		return;
	}

	do
	{
		item_t type = group_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			if (group_type == mode_void)
			{
				type = mode_void_pointer;
			}
			else
			{
				type = to_modetab(parser->sx, mode_pointer, group_type);
			}
		}

		if (try_consume_token(parser, identifier))
		{
			if (parser->next_token == l_paren)
			{
				parse_function_definition(parser, type);
			}
			else if (group_type == mode_void)
			{
				parser_error(parser, only_functions_may_have_type_VOID);
			}
			else
			{
				parse_init_declarator(parser, type);
			}
		}
		else
		{
			parser_error(parser, after_type_must_be_ident);
			skip_until(parser, comma | semicolon);
		}

	} while (try_consume_token(parser, comma));

	if (parser->func_def != 1)
	{
		expect_and_consume_token(parser, semicolon, expected_semi_after_decl);
	}
}

void parse_initializer(parser *const parser, const item_t type)
{
	if (parser->curr_token != l_brace)
	{
		const item_t expr_type = parse_assignment_expression(parser);
		if (!is_undefined(expr_type) && !is_undefined(type))
		{
			if (is_int(type) && is_float(expr_type))
			{
				parser_error(parser, init_int_by_float);
			}
			else if (is_float(type) && is_int(expr_type))
			{
				insertwiden(parser);
			}
			else if (type != expr_type)
			{
				parser_error(parser, error_in_initialization);
			}
		}
	}
	else if (is_struct(parser->sx, type))
	{
		parse_struct_initializer(parser, type);
	}
	else if (is_array(parser->sx, type))
	{
		parse_array_initializer(parser, type);
	}
	else
	{
		parser_error(parser, wrong_init);
		skip_until(parser, semicolon);
	}
}
