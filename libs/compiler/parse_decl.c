/*
 *	Copyright 2016 Andrey Terekhov
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

#include "extdecl.h"
#include "errors.h"
#include "defs.h"
#include "lexer.h"
#include <string.h>


void inition(parser *context, int decl_type)
{
	if (decl_type < 0 || is_pointer(context->sx, decl_type) || // Обработка для базовых типов, указателей
		(is_string(context->sx, decl_type))) // или строк
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
		toval(context);
		totree(context, TExprend);
		// съедаем выражение, его значение будет на стеке
		context->sopnd--;
		if (is_int(decl_type) && is_float(context->ansttype))
		{
			context_error(context, init_int_by_float);
			context->error_flag = 5;
			return; // 1
		}
		if (is_float(decl_type) && is_int(context->ansttype))
		{
			insertwiden(context);
		}
		else if (decl_type != context->ansttype)
		{
			context_error(context, error_in_initialization);
			context->error_flag = 5;
			return; // 1
		}
	}
	else if (context->cur == BEGIN)
	{
		struct_init(context, decl_type);
	}
	else
	{
		context_error(context, wrong_init);
		context->error_flag = 5;
		return; // 1
	}
}

void struct_init(parser *context, int decl_type)
{
	// сейчас modetab[decl_type] равен mode_struct

	int next_field = decl_type + 3;
	item_t nf = mode_get(context->sx, decl_type + 2) / 2;

	if (context->cur != BEGIN)
	{
		context_error(context, struct_init_must_start_from_BEGIN);
		context->buf_cur = context->next;
		context->next = context->cur;
		context->cur = BEGIN;
		context->buf_flag++;
	}
	totree(context, TStructinit);
	totree(context, nf);
	for (item_t i = 0; i < nf; i++)
	{
		scanner(context);
		inition(context, (int)mode_get(context->sx, next_field));
		if (context->error_flag == 5)
		{
			context->error_flag = 1;
			return; // 1
		}
		next_field += 2;
		if (i != nf - 1)
		{
			if (context->next == COMMA) // поля инициализации идут через запятую, заканчиваются }
			{
				scanner(context);
			}
			else
			{
				context_error(context, no_comma_in_init_list);
				context->next = context->cur;
				context->cur = COMMA;
			}
		}
	}

	if (context->next == END)
	{
		totree(context, TExprend);
		scanner(context);
	}
	else
	{
		context_error(context, wait_end);
		context->cur = END;
	}
	context->leftansttype = decl_type;
}

void array_init(parser *context, int decl_type)
{
	// сейчас modetab[decl_type] равен mode_array

	if (is_array(context->sx, decl_type))
	{
		if (context->cur == STRING)
		{
			if (context->onlystrings == 0)
			{
				context_error(context, string_and_notstring);
				context->error_flag = 7;
				return; // 1
			}
			if (context->onlystrings == 2)
			{
				context->onlystrings = 1;
			}
			primaryexpr(context);
			if (context->error_flag == 4)
			{
				context->error_flag = 7;
				return; // 1
			}
			totree(context, TExprend);
		}
		else
		{
			if (context->cur != BEGIN)
			{
				context_error(context, arr_init_must_start_from_BEGIN);
				context->buf_cur = context->next;
				context->next = context->cur;
				context->cur = BEGIN;
				context->buf_flag++;
			}
			totree(context, TBeginit);
			size_t ad = vector_size(&TREE);
			vector_increase(&TREE, 1);

			int all = 0;
			do
			{
				scanner(context);
				all++;
				array_init(context, (int)mode_get(context->sx, decl_type + 1));
				if (context->error_flag == 7)
				{
					return; // 1
				}
			} while (scanner(context) == COMMA);

			if (context->cur == END)
			{
				vector_set(&TREE, ad, all);
				totree(context, TExprend);
			}
			else
			{
				context_error(context, wait_end);
				context->error_flag = 7;
				return; // 1
			}
		}
	}
	else if (context->cur == BEGIN)
	{
		if (is_struct(context->sx, decl_type))
		{
			struct_init(context, decl_type);
		}
		else
		{
			context_error(context, begin_with_notarray);
			context->error_flag = 7;
			return; // 1
		}
	}
	else if (context->onlystrings == 1)
	{
		context_error(context, string_and_notstring);
		context->error_flag = 7;
		return; // 1
	}
	else
	{
		inition(context, decl_type);
		context->onlystrings = 0;
	}
}

int arrdef(parser *context, item_t t)
{
	// вызывается при описании массивов и структур из массивов сразу после idorpnt

	context->arrdim = 0;
	context->usual = 1; // описание массива без пустых границ
	if (is_pointer(context->sx, (int)t))
	{
		context_error(context, pnt_before_array);
		context->error_flag = 5;
		return 0; // 1
	}

	while (context->next == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		context->arrdim++;
		scanner(context);
		if (context->next == RIGHTSQBR)
		{
			scanner(context);
			if (context->next == LEFTSQBR) // int a[][]={{1,2,3}, {4,5,6}} - нельзя;
			{
				context_error(context, empty_init); // границы определять по инициализации можно
				context->error_flag = 5;
				return 0; // 1
			}
			// только по последнему изм.
			context->usual = 0;
		}
		else
		{
			scanner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 5;
				return 0; // 1
			}
			condexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 5;
				return 0; // 1
			}
			toval(context);
			if (!is_int(context->ansttype))
			{
				context_error(context, array_size_must_be_int);
				context->error_flag = 5;
				return 0; // 1
			}
			totree(context, TExprend);
			context->sopnd--;
			mustbe(context, RIGHTSQBR, wait_right_sq_br);
		}
		t = newdecl(context->sx, mode_array, t); // Меняем тип в identtab (увеличиваем размерность массива)
	}
	return (int)t;
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
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}

	if (context->next == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		totree(context, TDeclarr);
		adN = vector_size(&TREE);
		vector_increase(&TREE, 1);
		// Меняем тип (увеличиваем размерность массива)
		decl_type = arrdef(context, decl_type);
		ident_set_mode(context->sx, oldid, decl_type);
		vector_set(&TREE, adN, context->arrdim);
		if (context->error_flag == 5)
		{
			context->error_flag = 4;
			return; // 1
		}
		if ((!context->usual && context->next != ASS))
		{
			context_error(context, empty_bound_without_init);
			context->error_flag = 4;
			return; // 1
		}
	}
	totree(context, TDeclid);
	totree(context, ident_get_displ(context->sx, oldid));											// displ
	totree(context, elem_type);																		// elem_type
	totree(context, context->arrdim);																// N
	size_t all = vector_size(&TREE);	// all - место в дереве, где будет общее количество выражений в инициализации,
										// для массивов - только признак (1) наличия инициализации
	vector_increase(&TREE, 1);
	vector_set(&TREE, all, 0);
	vector_add(&TREE, is_pointer(context->sx, decl_type) ? 0 : context->was_struct_with_arr);		// proc
	totree(context, context->usual);																// context->usual
	totree(context, 0); // массив не в структуре

	if (context->next == ASS)
	{
		scanner(context);
		scanner(context);
		vector_set(&TREE, all, szof(context, decl_type));
		if (is_array(context->sx, decl_type)) // инициализация массива
		{
			context->onlystrings = 2;
			if (!context->usual)
			{
				vector_set(&TREE, adN, vector_get(&TREE, adN) - 1); // это уменьшение N в Declarr
			}
			array_init(context, decl_type);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (context->onlystrings == 1)
			{
				vector_set(&TREE, all + 2, context->usual + 2); // только из строк 2 - без границ, 3 - с границами
			}
		}
		else
		{
			inition(context, decl_type);
		}
	}
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
}

item_t idorpnt(parser *context, int e, item_t t)
{
	if (context->next == LMULT)
	{
		scanner(context);
		t = t == LVOID ? LVOIDASTER : newdecl(context->sx, mode_pointer, t);
	}
	mustbe_complex(context, IDENT, e);
	return t;
}

int struct_decl_list(parser *context)
{
	int field_count = 0;
	item_t t;
	item_t elem_type;
	int curdispl = 0;
	int wasarr = 0;
	item_t loc_modetab[100];
	int locmd = 3;

	loc_modetab[0] = mode_struct;
	size_t tstrbeg = vector_size(&TREE);
	totree(context, TStructbeg);
	vector_increase(&TREE, 1); // тут будет номер иниц процедуры

	scanner(context);
	scanner(context);

	do
	{
		int oldrepr;
		t = elem_type = idorpnt(context, wait_ident_after_semicomma_in_struct, gettype(context));
		if (context->error_flag == wait_ident_after_semicomma_in_struct || context->error_flag == 3)
		{
			context->error_flag = 3;
			return 0;
		}
		oldrepr = REPRTAB_POS;
		if (context->next == LEFTSQBR)
		{
			totree(context, TDeclarr);
			size_t adN = vector_size(&TREE);
			vector_increase(&TREE, 1);
			t = arrdef(context, elem_type); // Меняем тип (увеличиваем размерность массива)
			if (context->error_flag == 5)
			{
				context->error_flag = 3;
				return 0;
			}
			vector_set(&TREE, adN, context->arrdim);

			totree(context, TDeclid);
			totree(context, curdispl);
			totree(context, elem_type);
			totree(context, context->arrdim);						// N
			size_t all = vector_size(&TREE);
			vector_increase(&TREE, 1);
			vector_set(&TREE, all, 0);								// all
			vector_add(&TREE, context->was_struct_with_arr);	 	// proc
			totree(context, context->usual);						// context->usual
			totree(context, 1); // признак, что массив в структуре
			wasarr = 1;
			if (context->next == ASS)
			{
				scanner(context);
				scanner(context);
				if (is_array(context->sx, (int)t)) // инициализация массива
				{
					context->onlystrings = 2;
					vector_set(&TREE, all, 1);
					if (!context->usual)
					{
						vector_set(&TREE, adN, vector_get(&TREE, adN) - 1); // это уменьшение N в Declarr
					}
					array_init(context, (int)t);
					if (context->error_flag == 7)
					{
						context->error_flag = 3;
						return 0;
					}
					if (context->onlystrings == 1)
					{
						vector_set(&TREE, all + 2, context->usual + 2);			// только из строк 2 - без границ, 3 - с границами
					}
				}
				else
				{
					// structdispl = ident_get_displ(context->sx, oldid);
					// vector_get(&TREE, all) = inition(context, t);
				}
			} // конец ASS
		}	  // конец LEFTSQBR
		loc_modetab[locmd++] = (int)t;
		loc_modetab[locmd++] = oldrepr;
		field_count++;
		curdispl += szof(context, (int)t);
		if (scanner(context) != SEMICOLON)
		{
			context_error(context, no_semicomma_in_struct);
			context->buf_cur = context->next;
			context->next = context->cur;
			context->cur = BEGIN;
			context->buf_flag++;
		}
	} while (scanner(context) != END);

	if (wasarr)
	{
		totree(context, TStructend);

		const int procd = (int)vector_size(&context->sx->processes);
		vector_increase(&context->sx->processes, 1);

		totree(context, procd);
		vector_set(&TREE, tstrbeg + 1, procd);
		context->was_struct_with_arr = procd;
	}
	else
	{
		vector_set(&TREE, tstrbeg, NOP);
		vector_set(&TREE, tstrbeg + 1, NOP);
	}

	loc_modetab[1] = curdispl; // тут длина структуры
	loc_modetab[2] = field_count * 2;
	
	return (int)mode_add(context->sx, loc_modetab, locmd);
}

item_t gettype(parser *context)
{
	// gettype(context) выедает тип (кроме верхних массивов и указателей)
	// при этом, если такого типа нет в modetab, тип туда заносится;
	// возвращает отрицательное число(базовый тип), положительное (ссылка на modetab)
	// или 0, если типа не было

	context->was_struct_with_arr = 0;
	if (is_int(context->type = context->cur) || is_float(context->type) || context->type == LVOID)
	{
		return (context->cur == LLONG ? LINT : context->cur == LDOUBLE ? LFLOAT : context->type);
	}

	if (context->type == LSTRUCT)
	{
		if (context->next == BEGIN) // struct {
		{
			return (struct_decl_list(context));
		}

		if (context->next == IDENT)
		{
			int l;

			l = REPRTAB[REPRTAB_POS + 1];
			scanner(context);
			if (context->next == BEGIN) // struct key {
			{
				// если такое описание уже было, то это ошибка - повторное описание
				int lid;
				context->wasstructdef = 1; // это  определение типа (может быть,
										   // без описания переменных)
				toidentab(context, 1000, 0);
				if (context->error_flag == 5)
				{
					context->error_flag = 3;
					return 0; // 1
				}
				lid = context->lastid;
				ident_set_mode(context->sx, lid, struct_decl_list(context));
				ident_set_displ(context->sx, lid, 1000 + context->was_struct_with_arr);
				return ident_get_mode(context->sx, lid);
			}
			else // struct key это применение типа
			{
				if (l == 1)
				{
					context_error(context, ident_is_not_declared);
					context->error_flag = 3;
					return 0; // 1
				}
				context->was_struct_with_arr = (int)ident_get_displ(context->sx, l) - 1000;
				return ident_get_mode(context->sx, l);
			}
		}

		context_error(context, wrong_struct);
		context->error_flag = 3;
		return 0; // 1
	}

	if (context->cur == IDENT)
	{
		applid(context);
		if (context->error_flag == 5)
		{
			context->error_flag = 3;
			return 0; // 1
		}

		const item_t displ = ident_get_displ(context->sx, context->lastid);
		if (displ == ITEM_MAX || displ < 1000)
		{
			context_error(context, ident_not_type);
			context->error_flag = 3;
			return 0; // 1
		}

		context->was_struct_with_arr = (int)displ - 1000;
		return ident_get_mode(context->sx, context->lastid);
	}

	context_error(context, not_decl);
	context->error_flag = 3;
	return 0; // 1
}


void function_definition(parser *context)
{
	item_t fn = ident_get_displ(context->sx, context->lastid);
	int oldrepr = REPRTAB_POS;
	int fid = context->lastid;

	context->pgotost = 0;
	context->functype = (int)ident_get_mode(context->sx, context->lastid);
	item_t ftype = mode_get(context->sx, context->functype + 1);
	item_t n = mode_get(context->sx, context->functype + 2);
	context->wasret = 0;
	
	size_t prev = (size_t)vector_get(&context->sx->identifiers, context->lastid);
	if (prev > 1) // был прототип
	{
		if (context->functype != ident_get_mode(context->sx, prev))
		{
			context_error(context, decl_and_def_have_diff_type);
			return; // 1
		}
		ident_set_displ(context->sx, prev, fn);
	}
	
	const item_t old_displ = scope_func_enter(context->sx);
	for (item_t i = 0; i < n; i++)
	{
		context->type = (int)mode_get(context->sx, context->functype + (int)i + 3);
		item_t temp = func_get(context->sx, (size_t)fn + (size_t)i + 1);
		if (temp == ITEM_MAX)
		{
			context->error_flag = 1;
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
		if (context->error_flag == 5)
		{
			context->error_flag = 1;
			return; // 1
		}
	}
	const size_t size = vector_size(&TREE);
	func_set(context->sx, (size_t)fn, (int)size);
	totree(context, TFuncdef);
	totree(context, fid);
	prev = vector_size(&TREE);
	vector_increase(&TREE, 1);
	REPRTAB_POS = oldrepr;

	block(context, 0);

	// if (ftype == LVOID && vector_get(&TREE, vector_size(&TREE) - 1) != TReturnvoid)
	// {
	vector_remove(&TREE);
	totree(context, TReturnvoid);
	totree(context, TEnd);
	// }
	if (ftype != LVOID && !context->wasret)
	{
		context_error(context, no_ret_in_func);
		context->error_flag = 1;
		return; // 1
	}
	
	scope_func_exit(context->sx, prev, old_displ);

	for (int i = 0; i < context->pgotost - 1; i += 2)
	{
		REPRTAB_POS = (int)ident_get_repr(context->sx, context->gotost[i]);
		context->sx->hash = context->gotost[i + 1];
		if (context->sx->hash < 0)
		{
			context->sx->hash = -context->sx->hash;
		}
		if (!ident_get_mode(context->sx, context->gotost[i]))
		{
			context_error(context, label_not_declared);
			context->error_flag = 1;
			return; // 1
		}
	}
}

int func_declarator(parser *context, int level, int func_d, int firstdecl)
{
	// на 1 уровне это может быть определением функции или предописанием, на
	// остальных уровнях - только декларатором (без идентов)

	item_t loc_modetab[100];
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
		if (context->cur == LVOID || is_int(context->cur) || is_float(context->cur) ||
			context->cur == LSTRUCT)
		{
			maybe_fun = 0; // м.б. параметр-ф-ция?
						   // 0 - ничего не было,
						   // 1 - была *,
						   // 2 - была [

			ident = 0; // = 0 - не было идента,
					   // 1 - был статический идент,
					   // 2 - был идент-параметр-функция
			wastype = 1;
			context->type = (int)gettype(context);
			if (context->error_flag == 3)
			{
				context->error_flag = 2;
				return 0;
			}
			if (context->next == LMULT)
			{
				maybe_fun = 1;
				scanner(context);
				context->type = context->type == LVOID ? LVOIDASTER : (int)newdecl(context->sx, mode_pointer, context->type);
			}
			if (level)
			{
				if (context->next == IDENT)
				{
					scanner(context);
					ident = 1;
					func_add(context->sx, REPRTAB_POS);
				}
			}
			else if (context->next == IDENT)
			{
				context_error(context, ident_in_declarator);
				context->error_flag = 2;
				return 0;
			}

			if (context->next == LEFTSQBR)
			{
				maybe_fun = 2;

				if (is_pointer(context->sx, context->type) && ident == 0)
				{
					context_error(context, aster_with_row);
					context->error_flag = 2;
					return 0;
				}

				while (context->next == LEFTSQBR)
				{
					scanner(context);
					mustbe(context, RIGHTSQBR, wait_right_sq_br);
					context->type = (int)newdecl(context->sx, mode_array, context->type);
				}
			}
		}
		if (context->cur == LVOID)
		{
			context->type = LVOID;
			wastype = 1;
			if (context->next != LEFTBR)
			{
				context_error(context, par_type_void_with_nofun);
				context->error_flag = 2;
				return 0;
			}
		}

		if (wastype)
		{
			numpar++;
			loc_modetab[locmd++] = context->type;

			if (context->next == LEFTBR)
			{
				scanner(context);
				mustbe(context, LMULT, wrong_fun_as_param);
				if (context->next == IDENT)
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
							context_error(context, two_idents_for_1_declarer);
							context->error_flag = 2;
							return 0;
						}
						func_add(context->sx, -REPRTAB_POS);
					}
					else
					{
						context_error(context, ident_in_declarator);
						context->error_flag = 2;
						return 0;
					}
				}
				mustbe(context, RIGHTBR, no_right_br_in_paramfun);
				mustbe(context, LEFTBR, wrong_fun_as_param);
				scanner(context);
				if (maybe_fun == 1)
				{
					context_error(context, aster_before_func);
					context->error_flag = 2;
					return 0;
				}
				else if (maybe_fun == 2)
				{
					context_error(context, array_before_func);
					context->error_flag = 2;
					return 0;
				}

				old = context->func_def;
				loc_modetab[locmd - 1] = func_declarator(context, 0, 2, context->type);
				if (context->error_flag == 2)
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
				context_error(context, wait_declarator);
				context->error_flag = 2;
				return 0;
			}
			else if (func_d == 1 && ident == 0)
			{
				context_error(context, wait_definition);
				context->error_flag = 2;
				return 0;
			}

			if (scanner(context) == COMMA)
			{
				scanner(context);
			}
			else if (context->cur == RIGHTBR)
			{
				repeat = 0;
			}
		}
		else if (context->cur == RIGHTBR)
		{
			repeat = 0;
			func_d = 0;
		}
		else
		{
			context_error(context, wrong_param_list);
			context->buf_cur = context->next;
			context->next = context->cur;
			context->cur = RIGHTBR;
			context->buf_flag++;
			repeat = 0;
			func_d = 0;
		}
	}
	context->func_def = func_d;
	loc_modetab[2] = numpar;
	
	return (int)mode_add(context->sx, loc_modetab, locmd);
}
