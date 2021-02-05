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

size_t parse_struct_or_union_specifier(parser *const parser);
size_t parse_struct_declaration_list(parser *const parser);


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
int parse_type_specifier(parser *const parser)
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
			const size_t id = repr_get_reference(parser->sx, parser->lexer->repr);
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
			return (int)parse_struct_or_union_specifier(parser);

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
size_t parse_struct_or_union_specifier(parser *const parser)
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
				const int mode = parse_struct_declaration_list(parser);
				const size_t id = toidentab(parser, repr, 1000, mode);
				ident_set_displ(parser->sx, id, 1000 + parser->was_struct_with_arr);

				parser->flag_was_type_def = 1;

				return ident_get_mode(parser->sx, id);
			}
			else // if (parser->next_token != l_brace)
			{
				const size_t id = repr_get_reference(parser->sx, repr);

				if (id == 1)
				{
					parser_error(parser, ident_is_not_declared, parser->sx->reprtab, parser->lexer->repr);
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
int parse_array_definition(parser *const parser, int type)
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
			const int size_type = parse_constant_expression(parser);
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
		type = newdecl(parser->sx, mode_array, type);
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
size_t parse_struct_declaration_list(parser *const parser)
{
	const size_t ref_struct_begin = parser->sx->tc;
	totree(parser, TStructbeg);

	int local_modetab[100];
	size_t local_md = 3;

	local_modetab[0] = mode_struct;
	totree(parser, 0);	// Тут будет номер иниц процедуры

	size_t field_number = 0;
	int displ = 0;
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
		int element_type = parse_type_specifier(parser);
		if (element_type == mode_void)
		{
			parser_error(parser, only_functions_may_have_type_VOID);
			element_type = mode_undefined;
		}

		int type = element_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			type = newdecl(parser->sx, mode_pointer, element_type);
		}

		const size_t repr = parser->lexer->repr;
		size_t ref_array_dim = 0;
		if (parser->next_token == identifier)
		{
			consume_token(parser);
			if (parser->next_token == l_square)
			{
				totree(parser, TDeclarr);
				const size_t adN = parser->sx->tc++;
				// Меняем тип (увеличиваем размерность массива)
				type = parse_array_definition(parser, element_type);
				parser->sx->tree[adN] = parser->arrdim;

				totree(parser, TDeclid);
				totree(parser, displ);
				totree(parser, element_type);
				totree(parser, parser->arrdim);										// N
				// all - место в дереве, где будет общее количество выражений в инициализации,
				const size_t all = parser->sx->tc++;	// для массивов - только признак (1) наличия инициализации
				parser->sx->tree[all] = 0;											// all
				parser->sx->tree[parser->sx->tc++] = parser->was_struct_with_arr;	// proc
				totree(parser, parser->usual);										// usual
				totree(parser, 1); // Признак, что массив в структуре
				wasarr = 1;

				if (try_consume_token(parser, equal))
				{
					consume_token(parser);
					if (is_array(parser->sx, type))
					{
						parser->onlystrings = 2;
						parser->sx->tree[all] = 1;
						if (!parser->usual)
						{
							parser->sx->tree[ref_array_dim]--;
						}
						parse_initializer(parser, type);
						if (parser->onlystrings == 1)
						{
							parser->sx->tree[all + 2] = parser->usual + 2;
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
		local_modetab[local_md++] = (int)repr;
		field_number++;
		displ += size_of(parser->sx, type);

		expect_and_consume_token(parser, semicolon, no_semicolon_in_struct);
	} while (!try_consume_token(parser, r_brace));

	if (wasarr)
	{
		totree(parser, TStructend);
		totree(parser, (int)ref_struct_begin);
		parser->sx->tree[ref_struct_begin + 1] = parser->was_struct_with_arr = parser->sx->procd++;
	}
	else
	{
		parser->sx->tree[ref_struct_begin] = NOP;
		parser->sx->tree[ref_struct_begin + 1] = NOP;
	}

	local_modetab[1] = displ;
	local_modetab[2] = (int)field_number * 2;

	return mode_add(parser->sx, local_modetab, local_md);
}

/**
 *	Parse struct initializer
 *
 *	@param	parser		Parser structure
 *	@param	type		Index for modes table
 */
void parse_struct_initializer(parser *const parser, const int type)
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

	totree(parser, TStructinit);
	totree(parser, expected_field_number);

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
	parser->leftansttype = type;
	totree(parser, TExprend);
}

/**
 *	Parse array initializer
 *
 *	@param	parser		Parser structure
 *	@param	type		Index for modes table
 */
void parse_array_initializer(parser *const parser, const int type)
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
		totree(parser, TExprend);
	}
	else
	{
		if (parser->curr_token != l_brace)
		{
			parser_error(parser, arr_init_must_start_from_BEGIN);
			skip_until(parser, comma | semicolon);
			return;
		}

		totree(parser, TBeginit);
		const size_t ref_list_length = parser->sx->tc++;
		int list_length = 0;

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
		parser->sx->tree[ref_list_length] = list_length;
		totree(parser, TExprend);
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
void parse_init_declarator(parser *const parser, int type)
{
	const size_t old_id = toidentab(parser, parser->lexer->repr, 0, type);
	size_t ref_array_dim = 0;

	parser->usual = 1;
	parser->arrdim = 0;
	const int element_type = type;

	if (parser->next_token == l_square)
	{
		totree(parser, TDeclarr);
		ref_array_dim = parser->sx->tc++;
		// Меняем тип (увеличиваем размерность массива)
		type = parse_array_definition(parser, type);
		ident_set_mode(parser->sx, old_id, type);
		parser->sx->tree[ref_array_dim] = parser->arrdim;
		if ((!parser->usual && parser->next_token != equal))
		{
			parser_error(parser, empty_bound_without_init);
		}
	}

	totree(parser, TDeclid);
	totree(parser, ident_get_displ(parser->sx, old_id));							// displ
	totree(parser, element_type);													// elem_type
	totree(parser, parser->arrdim);													// N
	// all - место в дереве, где будет общее количество выражений в инициализации,
	size_t all = parser->sx->tc++;	// для массивов - только признак (1) наличия инициализации
	parser->sx->tree[all] = 0;
	totree(parser, is_pointer(parser->sx, type) ? 0 : parser->was_struct_with_arr);	// proc
	totree(parser, parser->usual);													// usual
	totree(parser, 0);	// Признак того, массив не в структуре

	if (try_consume_token(parser, equal))
	{
		consume_token(parser);
		parser->sx->tree[all] = size_of(parser->sx, type);
		if (is_array(parser->sx, type))
		{
			if (!parser->usual)
			{
				parser->sx->tree[ref_array_dim]--;
			}

			parser->onlystrings = 2;
			parse_array_initializer(parser, type);
			if (parser->onlystrings == 1)
			{
				parser->sx->tree[all + 2] = parser->usual + 2;
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
size_t parse_function_declarator(parser *const parser, const int level, int func_d, const int return_type)
{
	int local_modetab[100];
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
			int type = parse_type_specifier(parser);

			if (parser->next_token == star)
			{
				consume_token(parser);
				if (type == mode_void)
				{
					type = mode_void_pointer;
				}
				else
				{
					type = newdecl(parser->sx, mode_pointer, type);
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
				if (is_pointer(parser->sx, type) && flag_was_ident == 0)
				{
					parser_error(parser, aster_with_row);
				}

				while (try_consume_token(parser, l_square))
				{
					type = newdecl(parser->sx, mode_array, type);
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
				mustbe(parser, RIGHTBR, no_right_br_in_paramfun);
				mustbe(parser, LEFTBR, wrong_fun_as_param);
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
				type = (int)parse_function_declarator(parser, 0, 2, type);
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

	local_modetab[2] = param_number;
	return mode_add(parser->sx, local_modetab, local_md);
}

/**
 *	Parse function body
 *
 *	@param	parser		Parser structure
 *	@param	function_id	Function number
 */
void parse_function_body(parser *const parser, const size_t function_id)
{
	parser->function_type = ident_get_mode(parser->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(parser->sx, function_id);
	const size_t param_number = (size_t)mode_get(parser->sx, parser->function_type + 2);

	parser->pgotost = 0;
	parser->flag_was_return = 0;

	const int pred = parser->sx->identab[function_id];
	if (pred > 1) // Был прототип
	{
		if (parser->function_type != ident_get_mode(parser->sx, pred))
		{
			parser_error(parser, decl_and_def_have_diff_type);
			skip_until(parser, r_brace);
			return;
		}
		ident_set_displ(parser->sx, pred, function_number);
	}

	const int old_displ = scope_func_enter(parser->sx);

	for (size_t i = 0; i < param_number; i++)
	{
		const int type = mode_get(parser->sx, parser->function_type + i + 3);
		const int repr = func_get(parser->sx, function_number + i + 1);

		toidentab(parser, abs(repr), repr > 0 ? 0 : -1, type);
	}

	func_set(parser->sx, function_number, parser->sx->tc);
	totree(parser, TFuncdef);
	totree(parser, function_id);

	const size_t ref_maxdispl = parser->sx->tc++;

	consume_token(parser);
	parse_compound_statement(parser, FUNCBODY);

	parser->sx->tc--;
	totree(parser, TReturnvoid);
	totree(parser, TEnd);

	if (mode_get(parser->sx, parser->function_type + 1) != mode_void && !parser->flag_was_return)
	{
		parser_error(parser, no_ret_in_func);
	}

	scope_func_exit(parser->sx, ref_maxdispl, old_displ);

	for (int i = 0; i < parser->pgotost - 1; i += 2)
	{
		parser->lexer->repr = (size_t)parser->sx->identab[parser->gotost[i] + 1];
		parser->sx->hash = parser->gotost[i + 1];
		if (parser->sx->hash < 0)
		{
			parser->sx->hash = -parser->sx->hash;
		}
		if (!parser->sx->identab[parser->gotost[i] + 2])
		{
			parser_error(parser, label_not_declared, parser->sx->hash, parser->sx->reprtab, parser->lexer->repr);
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
void parse_function_definition(parser *const parser, const int type)
{
	const size_t function_num = parser->sx->funcnum++;
	const size_t function_repr = parser->lexer->repr;
	consume_token(parser);
	const int function_mode = (int)parse_function_declarator(parser, 1, 3, type);

	if (parser->func_def == 0 && parser->next_token == l_brace)
	{
		parser->func_def = 1;
	}
	else if (parser->func_def == 0)
	{
		parser->func_def = 2;
	}

	const size_t function_id = toidentab(parser, function_repr, (int)function_num, function_mode);

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
	int group_type = parse_type_specifier(parser);

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
		int type = group_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			type = newdecl(parser->sx, mode_pointer, group_type);
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
	int group_type = parse_type_specifier(parser);

	if (parser->flag_was_type_def && try_consume_token(parser, semicolon))
	{
		return;
	}

	do
	{
		int type = group_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			if (group_type == mode_void)
			{
				type = mode_void_pointer;
			}
			else
			{
				type = newdecl(parser->sx, mode_pointer, group_type);
			}
		}

		if (try_consume_token(parser, identifier))
		{
			if (parser->next_token == l_paren)
			{
				parse_function_definition(parser, type);
			}
			else if (group_type == LVOID)
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

void parse_initializer(parser *const parser, const int type)
{
	if (parser->curr_token != l_brace)
	{
		const int expr_type = parse_assignment_expression(parser);
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
	}
}
