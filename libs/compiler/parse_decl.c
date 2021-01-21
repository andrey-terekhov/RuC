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

#define undefined 0

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
			return LVOID;

		case kw_char:
			return LCHAR;

		//case kw_short:
		case kw_int:
		case kw_long:
			return LINT;

		case kw_float:
		case kw_double:
			return LFLOAT;

		case identifier:
		{
			const size_t id = repr_get_reference(parser->sx, parser->lxr->repr);

			if (ident_get_displ(parser->sx, id) < 1000)
			{
				parser_error(parser, ident_not_type);
				return undefined;
			}

			parser->was_struct_with_arr = ident_get_displ(parser->sx, id) - 1000;
			return ident_get_mode(parser->sx, id);
		}

		case kw_struct:
		{
			switch (parser->next_token)
			{
				case l_brace:
					return struct_decl_list(parser);

				case identifier:
				{
					const size_t repr = (size_t)parser->lxr->repr;
					consume_token(parser);

					if (parser->next_token == l_brace)
					{
						const int mode = struct_decl_list(parser);
						const size_t id = ident_add(parser->sx, repr, 1000, mode, 3);
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
							return undefined;
						}

						// TODO: what if it was not a type specifier?
						parser->was_struct_with_arr = ident_get_displ(parser->sx, id) - 1000;
						return ident_get_mode(parser->sx, id);
					}
				}

				default:
					parser_error(parser, wrong_struct);
					return undefined;
			}
		}

		default:
			parser_error(parser, not_decl);
			return undefined;
	}
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
		t = newdecl(context->sx, MARRAY, t); // Меняем тип в identtab (увеличиваем размерность массива)
	}
	return t;
}

void decl_id(parser *context, int decl_type)
{
	// вызывается из block и extdecl, только эта процедура реально отводит память
	// если встретятся массивы (прямо или в структурах), их размеры уже будут в стеке

	int oldid = toidentab(context, 0, decl_type);
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

int idorpnt(parser *context, int e, int t)
{
	if (context->next_token == LMULT)
	{
		scanner(context);
		t = t == LVOID ? LVOIDASTER : newdecl(context->sx, MPOINT, t);
	}
	mustbe_complex(context, IDENT, e);
	return t;
}

int struct_decl_list(parser *context)
{
	int field_count = 0;
	int t;
	int elem_type;
	int curdispl = 0;
	int wasarr = 0;
	int loc_modetab[100];
	int locmd = 3;

	loc_modetab[0] = MSTRUCT;
	size_t tstrbeg = context->sx->tc;
	totree(context, TStructbeg);
	context->sx->tree[context->sx->tc++] = 0; // тут будет номер иниц процедуры

	scanner(context);
	scanner(context);

	do
	{
		int oldrepr;
		t = elem_type = idorpnt(context, wait_ident_after_semicomma_in_struct, parse_type_specifier(context));
		if (context->was_error == wait_ident_after_semicomma_in_struct || context->was_error == 3)
		{
			context->was_error = 3;
			return 0;
		}
		oldrepr = REPRTAB_POS;
		if (context->next_token == LEFTSQBR)
		{
			totree(context, TDeclarr);
			size_t adN = context->sx->tc++;
			t = arrdef(context, elem_type); // Меняем тип (увеличиваем размерность массива)
			if (context->was_error == 5)
			{
				context->was_error = 3;
				return 0;
			}
			context->sx->tree[adN] = context->arrdim;

			totree(context, TDeclid);
			totree(context, curdispl);
			totree(context, elem_type);
			totree(context, context->arrdim);							 // N
			size_t all = context->sx->tc++;
			context->sx->tree[all] = 0;						 // all
			context->sx->tree[context->sx->tc++] = context->was_struct_with_arr; // proc
			totree(context, context->usual);							 // context->usual
			totree(context, 1); // признак, что массив в структуре
			wasarr = 1;
			if (context->next_token == ASS)
			{
				scanner(context);
				scanner(context);
				if (is_array(context->sx, t)) // инициализация массива
				{
					context->onlystrings = 2;
					context->sx->tree[all] = 1;
					if (!context->usual)
					{
						context->sx->tree[adN]--; // это уменьшение N в Declarr
					}
					array_init(context, t);
					if (context->was_error == 7)
					{
						context->was_error = 3;
						return 0;
					}
					if (context->onlystrings == 1)
					{
						context->sx->tree[all + 2] = context->usual + 2; // только из строк 2 - без
					}
					// границ, 3 - с границами
				}
				else
				{
					// structdispl = context->sx->identab[oldid+3];
					// context->sx->tree[all] = inition(context, t);
				}
			} // конец ASS
		}	  // конец LEFTSQBR
		loc_modetab[locmd++] = t;
		loc_modetab[locmd++] = oldrepr;
		field_count++;
		curdispl += szof(context, t);
		if (scanner(context) != SEMICOLON)
		{
			parser_error(context, no_semicomma_in_struct);
			context->buf_cur = context->next_token;
			context->next_token = context->curr_token;
			context->curr_token = BEGIN;
			context->buf_flag++;
		}
	} while (scanner(context) != END);

	if (wasarr)
	{
		totree(context, TStructend);
		totree(context, (int)tstrbeg);
		context->sx->tree[tstrbeg + 1] = context->was_struct_with_arr = context->sx->procd++;
	}
	else
	{
		context->sx->tree[tstrbeg] = NOP;
		context->sx->tree[tstrbeg + 1] = NOP;
	}

	loc_modetab[1] = curdispl; // тут длина структуры
	loc_modetab[2] = field_count * 2;

	return (int)mode_add(context->sx, loc_modetab, locmd);
}

void function_definition(parser *context)
{
	int fn = ident_get_displ(context->sx, context->lastid);
	int pred;
	int oldrepr = REPRTAB_POS;
	int ftype;
	int n;
	int fid = context->lastid;

	context->pgotost = 0;
	context->functype = ident_get_mode(context->sx, context->lastid);
	ftype = mode_get(context->sx, context->functype + 1);
	n = mode_get(context->sx, context->functype + 2);
	context->wasret = 0;

	if ((pred = context->sx->identab[context->lastid]) > 1) // был прототип
	{
		if (context->functype != ident_get_mode(context->sx, pred))
		{
			parser_error(context, decl_and_def_have_diff_type);
			return; // 1
		}
		ident_set_displ(context->sx, pred, fn);
	}

	const int old_displ = scope_func_enter(context->sx);
	for (int i = 0; i < n; i++)
	{
		context->type = mode_get(context->sx, context->functype + i + 3);
		size_t temp = func_get(context->sx, fn + i + 1);
		if (temp == SIZE_MAX)
		{
			context->was_error = 1;
			return;
		}

		REPRTAB_POS = (int)temp;
		if (REPRTAB_POS > 0)
		{
			toidentab(context, 0, context->type);
		}
		else
		{
			REPRTAB_POS = -REPRTAB_POS;
			toidentab(context, -1, context->type);
		}
		if (context->was_error == 5)
		{
			context->was_error = 1;
			return; // 1
		}
	}
	func_set(context->sx, fn, (int)context->sx->tc);
	totree(context, TFuncdef);
	totree(context, fid);
	pred = (int)context->sx->tc++;
	REPRTAB_POS = oldrepr;

	parse_compound_statement(context, 0);

	// if (ftype == LVOID && context->sx->tree[context->sx->tc - 1] != TReturnvoid)
	// {
	context->sx->tc--;
	totree(context, TReturnvoid);
	totree(context, TEnd);
	// }
	if (ftype != LVOID && !context->wasret)
	{
		parser_error(context, no_ret_in_func);
		context->was_error = 1;
		return; // 1
	}

	scope_func_exit(context->sx, pred, old_displ);

	for (int i = 0; i < context->pgotost - 1; i += 2)
	{
		REPRTAB_POS = context->sx->identab[context->gotost[i] + 1];
		context->sx->hash = context->gotost[i + 1];
		if (context->sx->hash < 0)
		{
			context->sx->hash = -context->sx->hash;
		}
		if (!context->sx->identab[context->gotost[i] + 2])
		{
			parser_error(context, label_not_declared);
			context->was_error = 1;
			return; // 1
		}
	}
}

int func_declarator(parser *context, int level, int func_d, int firstdecl)
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

	loc_modetab[0] = MFUNCTION;
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
				context->type = context->type == LVOID ? LVOIDASTER : newdecl(context->sx, MPOINT, context->type);
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
					context->type = newdecl(context->sx, MARRAY, context->type);
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
				loc_modetab[locmd - 1] = func_declarator(context, 0, 2, context->type);
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

	return (int)mode_add(context->sx, loc_modetab, locmd);
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
	int group_type = parse_type_specifier(parser);

	if (parser->flag_was_type_def && parser->next_token == semicolon)
	{
		consume_token(parser);
		return;
	}

	do
	{
		int type = group_type;
		if (parser->next_token == star)
		{
			consume_token(parser);
			type = group_type == LVOID ? LVOIDASTER : newdecl(parser->sx, MPOINT, group_type);
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
	} while (parser->next_token == comma ? consume_token(parser), 1 : 0);

	try_consume_token(parser, semicolon, expected_semi_after_decl);
}

void parse_external_declaration(parser *const parser)
{
	int repeat = 1;
	int funrepr;
	int first = 1;
	parser->flag_was_type_def = 0;
	scanner(parser);

	parser->firstdecl = parse_type_specifier(parser);
	if (parser->was_error == 3)
	{
		parser->was_error = 1;
		return;
	}
	if (parser->flag_was_type_def && parser->next_token == SEMICOLON) // struct point {float x, y;};
	{
		scanner(parser);
		return;
	}

	parser->func_def = 3; // context->func_def = 0 - (),
						   // 1 - определение функции,
						   // 2 - это предописание,
						   // 3 - не знаем или вообще не функция

	//	if (firstdecl == 0)
	//		firstdecl = LINT;

	do // описываемые объекты через ',' определение функции может быть только одно, никаких ','
	{
		parser->type = parser->firstdecl;
		if (parser->next_token == LMULT)
		{
			scanner(parser);
			parser->type = parser->firstdecl == LVOID ? LVOIDASTER : newdecl(parser->sx, MPOINT, parser->firstdecl);
		}
		mustbe_complex(parser, IDENT, after_type_must_be_ident);
		if (parser->was_error == after_type_must_be_ident)
		{
			parser->was_error = 1;
			break;
		}

		if (parser->next_token == LEFTBR) // определение или предописание функции
		{
			size_t oldfuncnum = parser->sx->funcnum++;
			int firsttype = parser->type;
			funrepr = parser->lxr->repr;
			scanner(parser);
			scanner(parser);

			// выкушает все параметры до ) включительно
			parser->type = func_declarator(parser, first, 3, firsttype);
			if (parser->was_error == 2)
			{
				parser->was_error = 1;
				break;
			}

			if (parser->next_token == BEGIN)
			{
				if (parser->func_def == 0)
				{
					parser->func_def = 1;
				}
			}
			else if (parser->func_def == 0)
			{
				parser->func_def = 2;
			}
			// теперь я точно знаю, это определение ф-ции или предописание
			// (context->func_def=1 или 2)
			parser->lxr->repr = funrepr;

			toidentab(parser, (int) oldfuncnum, parser->type);
			if (parser->was_error == 5)
			{
				parser->was_error = 1;
				break;
			}

			if (parser->next_token == BEGIN)
			{
				scanner(parser);
				if (parser->func_def == 2)
				{
					parser_error(parser, func_decl_req_params);
					break;
				}

				function_definition(parser);
				break;
			}
			else
			{
				if (parser->func_def == 1)
				{
					parser_error(parser, function_has_no_body);
					break;
				}
			}
		}
		else if (parser->firstdecl == LVOID)
		{
			parser_error(parser, only_functions_may_have_type_VOID);
			break;
		}

		// описания идентов-не-функций

		if (parser->func_def == 3)
		{
			decl_id(parser, parser->type);
		}

		if (parser->was_error == 4)
		{
			parser->was_error = 1;
			break;
		}

		if (parser->next_token == COMMA)
		{
			scanner(parser);
			first = 0;
		}
		else if (parser->next_token == SEMICOLON)
		{
			scanner(parser);
			repeat = 0;
		}
		else
		{
			parser_error(parser, expected_semi_after_decl);
			parser->curr_token = SEMICOLON;
			repeat = 0;
		}
	} while (repeat);
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
