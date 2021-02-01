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

int parse_type_specifier(parser *const parser);
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
 *	@param	parser	Parser structure
 *
 *	@return	Standart type or index for modes table
 */
int parse_type_specifier(parser *const parser)
{
	parser->was_struct_with_arr = 0;
	switch (parser->curr_token)
	{
		case kw_void:
			return mode_void;

		case kw_char:
			return mode_character;

		//case kw_short:
		case kw_int:
		case kw_long:
			return mode_integer;

		case kw_float:
		case kw_double:
			return mode_float;

		case identifier:
		{
			const size_t id = repr_get_reference(parser->sx, parser->lxr->repr);

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
			const size_t repr = (size_t)parser->lxr->repr;
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
					parser_error(parser, ident_is_not_declared);
					return mode_undefined;
				}

				// TODO: what if it was not a type specifier?
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
	const size_t struct_begin_ref = parser->sx->tc;
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
		// Что делать с пустой структурой?
		//parser_error(parser, empty_struct);
		consume_token(parser);
		return mode_undefined;
	}

	do
	{
		consume_token(parser);
		
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

		const size_t repr = parser->lxr->repr;
		if (parser->next_token == identifier)
		{
			consume_token(parser);
			if (parser->next_token == l_square)
			{
				totree(parser, TDeclarr);
				const size_t adN = parser->sx->tc++;
				type = arrdef(parser, element_type);	// Меняем тип (увеличиваем размерность массива)
				parser->sx->tree[adN] = parser->arrdim;

				totree(parser, TDeclid);
				totree(parser, displ);
				totree(parser, element_type);
				totree(parser, parser->arrdim);							 // N
				const size_t all = parser->sx->tc++;
				parser->sx->tree[all] = 0;						 // all
				parser->sx->tree[parser->sx->tc++] = parser->was_struct_with_arr; // proc
				totree(parser, parser->usual);							 // context->usual
				totree(parser, 1); // признак, что массив в структуре
				wasarr = 1;

				if (parser->next_token == equal)
				{
					consume_token(parser);
					consume_token(parser);
					if (is_array(parser->sx, type)) // инициализация массива
					{
						parser->onlystrings = 2;
						parser->sx->tree[all] = 1;
						if (!parser->usual)
						{
							parser->sx->tree[adN]--; // это уменьшение N в Declarr
						}
						array_init(parser, type);
						if (parser->onlystrings == 1)
						{
							parser->sx->tree[all + 2] = parser->usual + 2; // только из строк 2 - без
						}
						// границ, 3 - с границами
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
		displ += szof(parser, type);

		expect_and_consume_token(parser, semicolon, no_semicolon_in_struct);
	} while (!try_consume_token(parser, r_brace));

	if (wasarr)
	{
		totree(parser, TStructend);
		totree(parser, (int)struct_begin_ref);
		parser->sx->tree[struct_begin_ref + 1] = parser->was_struct_with_arr = parser->sx->procd++;
	}
	else
	{
		parser->sx->tree[struct_begin_ref] = NOP;
		parser->sx->tree[struct_begin_ref + 1] = NOP;
	}

	local_modetab[1] = displ;
	local_modetab[2] = (int)field_number * 2;

	return mode_add(parser->sx, local_modetab, local_md);
}


void inition(parser *context, int decl_type)
{
	if (decl_type < 0 || is_pointer(context->sx, decl_type) || // Обработка для базовых типов, указателей
		(is_string(context->sx, decl_type))) // или строк
	{
		exprassn(context, 1);
		if (context->was_error == 6)
		{
			context->was_error = 5;
			return; // 1
		}
		toval(context);
		totree(context, TExprend);
		// съедаем выражение, его значение будет на стеке
		context->sopnd--;
		if (is_int(decl_type) && is_float(context->ansttype))
		{
			parser_error(context, init_int_by_float);
			context->was_error = 5;
			return; // 1
		}
		if (is_float(decl_type) && is_int(context->ansttype))
		{
			insertwiden(context);
		}
		else if (decl_type != context->ansttype)
		{
			parser_error(context, error_in_initialization);
			context->was_error = 5;
			return; // 1
		}
	}
	else if (context->curr_token == BEGIN)
	{
		struct_init(context, decl_type);
	}
	else
	{
		parser_error(context, wrong_init);
		context->was_error = 5;
		return; // 1
	}
}

void struct_init(parser *context, int decl_type)
{
	// сейчас modetab[decl_type] равен MSTRUCT

	int next_field = decl_type + 3;
	int i;
	int nf = mode_get(context->sx, decl_type + 2) / 2;

	if (context->curr_token != BEGIN)
	{
		parser_error(context, struct_init_must_start_from_BEGIN);
		context->buf_cur = context->next_token;
		context->next_token = context->curr_token;
		context->curr_token = BEGIN;
		context->buf_flag++;
	}
	totree(context, TStructinit);
	totree(context, nf);
	for (i = 0; i < nf; i++)
	{
		scanner(context);
		inition(context, mode_get(context->sx, next_field));
		if (context->was_error == 5)
		{
			context->was_error = 1;
			return; // 1
		}
		next_field += 2;
		if (i != nf - 1)
		{
			if (context->next_token == COMMA) // поля инициализации идут через запятую, заканчиваются }
			{
				scanner(context);
			}
			else
			{
				parser_error(context, no_comma_in_init_list);
				context->next_token = context->curr_token;
				context->curr_token = COMMA;
			}
		}
	}

	if (context->next_token == END)
	{
		totree(context, TExprend);
		scanner(context);
	}
	else
	{
		parser_error(context, wait_end);
		context->curr_token = END;
	}
	context->leftansttype = decl_type;
}

void array_init(parser *context, int decl_type)
{
	// сейчас modetab[decl_type] равен MARRAY

	if (is_array(context->sx, decl_type))
	{
		if (context->curr_token == STRING)
		{
			if (context->onlystrings == 0)
			{
				parser_error(context, string_and_notstring);
				context->was_error = 7;
				return; // 1
			}
			if (context->onlystrings == 2)
			{
				context->onlystrings = 1;
			}
			primaryexpr(context);
			if (context->was_error == 4)
			{
				context->was_error = 7;
				return; // 1
			}
			totree(context, TExprend);
		}
		else
		{
			if (context->curr_token != BEGIN)
			{
				parser_error(context, arr_init_must_start_from_BEGIN);
				context->buf_cur = context->next_token;
				context->next_token = context->curr_token;
				context->curr_token = BEGIN;
				context->buf_flag++;
			}
			totree(context, TBeginit);
			size_t ad = context->sx->tc++;

			int all = 0;
			do
			{
				scanner(context);
				all++;
				array_init(context, mode_get(context->sx, decl_type + 1));
				if (context->was_error == 7)
				{
					return; // 1
				}
			} while (scanner(context) == COMMA);

			if (context->curr_token == END)
			{
				context->sx->tree[ad] = all;
				totree(context, TExprend);
			}
			else
			{
				parser_error(context, wait_end);
				context->was_error = 7;
				return; // 1
			}
		}
	}
	else if (context->curr_token == BEGIN)
	{
		if (is_struct(context->sx, decl_type))
		{
			struct_init(context, decl_type);
		}
		else
		{
			parser_error(context, begin_with_notarray);
			context->was_error = 7;
			return; // 1
		}
	}
	else if (context->onlystrings == 1)
	{
		parser_error(context, string_and_notstring);
		context->was_error = 7;
		return; // 1
	}
	else
	{
		inition(context, decl_type);
		context->onlystrings = 0;
	}
}

int arrdef(parser *context, int t)
{
	// вызывается при описании массивов и структур из массивов сразу после idorpnt

	context->arrdim = 0;
	context->usual = 1; // описание массива без пустых границ
	if (is_pointer(context->sx, t))
	{
		parser_error(context, pnt_before_array);
		context->was_error = 5;
		return 0; // 1
	}

	while (context->next_token == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		context->arrdim++;
		scanner(context);
		if (context->next_token == RIGHTSQBR)
		{
			scanner(context);
			if (context->next_token == LEFTSQBR) // int a[][]={{1,2,3}, {4,5,6}} - нельзя;
			{
				parser_error(context, empty_init); // границы определять по инициализации можно
				context->was_error = 5;
				return 0; // 1
			}
			// только по последнему изм.
			context->usual = 0;
		}
		else
		{
			scanner(context);
			unarexpr(context);
			if (context->was_error == 7)
			{
				context->was_error = 5;
				return 0; // 1
			}
			condexpr(context);
			if (context->was_error == 7)
			{
				context->was_error = 5;
				return 0; // 1
			}
			toval(context);
			if (!is_int(context->ansttype))
			{
				parser_error(context, array_size_must_be_int);
				context->was_error = 5;
				return 0; // 1
			}
			totree(context, TExprend);
			context->sopnd--;
			mustbe(context, RIGHTSQBR, wait_right_sq_br);
		}
		t = newdecl(context->sx, mode_array, t); // Меняем тип в identtab (увеличиваем размерность массива)
	}
	return t;
}

void decl_id(parser *context, int decl_type)
{
	// вызывается из block и extdecl, только эта процедура реально отводит память
	// если встретятся массивы (прямо или в структурах), их размеры уже будут в стеке

	int oldid = toidentab(context, (size_t)REPRTAB_POS, 0, decl_type);
	int elem_type;
	size_t adN = 0; // warning C4701: potentially uninitialized local variable used

	context->usual = 1;
	context->arrdim = 0; // arrdim - размерность (0-скаляр), д.б. столько выражений-границ
	elem_type = decl_type;
	if (context->was_error == 5)
	{
		context->was_error = 4;
		return; // 1
	}

	if (context->next_token == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		totree(context, TDeclarr);
		adN = context->sx->tc++;
		// Меняем тип (увеличиваем размерность массива)
		decl_type = arrdef(context, decl_type);
		ident_set_mode(context->sx, oldid, decl_type);
		context->sx->tree[adN] = context->arrdim;
		if (context->was_error == 5)
		{
			context->was_error = 4;
			return; // 1
		}
		if ((!context->usual && context->next_token != ASS))
		{
			parser_error(context, empty_bound_without_init);
			context->was_error = 4;
			return; // 1
		}
	}
	totree(context, TDeclid);
	totree(context, ident_get_displ(context->sx, oldid));												// displ
	totree(context, elem_type);																		// elem_type
	totree(context, context->arrdim);																// N
	size_t all = context->sx->tc++; // all - место в дереве, где будет общее количество выражений в инициализации,
									// для массивов - только признак (1) наличия инициализации
	context->sx->tree[all] = 0;
	context->sx->tree[context->sx->tc++] = is_pointer(context->sx, decl_type) ? 0 : context->was_struct_with_arr; // proc
	totree(context, context->usual);																	// context->usual
	totree(context, 0); // массив не в структуре

	if (context->next_token == ASS)
	{
		scanner(context);
		scanner(context);
		context->sx->tree[all] = szof(context, decl_type);
		if (is_array(context->sx, decl_type)) // инициализация массива
		{
			context->onlystrings = 2;
			if (!context->usual)
			{
				context->sx->tree[adN]--; // это уменьшение N в Declarr
			}
			array_init(context, decl_type);
			if (context->was_error == 7)
			{
				context->was_error = 4;
				return; // 1
			}
			if (context->onlystrings == 1)
			{
				context->sx->tree[all + 2] = context->usual + 2; // только из строк 2 - без границ, 3 - с границами
			}
		}
		else
		{
			inition(context, decl_type);
		}
	}
	if (context->was_error == 5)
	{
		context->was_error = 4;
		return; // 1
	}
}


size_t func_declarator(parser *context, int level, int func_d, int firstdecl)
{
	// на 1 уровне это может быть определением функции или предописанием, на
	// остальных уровнях - только декларатором (без идентов)

	int loc_modetab[100];
	int locmd;
	int numpar = 0;
	int ident = 0; // warning C4701: potentially uninitialized local variable used
	int maybe_fun = 0; // warning C4701: potentially uninitialized local variable used
	int repeat = 1;
	int wastype = 0;
	int old;

	loc_modetab[0] = mode_function;
	loc_modetab[1] = firstdecl;
	loc_modetab[2] = 0;
	locmd = 3;

	while (repeat)
	{
		if (context->curr_token == LVOID || is_int(context->curr_token) || is_float(context->curr_token) ||
			context->curr_token == LSTRUCT)
		{
			maybe_fun = 0; // м.б. параметр-ф-ция?
						   // 0 - ничего не было,
						   // 1 - была *,
						   // 2 - была [

			ident = 0; // = 0 - не было идента,
					   // 1 - был статический идент,
					   // 2 - был идент-параметр-функция
			wastype = 1;
			context->type = parse_type_specifier(context);
			if (context->was_error == 3)
			{
				context->was_error = 2;
				return 0;
			}
			if (context->next_token == LMULT)
			{
				maybe_fun = 1;
				scanner(context);
				context->type = context->type == LVOID ? LVOIDASTER : newdecl(context->sx, mode_pointer, context->type);
			}
			if (level)
			{
				if (context->next_token == IDENT)
				{
					scanner(context);
					ident = 1;
					func_add(context->sx, REPRTAB_POS);
				}
			}
			else if (context->next_token == IDENT)
			{
				parser_error(context, ident_in_declarator);
				context->was_error = 2;
				return 0;
			}

			if (context->next_token == LEFTSQBR)
			{
				maybe_fun = 2;

				if (is_pointer(context->sx, context->type) && ident == 0)
				{
					parser_error(context, aster_with_row);
					context->was_error = 2;
					return 0;
				}

				while (context->next_token == LEFTSQBR)
				{
					scanner(context);
					mustbe(context, RIGHTSQBR, wait_right_sq_br);
					context->type = newdecl(context->sx, mode_array, context->type);
				}
			}
		}
		if (context->curr_token == LVOID)
		{
			context->type = LVOID;
			wastype = 1;
			if (context->next_token != LEFTBR)
			{
				parser_error(context, par_type_void_with_nofun);
				context->was_error = 2;
				return 0;
			}
		}

		if (wastype)
		{
			numpar++;
			loc_modetab[locmd++] = context->type;

			if (context->next_token == LEFTBR)
			{
				scanner(context);
				mustbe(context, LMULT, wrong_fun_as_param);
				if (context->next_token == IDENT)
				{
					if (level)
					{
						scanner(context);
						if (ident == 0)
						{
							ident = 2;
						}
						else
						{
							parser_error(context, two_idents_for_1_declarer);
							context->was_error = 2;
							return 0;
						}
						func_add(context->sx, -REPRTAB_POS);
					}
					else
					{
						parser_error(context, ident_in_declarator);
						context->was_error = 2;
						return 0;
					}
				}
				mustbe(context, RIGHTBR, no_right_br_in_paramfun);
				mustbe(context, LEFTBR, wrong_fun_as_param);
				scanner(context);
				if (maybe_fun == 1)
				{
					parser_error(context, aster_before_func);
					context->was_error = 2;
					return 0;
				}
				else if (maybe_fun == 2)
				{
					parser_error(context, array_before_func);
					context->was_error = 2;
					return 0;
				}

				old = context->func_def;
				loc_modetab[locmd - 1] = (int)func_declarator(context, 0, 2, context->type);
				if (context->was_error == 2)
				{
					return 0;
				}
				context->func_def = old;
			}
			if (func_d == 3)
			{
				func_d = ident > 0 ? 1 : 2;
			}
			else if (func_d == 2 && ident > 0)
			{
				parser_error(context, wait_declarator);
				context->was_error = 2;
				return 0;
			}
			else if (func_d == 1 && ident == 0)
			{
				parser_error(context, wait_definition);
				context->was_error = 2;
				return 0;
			}

			if (scanner(context) == COMMA)
			{
				scanner(context);
			}
			else if (context->curr_token == RIGHTBR)
			{
				repeat = 0;
			}
		}
		else if (context->curr_token == RIGHTBR)
		{
			repeat = 0;
			func_d = 0;
		}
		else
		{
			parser_error(context, wrong_param_list);
			context->buf_cur = context->next_token;
			context->next_token = context->curr_token;
			context->curr_token = RIGHTBR;
			context->buf_flag++;
			repeat = 0;
			func_d = 0;
		}
	}
	context->func_def = func_d;
	loc_modetab[2] = numpar;

	return mode_add(context->sx, loc_modetab, locmd);
}

void function_definition(parser *const parser, const size_t function_id)
{
	parser->function_type = ident_get_mode(parser->sx, function_id);
	const size_t function_number = (size_t)ident_get_displ(parser->sx, function_id);
	const size_t param_number = (size_t)mode_get(parser->sx, parser->function_type + 2);

	parser->pgotost = 0;
	parser->flag_was_return = 0;

	const int pred = parser->sx->identab[function_id];
	if (pred > 1) // был прототип
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

		toidentab(parser, abs(repr), 0, type);
	}

	func_set(parser->sx, function_number, parser->sx->tc);
	totree(parser, TFuncdef);
	totree(parser, function_id);

	const size_t maxdispl_ref = parser->sx->tc++;

	consume_token(parser);
	parse_compound_statement(parser, FUNCBODY);

	parser->sx->tc--;
	totree(parser, TReturnvoid);
	totree(parser, TEnd);

	if (mode_get(parser->sx, parser->function_type + 1) != mode_void && !parser->flag_was_return)
	{
		parser_error(parser, no_ret_in_func);
	}

	scope_func_exit(parser->sx, maxdispl_ref, old_displ);

	for (int i = 0; i < parser->pgotost - 1; i += 2)
	{
		parser->lxr->repr = parser->sx->identab[parser->gotost[i] + 1];
		parser->sx->hash = parser->gotost[i + 1];
		if (parser->sx->hash < 0)
		{
			parser->sx->hash = -parser->sx->hash;
		}
		if (!parser->sx->identab[parser->gotost[i] + 2])
		{
			parser_error(parser, label_not_declared);
		}
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
	consume_token(parser);
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
			decl_id(parser, type);
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
	consume_token(parser);
	parser->flag_was_type_def = 0;
	int group_type = parse_type_specifier(parser);

	if (parser->flag_was_type_def && try_consume_token(parser, semicolon))
	{
		return;
	}

	parser->func_def = 3;
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
				const size_t function_num = parser->sx->funcnum++;
				const size_t function_repr = (size_t)parser->lxr->repr;
				consume_token(parser);
				consume_token(parser);

				type = (int)func_declarator(parser, 1, 3, type);

				if (parser->func_def == 0 && parser->next_token == l_brace)
				{
					parser->func_def = 1;
				}
				else if (parser->func_def == 0)
				{
					parser->func_def = 2;
				}
				
				const size_t function_id = toidentab(parser, function_repr, (int)function_num, type);

				if (parser->next_token == l_brace)
				{
					if (parser->func_def == 1)
					{
						function_definition(parser, function_id);
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
			else if (group_type == LVOID)
			{
				parser_error(parser, only_functions_may_have_type_VOID);
			}
			else
			{
				decl_id(parser, type);
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

/** Генерация дерева */
void ext_decl(parser *const parser)
{
	get_char(parser->lxr);
	get_char(parser->lxr);
	consume_token(parser);

	do
	{
		parse_external_declaration(parser);
	} while (parser->next_token != eof);

	totree(parser, TEnd);
}
