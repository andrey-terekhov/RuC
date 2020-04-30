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
#include "global.h"
#include "scanner.h"


void exprassnval(compiler_context *);
void expr(compiler_context *context, int level);
void exprassn(compiler_context *context, int);

void struct_init(compiler_context *context, int);
int gettype(compiler_context *context);

// если b=1, то это просто блок,
// b = 2 - блок нити,
// b = -1 - блок в switch, иначе
// b = 0 - это блок функции
void block(compiler_context *context, int b);


int modeeq(compiler_context *context, int first_mode, int second_mode)
{
	int n;
	int i;
	int flag = 1;
	int mode;
	if (context->modetab[first_mode] != context->modetab[second_mode])
	{
		return 0;
	}

	mode = context->modetab[first_mode];
	// определяем, сколько полей надо сравнивать для различных типов записей
	n = mode == MSTRUCT || mode == MFUNCTION ? 2 + context->modetab[first_mode + 2] : 1;

	for (i = 1; i <= n && flag; i++)
	{
		flag = context->modetab[first_mode + i] == context->modetab[second_mode + i];
	}

	return flag;
}

int check_duplicates(compiler_context *context)
{
	// проверяет, имеется ли в modetab только что внесенный тип.
	// если да, то возвращает ссылку на старую запись, иначе - на новую.

	int old = context->modetab[context->startmode];

	while (old)
	{
		if (modeeq(context, context->startmode + 1, old + 1))
		{
			context->md = context->startmode;
			context->startmode = context->modetab[context->startmode];
			return old + 1;
		}
		else
		{
			old = context->modetab[old];
		}
	}
	return context->startmode + 1;
}

int newdecl(compiler_context *context, int type, int elemtype)
{
	context->modetab[context->md] = context->startmode;
	context->startmode = context->md++;
	context->modetab[context->md++] = type;
	context->modetab[context->md++] = elemtype; // ссылка на элемент

	return check_duplicates(context);
}

int evaluate_params(compiler_context *context, int num, int formatstr[], int formattypes[], int placeholders[])
{
	int numofparams = 0;
	int i = 0;
	int fsi;

	//	for (i=0; i<num; i++)
	//		printf("%c %i\n", formatstr[i], formatstr[i]);

	for (i = 0; i < num; i++)
	{
		if (formatstr[i] == '%')
		{
			if (fsi = formatstr[++i], fsi != '%')
			{
				if (numofparams == MAXPRINTFPARAMS)
				{
					error(context, too_many_printf_params);
				}

				placeholders[numofparams] = fsi;
			}
			switch (fsi) // Если добавляется новый спецификатор -- не забыть
						 // внести его в switch в bad_printf_placeholder
			{
				case 'i':
				case 1094: // 'ц'
					formattypes[numofparams++] = LINT;
					break;

				case 'c':
				case 1083: // л
					formattypes[numofparams++] = LCHAR;
					break;

				case 'f':
				case 1074: // в
					formattypes[numofparams++] = LFLOAT;
					break;

				case 's':
				case 1089: // с
					formattypes[numofparams++] = newdecl(context, MARRAY, LCHAR);
					break;

				case '%':
					break;

				case 0:
					error(context, printf_no_format_placeholder);
					break;

				default:
					context->bad_printf_placeholder = fsi;
					error(context, printf_unknown_format_placeholder);
					break;
			}
		}
	}

	return numofparams;
}

int szof(compiler_context *context, int type)
{
	return context->next == LEFTSQBR
			   ? 1
			   : type == LFLOAT ? 2 : (type > 0 && context->modetab[type] == MSTRUCT) ? context->modetab[type + 1] : 1;
}

int is_row_of_char(compiler_context *context, int t)
{
	return t > 0 && context->modetab[t] == MARRAY && context->modetab[t + 1] == LCHAR;
}

int is_function(compiler_context *context, int t)
{
	return t > 0 && context->modetab[t] == MFUNCTION;
}

int is_array(compiler_context *context, int t)
{
	return t > 0 && context->modetab[t] == MARRAY;
}

int is_pointer(compiler_context *context, int t)
{
	return t > 0 && context->modetab[t] == MPOINT;
}

int is_struct(compiler_context *context, int t)
{
	return t > 0 && context->modetab[t] == MSTRUCT;
}

int is_float(compiler_context *context, int t)
{
	UNUSED(context);
	return t == LFLOAT || t == LDOUBLE;
}

int is_int(compiler_context *context, int t)
{
	UNUSED(context);
	return t == LINT || t == LLONG || t == LCHAR;
}

void mustbe(compiler_context *context, int what, int e)
{
	if (scaner(context) != what)
	{
		error(context, e);
	}
}

void totree(compiler_context *context, int op)
{
	context->tree[context->tc++] = op;
}

void totreef(compiler_context *context, int op)
{
	context->tree[context->tc++] = op;
	if (context->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		context->tree[context->tc - 1] += 50;
	}
}

int getstatic(compiler_context *context, int type)
{
	int olddispl = context->displ;
	context->displ += context->lg * szof(context, type); // lg - смещение от l (+1) или от g (-1)
	if (context->lg > 0)
	{
		context->maxdispl = (context->displ > context->maxdispl) ? context->displ : context->maxdispl;
	}
	else
	{
		context->maxdisplg = -context->displ;
	}
	return olddispl;
}

int toidentab(compiler_context *context, int f, int type)
{
	// f =  0, если не ф-ция, f=1, если метка, f=funcnum,
	// если описание ф-ции,
	// f = -1, если ф-ция-параметр, f>=1000, если это описание типа
	// f = -2, #define

	// printf("\n f= %i context->repr %i rtab[context->repr] %i
	// rtab[context->repr+1] %i rtab[context->repr+2] %i\n", f,
	// context->repr, context->reprtab[context->repr],
	// context->reprtab[context->repr+1], context->reprtab[context->repr+2]);
	int pred;

	compiler_table_expand(&context->reprtab, 1);

	context->lastid = context->id;
	if (REPRTAB[REPRTAB_POS + 1] == 0) // это может быть только MAIN
	{
		if (context->wasmain)
		{
			error(context, more_than_1_main);
		}
		context->wasmain = context->id;
	}

	// ссылка на описание с таким же
	// представлением в предыдущем блоке
	pred = context->identab[context->id] = REPRTAB[REPRTAB_POS + 1];
	if (pred)
	{
		// pred == 0 только для main, эту ссылку портить нельзя
		// ссылка на текущее описание с этим представлением
		// (это в reprtab)
		REPRTAB[REPRTAB_POS + 1] = context->id;
	}

	if (f != 1 && pred >= context->curid) // один  и тот же идент м.б. переменной и меткой
	{
		if (context->func_def == 3 ? 1 : context->identab[pred + 1] > 0 ? 1 : context->func_def == 1 ? 0 : 1)
		{
			error(context, repeated_decl);
			// только определение функции может иметь 2
			// описания, т.е. иметь предописание
		}
	}

	context->identab[context->id + 1] = REPRTAB_POS; // ссылка на представление
	if (f == -2)									 // #define
	{
		context->identab[context->id + 2] = 1;
		context->identab[context->id + 3] = type; // это целое число, определенное по #define
	}
	else // дальше тип или ссылка на modetab (для функций и структур)
	{
		context->identab[context->id + 2] = type; // тип -1 int, -2 char, -3 float, -4 long, -5 double,
												  // если тип > 0, то это ссылка на modetab
		if (f == 1)
		{
			context->identab[context->id + 2] = 0; // 0, если первым встретился goto, когда встретим метку,
												   // поставим 1
			context->identab[context->id + 3] = 0; // при генерации кода когда встретим метку, поставим pc
		}
		else if (f >= 1000)
		{
			context->identab[context->id + 3] = f; // это описание типа, если f > 1000, то f-1000 - это номер
												   // иниц проц
		}
		else if (f)
		{
			if (f < 0)
			{
				context->identab[context->id + 3] = -(context->displ++);
				context->maxdispl = context->displ;
			}
			else // identtab[context->lastid+3] - номер функции, если < 0, то
				 // это функция-параметр
			{
				context->identab[context->id + 3] = f;
				if (context->func_def == 2)
				{
					context->identab[context->lastid + 1] *= -1; //это предописание
					context->predef[++context->prdf] = REPRTAB_POS;
				}
				else
				{
					int i;

					for (i = 0; i <= context->prdf; i++)
					{
						if (context->predef[i] == REPRTAB_POS)
						{
							context->predef[i] = 0;
						}
					}
				}
			}
		}
		else
		{
			context->identab[context->id + 3] = getstatic(context, type);
		}
	}
	context->id += 4;
	return context->lastid;
}

void binop(compiler_context *context, int sp)
{
	int op = context->stackop[sp];
	int rtype = context->stackoperands[context->sopnd--];
	int ltype = context->stackoperands[context->sopnd];

	if (is_pointer(context, ltype) || is_pointer(context, rtype))
	{
		error(context, operand_is_pointer);
	}
	if ((op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND || op == LSHL || op == LSHR ||
		 op == LREM) &&
		(is_float(context, ltype) || is_float(context, rtype)))
	{
		error(context, int_op_for_float);
	}
	if (is_int(context, ltype) && is_float(context, rtype))
	{
		totree(context, WIDEN1);
	}
	if (is_int(context, rtype) && is_float(context, ltype))
	{
		totree(context, WIDEN);
	}
	if (is_float(context, ltype) || is_float(context, rtype))
	{
		context->ansttype = LFLOAT;
	}
	if (op == LOGOR || op == LOGAND)
	{
		totree(context, op);
		context->tree[context->stacklog[sp]] = context->tc++;
	}
	else
	{
		totreef(context, op);
	}
	if (op >= EQEQ && op <= LGE)
	{
		context->ansttype = LINT;
	}
	context->stackoperands[context->sopnd] = context->ansttype;
	// printf("binop context->sopnd=%i ltype=%i rtype=%i
	// context->ansttype=%i\n", context->sopnd, ltype, rtype,
	// context->ansttype);
	context->anst = VAL;
}

void toval(compiler_context *context)
{
	// надо значение положить на стек,
	// например, чтобы передать параметром

	if (context->anst == VAL || context->anst == NUMBER)
	{
		;
	}
	else if (is_struct(context, context->ansttype))
	{
		if (!context->inass)
		{
			if (context->anst == IDENT)
			{
				context->tc -= 2;
				totree(context, COPY0ST);
				totree(context, context->anstdispl);
			}
			else // тут может быть только ADDR
			{
				totree(context, COPY1ST);
			}
			totree(context, context->modetab[context->ansttype + 1]);
			context->anst = VAL;
		}
	}
	else
	{
		if (context->anst == IDENT)
		{
			context->tree[context->tc - 2] = is_float(context, context->ansttype) ? TIdenttovald : TIdenttoval;
		}

		if (!(is_array(context, context->ansttype) || is_pointer(context, context->ansttype)))
		{
			if (context->anst == ADDR)
			{
				totree(context, is_float(context, context->ansttype) ? TAddrtovald : TAddrtoval);
			}
		}
		context->anst = VAL;
	}
}

void insertwiden(compiler_context *context)
{
	context->tc--;
	totree(context, WIDEN);
	totree(context, TExprend);
}

void applid(compiler_context *context)
{
	compiler_table_expand(&context->reprtab, 1);
	context->lastid = REPRTAB[REPRTAB_POS + 1];
	if (context->lastid == 1)
	{
		error(context, ident_is_not_declared);
	}
}


void exprval(compiler_context *context);
void unarexpr(compiler_context *context);

void actstring(compiler_context *context)
{
	int n = 0;
	int adn;

	totree(context, TString);
	adn = context->tc++;
	do
	{
		/*
			if (scaner(context) == IDENT)
			{
				applid(context);
				if (context->identab[context->lastid+2] == 1)
					context->cur = NUMBER, context->ansttype = LINT, num =
				context->identab[context->lastid+3];
			}
		*/
		if (scaner(context) == NUMBER && (context->ansttype == LINT || context->ansttype == LCHAR))
		{
			totree(context, context->num);
		}
		else
		{
			error(context, wrong_init_in_actparam);
		}
		++n;
	} while (scaner(context) == COMMA);

	context->tree[adn] = n;
	if (context->cur != END)
	{
		error(context, no_comma_or_end);
	}
	context->ansttype = newdecl(context, MARRAY, LINT);
	context->anst = VAL;
}

void mustbestring(compiler_context *context)
{
	scaner(context);
	exprassn(context, 1);
	toval(context);
	context->sopnd--;
	if (!(context->ansttype > 0 && context->modetab[context->ansttype] == MARRAY &&
		  context->modetab[context->ansttype + 1] == LCHAR))
	{
		error(context, not_string_in_stanfunc);
	}
}

void mustbepointstring(compiler_context *context)
{
	scaner(context);
	exprassn(context, 1);
	toval(context);
	context->sopnd--;
	if (!(context->ansttype > 0 && context->modetab[context->ansttype] == MPOINT &&
		  is_array(context, context->modetab[context->ansttype + 1]) &&
		  context->modetab[context->modetab[context->ansttype + 1] + 1] == LCHAR))
	{
		error(context, not_point_string_in_stanfunc);
	}
}

void mustberow(compiler_context *context)
{
	scaner(context);
	exprassn(context, 1);
	toval(context);
	context->sopnd--;

	if (!(context->ansttype > 0 && context->modetab[context->ansttype] == MARRAY))
	{
		error(context, not_array_in_stanfunc);
	}
}

void mustbeint(compiler_context *context)
{
	scaner(context);
	exprassn(context, 1);
	toval(context);
	context->sopnd--;
	if (context->ansttype != LINT && context->ansttype != LCHAR)
	{
		error(context, not_int_in_stanfunc);
	}
}

void mustberowofint(compiler_context *context)
{
	scaner(context);
	if (context->cur == BEGIN)
	{
		actstring(context), totree(context, TExprend);
	}
	else
	{
		exprassn(context, 1);
		toval(context);
		context->sopnd--;
	}
	if (!(context->ansttype > 0 && context->modetab[context->ansttype] == MARRAY &&
		  (context->modetab[context->ansttype + 1] == LINT || context->modetab[context->ansttype + 1] == LCHAR)))
	{
		error(context, not_rowofint_in_stanfunc);
	}
}

void primaryexpr(compiler_context *context)
{
	if (context->cur == NUMBER)
	{
		if (context->ansttype == LFLOAT) // context->ansttype задается прямо в сканере
		{
			totree(context, TConstd);
			totree(context, context->numr.first);
			totree(context, context->numr.second);
		}
		else
		{
			totree(context, TConst);
			totree(context, context->num); // LINT, LCHAR
		}
		context->stackoperands[++context->sopnd] = context->ansttype;
		// printf("number context->sopnd=%i context->ansttype=%i\n",
		// context->sopnd, context->ansttype);
		context->anst = NUMBER;
	}
	else if (context->cur == STRING)
	{
		int i;

		context->ansttype = newdecl(context, MARRAY, LCHAR); // теперь пишем context->ansttype в
															 // анализаторе, а не в сканере
		totree(context, TString);
		totree(context, context->num);

		for (i = 0; i < context->num; i++)
		{
			totree(context, context->lexstr[i]);
		}

		context->stackoperands[++context->sopnd] = context->ansttype; // ROWOFCHAR
		context->anst = VAL;
	}
	else if (context->cur == IDENT)
	{
		applid(context);
		/*
			if (context->identab[context->lastid+2] == 1) // #define
			{
				totree(context, TConst);
				totree(context, num = context->identab[context->lastid+3]);
				context->anst = NUMBER;
				context->ansttype = LINT;
			}
			else
		*/
		{
			totree(context, TIdent);
			totree(context, context->anstdispl = context->identab[context->lastid + 3]);
			context->stackoperands[++context->sopnd] = context->ansttype = context->identab[context->lastid + 2];
			context->anst = IDENT;
		}
	}
	else if (context->cur == LEFTBR)
	{
		if (context->next == LVOID)
		{
			scaner(context);
			mustbe(context, LMULT, no_mult_in_cast);
			unarexpr(context);
			if (!is_pointer(context, context->ansttype))
			{
				error(context, not_pointer_in_cast);
			}
			mustbe(context, RIGHTBR, no_rightbr_in_cast);
			toval(context);
			// totree(context, CASTC);
			totree(context, TExprend);
		}
		else
		{
			int oldsp = context->sp;
			scaner(context);
			expr(context, 1);
			mustbe(context, RIGHTBR, wait_rightbr_in_primary);
			while (context->sp > oldsp)
			{
				binop(context, --context->sp);
			}
		}
	}
	else if (context->cur <= STANDARD_FUNC_START) // стандартная функция
	{
		int func = context->cur;

		if (scaner(context) != LEFTBR)
		{
			error(context, no_leftbr_in_stand_func);
		}
		if (func <= STRCPY && func >= STRLEN) // функции работы со строками
		{
			if (func >= STRNCAT)
			{
				mustbepointstring(context);
			}
			else
			{
				mustbestring(context);
			}
			if (func != STRLEN)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustbestring(context);
				if (func == STRNCPY || func == STRNCAT || func == STRNCMP)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
				}
			}
			if (func < STRNCAT)
			{
				context->stackoperands[++context->sopnd] = context->ansttype = LINT;
			}
		}
		else if (func >= ICON && func <= WIFI_CONNECT) // функции Фадеева
		{
			context->notrobot = 0;
			if (func <= PIXEL && func >= ICON)
			{
				mustberowofint(context);
				if (func != CLEAR)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				}

				if (func == LINE || func == RECTANGLE || func == ELLIPS)
				{
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (func != LINE)
					{
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
					}
				}
				else if (func == ICON || func == PIXEL)
				{
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (func == ICON)
					{
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
					}
				}
				else if (func == DRAW_NUMBER || func == DRAW_STRING)
				{
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					if (func == DRAW_STRING)
					{
						mustbestring(context);
					}
					else // DRAW_NUMBER
					{
						scaner(context);
						exprassn(context, 1);
						toval(context);
						context->sopnd--;
						if (is_int(context, context->ansttype))
						{
							totree(context, WIDEN);
						}
						else if (context->ansttype != LFLOAT)
						{
							error(context, not_float_in_stanfunc);
						}
					}
				}
			}
			else if (func == SETSIGNAL)
			{
				mustbeint(context);
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustberowofint(context);
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustberowofint(context);
			}
			else if (func == WIFI_CONNECT || func == BLYNK_AUTORIZATION || func == BLYNK_NOTIFICATION)
			{
				mustbestring(context);
				if (func == WIFI_CONNECT)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbestring(context);
				}
			}
			else
			{
				mustbeint(context);
				if (func != BLYNK_RECEIVE)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					if (func == BLYNK_TERMINAL)
					{
						mustbestring(context);
					}
					else if (func == BLYNK_SEND)
					{
						mustbeint(context);
					}
					else if (func == BLYNK_PROPERTY)
					{
						mustbestring(context);
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbestring(context);
					}
					else // BLYNK_LCD
					{
						mustbeint(context);
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbestring(context);
					}
				}
				else
				{
					context->stackoperands[++context->sopnd] = context->ansttype = LINT;
				}
			}
		}
		else if (func == UPB) // UPB
		{
			mustbeint(context);
			mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
			mustberow(context);
			context->stackoperands[++context->sopnd] = context->ansttype = LINT;
		}
		else if (func <= TMSGSEND && func >= TGETNUM) // процедуры управления параллельными нитями
		{
			if (func == TINIT || func == TDESTROY || func == TEXIT)
			{
				; // void()
			}
			else if (func == TMSGRECEIVE || func == TGETNUM) // getnum int()   msgreceive msg_info()
			{
				context->anst = VAL;
				context->ansttype = context->stackoperands[++context->sopnd] =
					func == TGETNUM ? LINT : 2; // 2 - это ссылка на msg_info
												//не было параметра,  выдали 1 результат
			}
			else
			{
				// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
				// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
				// у этих процедур 1 параметр
				scaner(context);

				if (func == TCREATE)
				{
					int dn;

					if (context->cur != IDENT)
					{
						error(context, act_param_not_ident);
					}
					applid(context);
					if (context->identab[context->lastid + 2] != 15) // 15 - это аргумент типа void* (void*)
					{
						error(context, wrong_arg_in_create);
					}

					context->stackoperands[context->sopnd] = context->ansttype = LINT;
					dn = context->identab[context->lastid + 3];
					if (dn < 0)
					{
						totree(context, TIdenttoval);
						totree(context, -dn);
					}
					else
					{
						totree(context, TConst);
						totree(context, dn);
					}
					context->anst = VAL;
				}
				else
				{
					context->leftansttype = 2;
					exprassn(context, 1);
					toval(context);

					if (func == TMSGSEND)
					{
						if (context->ansttype != 2) // 2 - это аргумент типа msg_info (struct{int numTh; int data;})
						{
							error(context, wrong_arg_in_send);
						}
						--context->sopnd;
					}
					else
					{
						if (!is_int(context, context->ansttype))
						{
							error(context, param_threads_not_int);
						}
						if (func == TSEMCREATE)
						{
							context->anst = VAL,
							context->ansttype = context->stackoperands[context->sopnd] =
								LINT; // съели 1 параметр, выдали int
						}
						else
						{
							--context->sopnd; // съели 1 параметр, не выдали
						}
						// результата
					}
				}
			}
		}
		else if (func == RAND)
		{
			context->ansttype = context->stackoperands[++context->sopnd] = LFLOAT;
		}
		else
		{
			scaner(context);
			exprassn(context, 1);
			toval(context);

			// GETDIGSENSOR int(int port, int pins[]),
			// GETANSENSOR int (int port, int pin),
			// SETMOTOR и VOLTAGE void (int port, int volt)
			if (func == GETDIGSENSOR || func == GETANSENSOR || func == SETMOTOR || func == VOLTAGE)
			{
				context->notrobot = 0;
				if (!is_int(context, context->ansttype))
				{
					error(context, param_setmotor_not_int);
				}
				mustbe(context, COMMA, no_comma_in_setmotor);
				scaner(context);
				if (func == GETDIGSENSOR)
				{
					if (context->cur == BEGIN)
					{
						context->sopnd--, actstring(context);
					}
					else
					{
						error(context, getdigsensorerr);
					}
				}
				else
				{
					exprassn(context, 1);
					toval(context);
					if (!is_int(context, context->ansttype))
					{
						error(context, param_setmotor_not_int);
					}
					if (func == SETMOTOR || func == VOLTAGE)
					{
						context->sopnd -= 2;
					}
					else
					{
						--context->sopnd, context->anst = VAL;
					}
				}
				if (func == SETMOTOR || func == VOLTAGE)
				{
					context->sopnd -= 2;
				}
				else
				{
					context->anst = VAL, --context->sopnd;
				}
			}
			else if (func == ABS && is_int(context, context->ansttype))
			{
				func = ABSI;
			}
			else
			{
				if (is_int(context, context->ansttype))
				{
					totree(context, WIDEN);
					context->ansttype = context->stackoperands[context->sopnd] = LFLOAT;
				}
				if (!is_float(context, context->ansttype))
				{
					error(context, bad_param_in_stand_func);
				}
				if (func == ROUND)
				{
					context->ansttype = context->stackoperands[context->sopnd] = LINT;
				}
			}
		}
		totree(context, 9500 - func);
		mustbe(context, RIGHTBR, no_rightbr_in_stand_func);
	}
	else
	{
		error(context, not_primary);
	}
}

void index_check(compiler_context *context)
{
	if (!is_int(context, context->ansttype))
	{
		error(context, index_must_be_int);
	}
}

int find_field(compiler_context *context, int stype)
{
	// выдает смещение до найденного поля или ошибку

	int i;
	int flag = 1;
	int select_displ = 0;

	scaner(context);
	mustbe(context, IDENT, after_dot_must_be_ident);

	for (i = 0; i < context->modetab[stype + 2]; i += 2) // тут хранится удвоенное n
	{
		int field_type = context->modetab[stype + 3 + i];

		if (context->modetab[stype + 4 + i] == REPRTAB_POS)
		{
			context->stackoperands[context->sopnd] = context->ansttype = field_type;
			flag = 0;
			break;
		}
		else
		{
			select_displ += szof(context, field_type);
		}
		// прибавляем к суммарному смещению длину поля
	}
	if (flag)
	{
		error(context, no_field);
	}
	return select_displ;
}

void selectend(compiler_context *context)
{
	while (context->next == DOT)
	{
		context->anstdispl += find_field(context, context->ansttype);
	}

	totree(context, context->anstdispl);
	if (is_array(context, context->ansttype) || is_pointer(context, context->ansttype))
	{
		totree(context, TAddrtoval);
	}
}

int Norder(compiler_context *context, int t)
{
	// вычислить размерность массива

	int n = 1;
	while ((t = context->modetab[t + 1]) > 0)
	{
		n++;
	}
	return n;
}

void array_init(compiler_context *context, int t);

void postexpr(compiler_context *context)
{
	int lid;
	int leftansttyp;
	int was_func = 0;

	lid = context->lastid;
	leftansttyp = context->ansttype;

	if (context->next == LEFTBR) // вызов функции
	{
		int i;
		int j;
		int n;
		int dn;
		int oldinass = context->inass;

		was_func = 1;
		scaner(context);
		if (!is_function(context, leftansttyp))
		{
			error(context, call_not_from_function);
		}

		n = context->modetab[leftansttyp + 2]; // берем количество аргументов функции

		totree(context, TCall1);
		totree(context, n);
		j = leftansttyp + 3;
		for (i = 0; i < n; i++) // фактические параметры
		{
			int mdj = context->leftansttype = context->modetab[j]; // это вид формального параметра, в
																   // context->ansttype будет вид фактического
																   // параметра
			scaner(context);
			if (is_function(context, mdj))
			{
				// фактическим параметром должна быть функция, в С - это только идентификатор

				if (context->cur != IDENT)
				{
					error(context, act_param_not_ident);
				}
				applid(context);
				if (context->identab[context->lastid + 2] != mdj)
				{
					error(context, diff_formal_param_type_and_actual);
				}
				dn = context->identab[context->lastid + 3];
				if (dn < 0)
				{
					totree(context, TIdenttoval);
					totree(context, -dn);
				}
				else
				{
					totree(context, TConst);
					totree(context, dn);
				}
				totree(context, TExprend);
			}
			else
			{
				if (context->cur == BEGIN && is_array(context, mdj))
				{
					actstring(context), totree(context, TExprend);
				}
				else
				{
					context->inass = 0;
					exprassn(context, 1);
					toval(context);
					totree(context, TExprend);

					if (mdj > 0 && mdj != context->ansttype)
					{
						error(context, diff_formal_param_type_and_actual);
					}

					if (is_int(context, mdj) && is_float(context, context->ansttype))
					{
						error(context, float_instead_int);
					}

					if (is_float(context, mdj) && is_int(context, context->ansttype))
					{
						insertwiden(context);
					}
					--context->sopnd;
				}
			}
			if (i < n - 1 && scaner(context) != COMMA)
			{
				error(context, no_comma_in_act_params);
			}
			j++;
		}
		context->inass = oldinass;
		mustbe(context, RIGHTBR, wrong_number_of_params);
		totree(context, TCall2);
		totree(context, lid);
		context->stackoperands[context->sopnd] = context->ansttype = context->modetab[leftansttyp + 1];
		context->anst = VAL;
		if (is_struct(context, context->ansttype))
		{
			context->x -= context->modetab[context->ansttype + 1] - 1;
		}
	}

	while (context->next == LEFTSQBR || context->next == ARROW || context->next == DOT)
	{
		while (context->next == LEFTSQBR) // вырезка из массива (возможно, многомерного)
		{
			int elem_type;
			if (was_func)
			{
				error(context, slice_from_func);
			}
			if (context->modetab[context->ansttype] != MARRAY) // вырезка не из массива
			{
				error(context, slice_not_from_array);
			}

			elem_type = context->modetab[context->ansttype + 1];

			scaner(context);
			scaner(context);

			if (context->anst == IDENT) // a[i]
			{
				context->tree[context->tc - 2] = TSliceident;
				context->tree[context->tc - 1] = context->anstdispl;
			}
			else // a[i][j]
			{
				totree(context, TSlice);
			}

			totree(context, elem_type);
			exprval(context);
			index_check(context); // проверка, что индекс int или char

			mustbe(context, RIGHTSQBR, no_rightsqbr_in_slice);

			context->stackoperands[--context->sopnd] = context->ansttype = elem_type;
			context->anst = ADDR;
		}

		while (context->next == ARROW)
		{
			// это выборка поля из указателя на структуру, если после
			// -> больше одной точки подряд, схлопываем в 1 select
			// перед выборкой мог быть вызов функции или вырезка элемента массива

			if (context->modetab[context->ansttype] != MPOINT ||
				context->modetab[context->modetab[context->ansttype + 1]] != MSTRUCT)
			{
				error(context, get_field_not_from_struct_pointer);
			}

			if (context->anst == IDENT)
			{
				context->tree[context->tc - 2] = TIdenttoval;
			}
			context->anst = ADDR;
			// pointer  мог быть значением функции (VAL) или, может быть,
			totree(context, TSelect); // context->anst уже был ADDR, т.е. адрес
									  // теперь уже всегда на верхушке стека

			context->anstdispl = find_field(context, context->ansttype = context->modetab[context->ansttype + 1]);
			selectend(context);
		}
		if (context->next == DOT)

		{
			if (context->ansttype < 0 || context->modetab[context->ansttype] != MSTRUCT)
			{
				error(context, select_not_from_struct);
			}
			if (context->anst == VAL) // структура - значение функции
			{
				int len1 = szof(context, context->ansttype);
				context->anstdispl = 0;
				while (context->next == DOT)
				{
					context->anstdispl += find_field(context, context->ansttype);
				}
				totree(context, COPYST);
				totree(context, context->anstdispl);
				totree(context, szof(context, context->ansttype));
				totree(context, len1);
			}
			else if (context->anst == IDENT)
			{
				int globid = context->anstdispl < 0 ? -1 : 1;
				while (context->next == DOT)
				{
					context->anstdispl += globid * find_field(context, context->ansttype);
				}
				context->tree[context->tc - 1] = context->anstdispl;
			}
			else // ADDR
			{
				totree(context, TSelect);
				context->anstdispl = 0;
				selectend(context);
			}
		}
	}
	if (context->next == INC || context->next == DEC) // a++, a--
	{
		int op;

		if (!is_int(context, context->ansttype) && !is_float(context, context->ansttype))
		{
			error(context, wrong_operand);
		}

		if (context->anst != IDENT && context->anst != ADDR)
		{
			error(context, unassignable_inc);
		}
		op = (context->next == INC) ? POSTINC : POSTDEC;
		if (context->anst == ADDR)
		{
			op += 4;
		}
		scaner(context);
		totreef(context, op);
		if (context->anst == IDENT)
		{
			totree(context, context->identab[lid + 3]);
		}
		context->anst = VAL;
	}
}

void unarexpr(compiler_context *context)
{
	int op = context->cur;
	if (context->cur == LNOT || context->cur == LOGNOT || context->cur == LPLUS || context->cur == LMINUS ||
		context->cur == LAND || context->cur == LMULT || context->cur == INC || context->cur == DEC)
	{
		if (context->cur == INC || context->cur == DEC)
		{
			scaner(context);
			unarexpr(context);
			if (context->anst != IDENT && context->anst != ADDR)
			{
				error(context, unassignable_inc);
			}
			if (context->anst == ADDR)
			{
				op += 4;
			}
			totreef(context, op);
			if (context->anst == IDENT)
			{
				totree(context, context->identab[context->lastid + 3]);
			}
			context->anst = VAL;
		}
		else
		{
			scaner(context);
			unarexpr(context);

			if (op == LAND)
			{
				if (context->anst == VAL)
				{
					error(context, wrong_addr);
				}

				if (context->anst == IDENT)
				{
					context->tree[context->tc - 2] = TIdenttoaddr; // &a
				}

				context->stackoperands[context->sopnd] = context->ansttype =
					newdecl(context, MPOINT, context->ansttype);
				context->anst = VAL;
			}
			else if (op == LMULT)
			{
				if (!is_pointer(context, context->ansttype))
				{
					error(context, aster_not_for_pointer);
				}

				if (context->anst == IDENT)
				{
					context->tree[context->tc - 2] = TIdenttoval; // *p
				}

				context->stackoperands[context->sopnd] = context->ansttype = context->modetab[context->ansttype + 1];
				context->anst = ADDR;
			}
			else
			{
				toval(context);
				if ((op == LNOT || op == LOGNOT) && context->ansttype == LFLOAT)
				{
					error(context, int_op_for_float);
				}
				else if (op == LMINUS)
				{
					totreef(context, UNMINUS);
				}
				else if (op == LPLUS)
				{
					;
				}
				else
				{
					totree(context, op);
				}
				context->anst = VAL;
			}
		}
	}
	else
	{
		primaryexpr(context);
	}

	postexpr(context);
	context->stackoperands[context->sopnd] = context->ansttype;
}

void exprinbrkts(compiler_context *context, int er)
{
	mustbe(context, LEFTBR, er);
	scaner(context);
	exprval(context);
	mustbe(context, RIGHTBR, er);
}

void exprassninbrkts(compiler_context *context, int er)
{
	mustbe(context, LEFTBR, er);
	scaner(context);
	exprassnval(context);
	mustbe(context, RIGHTBR, er);
}

int prio(compiler_context *context, int op)
{
	// возвращает 0, если не операция

	UNUSED(context);
	return op == LOGOR
			   ? 1
			   : op == LOGAND
					 ? 2
					 : op == LOR
						   ? 3
						   : op == LEXOR
								 ? 4
								 : op == LAND
									   ? 5
									   : op == EQEQ
											 ? 6
											 : op == NOTEQ
												   ? 6
												   : op == LLT
														 ? 7
														 : op == LGT
															   ? 7
															   : op == LLE
																	 ? 7
																	 : op == LGE
																		   ? 7
																		   : op == LSHL
																				 ? 8
																				 : op == LSHR
																					   ? 8
																					   : op == LPLUS
																							 ? 9
																							 : op == LMINUS
																								   ? 9
																								   : op == LMULT
																										 ? 10
																										 : op == LDIV
																											   ? 10
																											   : op == LREM
																													 ? 10
																													 : 0;
}

void subexpr(compiler_context *context)
{
	int p;
	int oldsp = context->sp;
	int wasop = 0;
	int ad = 0;

	while ((p = prio(context, context->next)))
	{
		wasop = 1;
		toval(context);
		while (context->sp > oldsp && context->stack[context->sp - 1] >= p)
		{
			binop(context, --context->sp);
		}

		if (p <= 2)
		{
			totree(context, p == 1 ? ADLOGOR : ADLOGAND);
			ad = context->tc++;
		}

		context->stack[context->sp] = p;
		context->stacklog[context->sp] = ad;
		context->stackop[context->sp++] = context->next;
		scaner(context);
		scaner(context);
		unarexpr(context);
	}
	if (wasop)
	{
		toval(context);
	}
	while (context->sp > oldsp)
	{
		binop(context, --context->sp);
	}
}

int intopassn(compiler_context *context, int next)
{
	UNUSED(context);
	return next == REMASS || next == SHLASS || next == SHRASS || next == ANDASS || next == EXORASS || next == ORASS;
}

int opassn(compiler_context *context)
{
	return (context->next == ASS || context->next == MULTASS || context->next == DIVASS || context->next == PLUSASS ||
			context->next == MINUSASS || intopassn(context, context->next))
			   ? context->op = context->next
			   : 0;
}

void condexpr(compiler_context *context)
{
	int globtype = 0;
	int adif = 0;
	int r;

	subexpr(context); // logORexpr();
	if (context->next == QUEST)
	{
		while (context->next == QUEST)
		{
			toval(context);
			if (!is_int(context, context->ansttype))
			{
				error(context, float_in_condition);
			}
			totree(context, TCondexpr);
			scaner(context);
			scaner(context);
			context->sopnd--;
			exprval(context); // then
			if (!globtype)
			{
				globtype = context->ansttype;
			}
			context->sopnd--;
			if (is_float(context, context->ansttype))
			{
				globtype = LFLOAT;
			}
			else
			{
				context->tree[context->tc] = adif;
				adif = context->tc++;
			}
			mustbe(context, COLON, no_colon_in_cond_expr);
			scaner(context);
			unarexpr(context);
			subexpr(context); // logORexpr();	else or elif
		}
		toval(context);
		totree(context, TExprend);
		if (is_float(context, context->ansttype))
		{
			globtype = LFLOAT;
		}
		else
		{
			context->tree[context->tc] = adif;
			adif = context->tc++;
		}

		while (adif != 0)
		{
			r = context->tree[adif];
			context->tree[adif] = TExprend;
			context->tree[adif - 1] = is_float(context, globtype) ? WIDEN : NOP;
			adif = r;
		}

		context->stackoperands[context->sopnd] = context->ansttype = globtype;
	}
	else
	{
		context->stackoperands[context->sopnd] = context->ansttype;
	}
}

void inition(compiler_context *context, int decl_type)
{
	if (decl_type < 0 || is_pointer(context, decl_type) || // Обработка для базовых типов, указателей
		(is_array(context, decl_type) && context->modetab[decl_type + 1] == LCHAR)) // или строк
	{
		exprassn(context, 1);
		toval(context);
		totree(context, TExprend);
		// съедаем выражение, его значение будет на стеке
		context->sopnd--;
		if (is_int(context, decl_type) && is_float(context, context->ansttype))
		{
			error(context, init_int_by_float);
		}
		if (is_float(context, decl_type) && is_int(context, context->ansttype))
		{
			insertwiden(context);
		}
		else if (decl_type != context->ansttype)
		{
			error(context, error_in_initialization);
		}
	}
	else if (context->cur == BEGIN)
	{
		struct_init(context, decl_type);
	}
	else
	{
		error(context, wrong_init);
	}
}

void struct_init(compiler_context *context, int decl_type)
{
	// сейчас context->modetab[decl_type] равен MSTRUCT

	int next_field = decl_type + 3;
	int i;
	int nf = context->modetab[decl_type + 2] / 2;

	if (context->cur != BEGIN)
	{
		error(context, struct_init_must_start_from_BEGIN);
	}
	totree(context, TStructinit);
	totree(context, nf);
	for (i = 0; i < nf; i++)
	{
		scaner(context);
		inition(context, context->modetab[next_field]);
		next_field += 2;
		if (i != nf - 1)
		{
			if (context->next == COMMA) // поля инициализации идут через запятую, заканчиваются }
			{
				scaner(context);
			}
			else
			{
				error(context, no_comma_in_init_list);
			}
		}
	}

	if (context->next == END)
	{
		totree(context, TExprend);
	}
	else
	{
		error(context, wait_end);
	}
	scaner(context);
	context->leftansttype = decl_type;
}

void exprassnvoid(compiler_context *context)
{
	int t = context->tree[context->tc - 2] < 9000 ? context->tc - 3 : context->tc - 2;
	int tt = context->tree[t];
	if ((tt >= ASS && tt <= DIVASSAT) || (tt >= POSTINC && tt <= DECAT) || (tt >= ASSR && tt <= DIVASSATR) ||
		(tt >= POSTINCR && tt <= DECATR))
	{
		context->tree[t] += 200;
	}
	--context->sopnd;
}

void exprassn(compiler_context *context, int level)
{
	int leftanst;
	int leftanstdispl;
	int ltype;
	int rtype;
	int lnext;

	if (context->cur == BEGIN)
	{
		if (is_struct(context, context->leftansttype))
		{
			struct_init(context, context->leftansttype);
		}
		// else if (is_array(context, context->leftansttype))
		// пока в RuC присваивать массивы нельзя
		// array_init(context, context->leftansttype);
		else
		{
			error(context, init_not_struct);
		}
		context->stackoperands[++context->sopnd] = context->ansttype = context->leftansttype;
		context->anst = VAL;
	}
	else
	{
		unarexpr(context);
	}

	leftanst = context->anst;
	leftanstdispl = context->anstdispl;
	context->leftansttype = context->ansttype;
	if (opassn(context))
	{
		int opp = context->op;
		lnext = context->next;
		context->inass = 1;
		scaner(context);
		scaner(context);
		exprassn(context, level + 1);
		context->inass = 0;

		if (leftanst == VAL)
		{
			error(context, unassignable);
		}
		rtype = context->stackoperands[context->sopnd--]; // снимаем типы
														  // операндов со стека
		ltype = context->stackoperands[context->sopnd];

		if (intopassn(context, lnext) && (is_float(context, ltype) || is_float(context, rtype)))
		{
			error(context, int_op_for_float);
		}

		if (is_array(context, ltype)) // присваивать массив в массив в си нельзя
		{
			error(context, array_assigment);
		}

		if (is_struct(context, ltype)) // присваивание в структуру
		{
			if (ltype != rtype) // типы должны быть равны
			{
				error(context, type_missmatch);
			}
			if (opp != ASS) // в структуру можно присваивать только с помощью =
			{
				error(context, wrong_struct_ass);
			}

			if (context->anst == VAL)
			{
				opp = leftanst == IDENT ? COPY0STASS : COPY1STASS;
			}
			else
			{
				opp = leftanst == IDENT ? context->anst == IDENT ? COPY00 : COPY01
										: context->anst == IDENT ? COPY10 : COPY11;
			}
			totree(context, opp);
			if (leftanst == IDENT)
			{
				totree(context, leftanstdispl); // displleft
			}
			if (context->anst == IDENT)
			{
				totree(context, context->anstdispl); // displright
			}
			totree(context, context->modetab[ltype + 1]); // длина
			context->anst = leftanst;
			context->anstdispl = leftanstdispl;
		}
		else // оба операнда базового типа или указатели
		{
			if (is_pointer(context, ltype) && opp != ASS) // в указатель можно присваивать только с помощью =
			{
				error(context, wrong_struct_ass);
			}

			if (is_int(context, ltype) && is_float(context, rtype))
			{
				error(context, assmnt_float_to_int);
			}

			toval(context);
			if (is_int(context, rtype) && is_float(context, ltype))
			{
				totree(context, WIDEN);
				context->ansttype = LFLOAT;
			}
			if (is_pointer(context, ltype) && is_pointer(context, rtype) && ltype != rtype)
			{
				// проверка нужна только для указателей
				error(context, type_missmatch);
			}

			if (leftanst == ADDR)
			{
				opp += 11;
			}
			totreef(context, opp);
			if (leftanst == IDENT)
			{
				totree(context, context->anstdispl = leftanstdispl);
			}
			context->anst = VAL;
		}
		context->stackoperands[context->sopnd] = context->ansttype = ltype; // тип результата - на стек
	}
	else
	{
		condexpr(context); // condexpr учитывает тот факт, что начало выражения
	}
	// в виде unarexpr уже выкушано
}

void expr(compiler_context *context, int level)
{
	exprassn(context, level);
	while (context->next == COMMA)
	{
		exprassnvoid(context);
		context->sopnd--;
		scaner(context);
		scaner(context);
		exprassn(context, level);
	}
	if (level == 0)
	{
		totree(context, TExprend);
	}
}

void exprval(compiler_context *context)
{
	expr(context, 1);
	toval(context);
	totree(context, TExprend);
}

void exprassnval(compiler_context *context)
{
	exprassn(context, 1);
	toval(context);
	totree(context, TExprend);
}

void array_init(compiler_context *context, int decl_type)
{
	// сейчас context->modetab[decl_type] равен MARRAY

	int ad;
	int all = 0;

	if (is_array(context, decl_type))
	{
		if (context->cur == STRING)
		{
			if (context->onlystrings == 0)
			{
				error(context, string_and_notstring);
			}
			if (context->onlystrings == 2)
			{
				context->onlystrings = 1;
			}
			primaryexpr(context);
			totree(context, TExprend);
		}
		else if (context->cur == BEGIN)
		{
			totree(context, TBeginit);
			ad = context->tc++;
			do
			{
				scaner(context);
				all++;
				array_init(context, context->modetab[decl_type + 1]);
			} while (scaner(context) == COMMA);

			if (context->cur == END)
			{
				context->tree[ad] = all;
				totree(context, TExprend);
			}
			else
			{
				error(context, wait_end);
			}
		}
		else
		{
			error(context, arr_init_must_start_from_BEGIN);
		}
	}
	else if (context->cur == BEGIN)
	{
		if (is_struct(context, decl_type))
		{
			struct_init(context, decl_type);
		}
		else
		{
			error(context, begin_with_notarray);
		}
	}
	else if (context->onlystrings == 1)
	{
		error(context, string_and_notstring);
	}
	else
	{
		inition(context, decl_type);
		context->onlystrings = 0;
	}
}

int arrdef(compiler_context *context, int t)
{
	// вызывается при описании массивов и структур из массивов сразу после idorpnt

	context->arrdim = 0;
	context->usual = 1; // описание массива без пустых границ
	if (is_pointer(context, t))
	{
		error(context, pnt_before_array);
	}

	while (context->next == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		context->arrdim++;
		scaner(context);
		if (context->next == RIGHTSQBR)
		{
			scaner(context);
			if (context->next == LEFTSQBR) // int a[][]={{1,2,3}, {4,5,6}} - нельзя;
			{
				error(context, empty_init); // границы определять по инициализации можно
			}
			// только по последнему изм.
			context->usual = 0;
		}
		else
		{
			scaner(context);
			unarexpr(context);
			condexpr(context);
			toval(context);
			if (!is_int(context, context->ansttype))
			{
				error(context, array_size_must_be_int);
			}
			totree(context, TExprend);
			context->sopnd--;
			mustbe(context, RIGHTSQBR, wait_right_sq_br);
		}
		t = newdecl(context, MARRAY, t); // Меняем тип в identtab (увеличиваем размерность массива)
	}
	return t;
}

void decl_id(compiler_context *context, int decl_type)
{
	// вызывается из block и extdecl, только эта процедура реально отводит память
	// если встретятся массивы (прямо или в структурах), их размеры уже будут в стеке

	int oldid = toidentab(context, 0, decl_type);
	int elem_len;
	int elem_type;
	int all; // all - место в дереве, где будет общее количество выражений в инициализации, для массивов - только
			 // признак (1) наличия инициализации
	int adN;

	context->usual = 1;
	context->arrdim = 0; // arrdim - размерность (0-скаляр), д.б. столько выражений-границ
	elem_type = decl_type;

	if (context->next == LEFTSQBR) // это определение массива (может быть многомерным)
	{
		totree(context, TDeclarr);
		adN = context->tc++;
		decl_type = context->identab[oldid + 2] =
			arrdef(context, decl_type); // Меняем тип (увеличиваем размерность массива)
		context->tree[adN] = context->arrdim;
		if (!context->usual && context->next != ASS)
		{
			error(context, empty_bound_without_init);
		}
	}
	totree(context, TDeclid);
	totree(context, context->identab[oldid + 3]);													  // displ
	totree(context, elem_type);																		  // elem_type
	totree(context, context->arrdim);																  // N
	context->tree[all = context->tc++] = 0;															  // all
	context->tree[context->tc++] = is_pointer(context, decl_type) ? 0 : context->was_struct_with_arr; // proc
	totree(context, context->usual);																  // context->usual
	totree(context, 0); // массив не в структуре

	if (context->next == ASS)
	{
		scaner(context);
		scaner(context);
		context->tree[all] = szof(context, decl_type);
		if (is_array(context, decl_type)) // инициализация массива
		{
			context->onlystrings = 2;
			if (!context->usual)
			{
				context->tree[adN]--; // это уменьшение N в Declarr
			}
			array_init(context, decl_type);
			if (context->onlystrings == 1)
			{
				context->tree[all + 2] = context->usual + 2; // только из строк 2 - без границ, 3 - с границами
			}
		}
		else
		{
			inition(context, decl_type);
		}
	}
}

void statement(compiler_context *context)
{
	int flagsemicol = 1;
	int oldwasdefault = context->wasdefault;
	int oldinswitch = context->inswitch;
	int oldinloop = context->inloop;

	context->wasdefault = 0;
	scaner(context);
	if ((is_int(context, context->cur) || is_float(context, context->cur) || context->cur == LVOID ||
		 context->cur == LSTRUCT) &&
		context->blockflag)
	{
		error(context, decl_after_strmt);
	}
	if (context->cur == BEGIN)
	{
		flagsemicol = 0;
		block(context, 1);
	}
	else if (context->cur == TCREATEDIRECT)
	{
		totree(context, CREATEDIRECTC);
		flagsemicol = 0;
		block(context, 2);
		totree(context, EXITC);
	}
	else if (context->cur == SEMICOLON)
	{
		totree(context, NOP);
		flagsemicol = 0;
	}
	else if (context->cur == IDENT && context->next == COLON)
	{
		int id;
		int i;
		int flag = 1;
		flagsemicol = 0;
		totree(context, TLabel);
		for (i = 0; flag && i < context->pgotost - 1; i += 2)
		{
			flag = context->identab[context->gotost[i] + 1] != REPRTAB_POS;
		}
		if (flag)
		{
			totree(context, id = toidentab(context, 1, 0));
			context->gotost[context->pgotost++] = id; // это определение метки, если она встретилась до
													  // переходов на нее
			context->gotost[context->pgotost++] = -context->line;
		}
		else
		{
			id = context->gotost[i - 2];
			REPRTAB_POS = context->identab[id + 1];
			if (context->gotost[i - 1] < 0)
			{
				error(context, repeated_label);
			}
			totree(context, id);
		}
		context->identab[id + 2] = 1;

		scaner(context);
		statement(context);
	}
	else
	{
		context->blockflag = 1;

		switch (context->cur)
		{
			case PRINT:
			{
				exprassninbrkts(context, print_without_br);
				context->tc--;
				totree(context, TPrint);
				totree(context, context->ansttype);
				totree(context, TExprend);
				if (is_pointer(context, context->ansttype))
				{
					error(context, pointer_in_print);
				}
				context->sopnd--;
			}
			break;
			case PRINTID:
			{
				mustbe(context, LEFTBR, no_leftbr_in_printid);
				mustbe(context, IDENT, no_ident_in_printid);
				compiler_table_expand(&context->reprtab, 1);
				context->lastid = REPRTAB[REPRTAB_POS + 1];
				if (context->lastid == 1)
				{
					error(context, ident_is_not_declared);
				}
				totree(context, TPrintid);
				totree(context, context->lastid);
				mustbe(context, RIGHTBR, no_rightbr_in_printid);
			}
			break;

			case PRINTF:
			{
				int formatstr[MAXSTRINGL];
				int formattypes[MAXPRINTFPARAMS];
				int placeholders[MAXPRINTFPARAMS];
				int paramnum = 0;
				int sumsize = 0;
				int i = 0;
				int fnum;

				mustbe(context, LEFTBR, no_leftbr_in_printf);
				if (scaner(context) != STRING) // выкушиваем форматную строку
				{
					error(context, wrong_first_printf_param);
				}

				for (i = 0; i < context->num; i++)
				{
					formatstr[i] = context->lexstr[i];
				}
				formatstr[context->num] = 0;

				paramnum = evaluate_params(context, fnum = context->num, formatstr, formattypes, placeholders);

				for (i = 0; scaner(context) == COMMA; i++)
				{
					if (i >= paramnum)
					{
						error(context, wrong_printf_param_number);
					}

					scaner(context);

					exprassn(context, 1);
					toval(context);
					totree(context, TExprend);

					if (formattypes[i] == LFLOAT && context->ansttype == LINT)
					{
						insertwiden(context);
					}
					else if (formattypes[i] != context->ansttype)
					{
						context->bad_printf_placeholder = placeholders[i];
						error(context, wrong_printf_param_type);
					}

					sumsize += szof(context, formattypes[i]);
					--context->sopnd;
				}

				if (context->cur != RIGHTBR)
				{
					error(context, no_rightbr_in_printf);
				}

				if (i != paramnum)
				{
					error(context, wrong_printf_param_number);
				}

				totree(context, TString);
				totree(context, fnum);

				for (i = 0; i < fnum; i++)
				{
					totree(context, formatstr[i]);
				}
				totree(context, TExprend);

				totree(context, TPrintf);
				totree(context, sumsize);
			}
			break;

			case GETID:
			{
				compiler_table_expand(&context->reprtab, 1);

				mustbe(context, LEFTBR, no_leftbr_in_printid);
				mustbe(context, IDENT, no_ident_in_printid);
				context->lastid = REPRTAB[REPRTAB_POS + 1];
				if (context->lastid == 1)
				{
					error(context, ident_is_not_declared);
				}
				totree(context, TGetid);
				totree(context, context->lastid);
				mustbe(context, RIGHTBR, no_rightbr_in_printid);
			}
			break;
			case LBREAK:
			{
				if (!(context->inloop || context->inswitch))
				{
					error(context, break_not_in_loop_or_switch);
				}
				totree(context, TBreak);
			}
			break;
			case LCASE:
			{
				if (!context->inswitch)
				{
					error(context, case_or_default_not_in_switch);
				}
				if (context->wasdefault)
				{
					error(context, case_after_default);
				}
				totree(context, TCase);
				scaner(context);
				unarexpr(context);
				condexpr(context);
				toval(context);
				totree(context, TExprend);
				if (context->ansttype == LFLOAT)
				{
					error(context, float_in_switch);
				}
				context->sopnd--;
				mustbe(context, COLON, no_colon_in_case);
				flagsemicol = 0;
				statement(context);
			}
			break;
			case LCONTINUE:
			{
				if (!context->inloop)
				{
					error(context, continue_not_in_loop);
				}
				totree(context, TContinue);
			}
			break;
			case LDEFAULT:
			{
				if (!context->inswitch)
				{
					error(context, case_or_default_not_in_switch);
				}
				mustbe(context, COLON, no_colon_in_case);
				context->wasdefault = 1;
				flagsemicol = 0;
				totree(context, TDefault);
				statement(context);
			}
			break;
			case LDO:
			{
				context->inloop = 1;
				totree(context, TDo);
				statement(context);
				if (context->next == LWHILE)
				{
					scaner(context);
					exprinbrkts(context, cond_must_be_in_brkts);
					context->sopnd--;
				}
				else
				{
					error(context, wait_while_in_do_stmt);
				}
			}
			break;
			case LFOR:
			{
				int fromref;
				int condref;
				int incrref;
				int stmtref;
				mustbe(context, LEFTBR, no_leftbr_in_for);
				totree(context, TFor);
				fromref = context->tc++;
				condref = context->tc++;
				incrref = context->tc++;
				stmtref = context->tc++;
				if (scaner(context) == SEMICOLON) // init
				{
					context->tree[fromref] = 0;
				}
				else
				{
					context->tree[fromref] = context->tc;
					expr(context, 0);
					exprassnvoid(context);
					mustbe(context, SEMICOLON, no_semicolon_in_for);
				}
				if (scaner(context) == SEMICOLON) // cond
				{
					context->tree[condref] = 0;
				}
				else
				{
					context->tree[condref] = context->tc;
					exprval(context);
					context->sopnd--;
					mustbe(context, SEMICOLON, no_semicolon_in_for);
					context->sopnd--;
				}
				if (scaner(context) == RIGHTBR) // incr
				{
					context->tree[incrref] = 0;
				}
				else
				{
					context->tree[incrref] = context->tc;
					expr(context, 0);
					exprassnvoid(context);
					mustbe(context, RIGHTBR, no_rightbr_in_for);
				}
				flagsemicol = 0;
				context->tree[stmtref] = context->tc;
				context->inloop = 1;
				statement(context);
			}
			break;
			case LGOTO:
			{
				int i;
				int flag = 1;
				mustbe(context, IDENT, no_ident_after_goto);
				totree(context, TGoto);
				for (i = 0; flag && i < context->pgotost - 1; i += 2)
				{
					flag = context->identab[context->gotost[i] + 1] != REPRTAB_POS;
				}
				if (flag)
				{
					// первый раз встретился переход на метку, которой не было,
					// в этом случае ссылка на identtab, стоящая после TGoto,
					// будет отрицательной
					totree(context, -toidentab(context, 1, 0));
					context->gotost[context->pgotost++] = context->lastid;
				}
				else
				{
					int id = context->gotost[i - 2];
					if (context->gotost[id + 1] < 0) // метка уже была
					{
						totree(context, id);
						break;
					}
					totree(context, context->gotost[context->pgotost++] = id);
				}
				context->gotost[context->pgotost++] = context->line;
			}
			break;
			case LIF:
			{
				int elseref;
				totree(context, TIf);
				elseref = context->tc++;
				flagsemicol = 0;
				exprinbrkts(context, cond_must_be_in_brkts);
				context->sopnd--;
				statement(context);
				if (context->next == LELSE)
				{
					scaner(context);
					context->tree[elseref] = context->tc;
					statement(context);
				}
				else
				{
					context->tree[elseref] = 0;
				}
			}
			break;
			case LRETURN:
			{
				int ftype = context->modetab[context->functype + 1];
				context->wasret = 1;
				if (context->next == SEMICOLON)
				{
					if (ftype != LVOID)
					{
						error(context, no_ret_in_func);
					}
					totree(context, TReturnvoid);
				}
				else
				{
					if (ftype == LVOIDASTER)
					{
						flagsemicol = 0;
					}
					else
					{
						if (ftype == LVOID)
						{
							error(context, notvoidret_in_void_func);
						}
						totree(context, TReturnval);
						totree(context, szof(context, ftype));
						scaner(context);
						expr(context, 1);
						toval(context);
						context->sopnd--;
						if (ftype == LFLOAT && context->ansttype == LINT)
						{
							totree(context, WIDEN);
						}
						else if (ftype != context->ansttype)
						{
							error(context, bad_type_in_ret);
						}
						totree(context, TExprend);
					}
				}
			}
			break;
			case LSWITCH:
			{
				totree(context, TSwitch);
				exprinbrkts(context, cond_must_be_in_brkts);
				if (context->ansttype != LCHAR && context->ansttype != LINT)
				{
					error(context, float_in_switch);
				}
				context->sopnd--;
				scaner(context);
				context->inswitch = 1;
				block(context, -1);
				flagsemicol = 0;
				context->wasdefault = 0;
			}
			break;
			case LWHILE:
			{
				context->inloop = 1;
				totree(context, TWhile);
				flagsemicol = 0;
				exprinbrkts(context, cond_must_be_in_brkts);
				context->sopnd--;
				statement(context);
			}
			break;
			default:
				expr(context, 0);
				exprassnvoid(context);
		}
	}
	if (flagsemicol && scaner(context) != SEMICOLON)
	{
		error(context, no_semicolon_after_stmt);
	}
	context->wasdefault = oldwasdefault;
	context->inswitch = oldinswitch;
	context->inloop = oldinloop;
}

int idorpnt(compiler_context *context, int e, int t)
{
	if (context->next == LMULT)
	{
		scaner(context);
		t = t == LVOID ? LVOIDASTER : newdecl(context, MPOINT, t);
	}
	mustbe(context, IDENT, e);
	return t;
}

int struct_decl_list(compiler_context *context)
{
	int field_count = 0;
	int i;
	int t;
	int elem_type;
	int curdispl = 0;
	int wasarr = 0;
	int tstrbeg;
	int loc_modetab[100];
	int locmd = 3;

	loc_modetab[0] = MSTRUCT;
	tstrbeg = context->tc;
	totree(context, TStructbeg);
	context->tree[context->tc++] = 0; // тут будет номер иниц процедуры

	scaner(context);
	scaner(context);

	do
	{
		t = elem_type = idorpnt(context, wait_ident_after_semicomma_in_struct, gettype(context));
		if (context->next == LEFTSQBR)
		{
			int adN;
			int all;
			totree(context, TDeclarr);
			adN = context->tc++;
			t = arrdef(context, elem_type); // Меняем тип (увеличиваем размерность массива)
			context->tree[adN] = context->arrdim;

			totree(context, TDeclid);
			totree(context, curdispl);
			totree(context, elem_type);
			totree(context, context->arrdim);							 // N
			context->tree[all = context->tc++] = 0;						 // all
			context->tree[context->tc++] = context->was_struct_with_arr; // proc
			totree(context, context->usual);							 // context->usual
			totree(context, 1); // признак, что массив в структуре
			wasarr = 1;
			if (context->next == ASS)
			{
				scaner(context);
				scaner(context);
				if (is_array(context, t)) // инициализация массива
				{
					context->onlystrings = 2;
					context->tree[all] = 1;
					if (!context->usual)
					{
						context->tree[adN]--; // это уменьшение N в Declarr
					}
					array_init(context, t);
					if (context->onlystrings == 1)
					{
						context->tree[all + 2] = context->usual + 2; // только из строк 2 - без
					}
					// границ, 3 - с границами
				}
				else
				{
					// structdispl = context->identab[oldid+3];
					// context->tree[all] = inition(context, t);
				}
			} // конец ASS
		}	  // конец LEFTSQBR
		loc_modetab[locmd++] = t;
		loc_modetab[locmd++] = REPRTAB_POS;
		field_count++;
		curdispl += szof(context, t);
		if (scaner(context) != SEMICOLON)
		{
			error(context, no_semicomma_in_struct);
		}
	} while (scaner(context) != END);

	if (wasarr)
	{
		totree(context, TStructend);
		totree(context, tstrbeg);
		context->tree[tstrbeg + 1] = context->was_struct_with_arr = context->procd++;
	}
	else
	{
		context->tree[tstrbeg] = NOP;
		context->tree[tstrbeg + 1] = NOP;
	}

	loc_modetab[1] = curdispl; // тут длина структуры
	loc_modetab[2] = field_count * 2;

	context->modetab[context->md] = context->startmode;
	context->startmode = context->md++;
	for (i = 0; i < locmd; i++)
	{
		context->modetab[context->md++] = loc_modetab[i];
	}
	return check_duplicates(context);
}

int gettype(compiler_context *context)
{
	// gettype(context) выедает тип (кроме верхних массивов и указателей)
	// при этом, если такого типа нет в modetab, тип туда заносится;
	// возвращает отрицательное число(базовый тип), положительное (ссылка на modetab)
	// или 0, если типа не было

	context->was_struct_with_arr = 0;
	if (is_int(context, context->type = context->cur) || is_float(context, context->type) || context->type == LVOID)
	{
		return (context->cur == LLONG ? LINT : context->cur == LDOUBLE ? LFLOAT : context->type);
	}
	else if (context->type == LSTRUCT)
	{
		if (context->next == BEGIN) // struct {
		{
			return (struct_decl_list(context));
		}
		else if (context->next == IDENT)
		{
			int l;

			compiler_table_expand(&context->reprtab, 1);
			l = REPRTAB[REPRTAB_POS + 1];
			scaner(context);
			if (context->next == BEGIN) // struct key {
			{
				// если такое описание уже было, то это ошибка - повторное описание
				int lid;
				context->wasstructdef = 1; // это  определение типа (может быть,
										   // без описания переменных)
				toidentab(context, 1000, 0);
				lid = context->lastid;
				context->identab[lid + 2] = struct_decl_list(context);
				context->identab[lid + 3] = 1000 + context->was_struct_with_arr;
				return context->identab[lid + 2];
			}
			else // struct key это применение типа
			{
				if (l == 1)
				{
					error(context, ident_is_not_declared);
				}
				context->was_struct_with_arr = context->identab[l + 3] - 1000;
				return (context->identab[l + 2]);
			}
		}
		else
		{
			error(context, wrong_struct);
		}
	}
	else if (context->cur == IDENT)
	{
		applid(context);
		if (context->identab[context->lastid + 3] < 1000)
		{
			error(context, ident_not_type);
		}
		context->was_struct_with_arr = context->identab[context->lastid + 3] - 1000;
		return context->identab[context->lastid + 2];
	}
	else
	{
		error(context, not_decl);
	}
	return 0;
}

void block(compiler_context *context, int b)
{
	// если b=1, то это просто блок,
	// b = 2 - блок нити,
	// b = -1 - блок в switch, иначе
	// b = 0 - это блок функции

	int oldinswitch = context->inswitch;
	int notended = 1;
	int i;
	int olddispl;
	int oldlg = context->lg;
	int firstdecl;

	context->inswitch = b < 0;
	totree(context, TBegin);
	if (b)
	{
		olddispl = context->displ;
		context->curid = context->id;
	}
	context->blockflag = 0;

	while (is_int(context, context->next) || is_float(context, context->next) || context->next == LSTRUCT ||
		   context->next == LVOID)
	{
		int repeat = 1;
		scaner(context);
		firstdecl = gettype(context);
		if (context->wasstructdef && context->next == SEMICOLON)
		{
			scaner(context);
			continue;
		}
		do
		{
			decl_id(context, idorpnt(context, after_type_must_be_ident, firstdecl));
			if (context->next == COMMA)
			{
				scaner(context);
			}
			else if (context->next == SEMICOLON)
			{
				scaner(context);
				repeat = 0;
			}
			else
			{
				error(context, def_must_end_with_semicomma);
			}
		} while (repeat);
	}

	// кончились описания, пошли операторы до }

	do
	{
		if (b == 2 ? context->next == TEXIT : context->next == END)
		{
			scaner(context);
			notended = 0;
		}
		else
		{
			statement(context);
		}
	} while (notended);

	if (b)
	{
		for (i = context->id - 4; i >= context->curid; i -= 4)
		{
			compiler_table_ensure_allocated(&context->reprtab, context->identab[i + 1] + 1);
			REPRTAB[context->identab[i + 1] + 1] = context->identab[i];
		}
		context->displ = olddispl;
	}
	context->inswitch = oldinswitch;
	context->lg = oldlg;
	totree(context, TEnd);
}

void function_definition(compiler_context *context)
{
	int fn = context->identab[context->lastid + 3];
	int i;
	int pred;
	int oldrepr = REPRTAB_POS;
	int ftype;
	int n;
	int fid = context->lastid;
	int olddispl = context->displ;

	context->pgotost = 0;
	context->functype = context->identab[context->lastid + 2];
	ftype = context->modetab[context->functype + 1];
	n = context->modetab[context->functype + 2];
	context->wasret = 0;
	context->displ = 3;
	context->maxdispl = 3;
	context->lg = 1;
	if ((pred = context->identab[context->lastid]) > 1) // был прототип
	{
		if (context->functype != context->identab[pred + 2])
		{
			error(context, decl_and_def_have_diff_type);
		}
		context->identab[pred + 3] = fn;
	}
	context->curid = context->id;
	for (i = 0; i < n; i++)
	{
		context->type = context->modetab[context->functype + i + 3];
		REPRTAB_POS = context->functions[fn + i + 1];
		if (REPRTAB_POS > 0)
		{
			toidentab(context, 0, context->type);
		}
		else
		{
			REPRTAB_POS = -REPRTAB_POS;
			toidentab(context, -1, context->type);
		}
	}
	context->functions[fn] = context->tc;
	totree(context, TFuncdef);
	totree(context, fid);
	pred = context->tc++;
	REPRTAB_POS = oldrepr;

	block(context, 0);

	// if (ftype == LVOID && context->tree[context->tc - 1] != TReturnvoid)
	// {
	context->tc--;
	totree(context, TReturnvoid);
	totree(context, TEnd);
	// }
	if (ftype != LVOID && !context->wasret)
	{
		error(context, no_ret_in_func);
	}
	for (i = context->id - 4; i >= context->curid; i -= 4)
	{
		compiler_table_ensure_allocated(&context->reprtab, context->identab[i + 1] + 1);
		REPRTAB[context->identab[i + 1] + 1] = context->identab[i];
	}

	for (i = 0; i < context->pgotost - 1; i += 2)
	{
		REPRTAB_POS = context->identab[context->gotost[i] + 1];
		context->hash = context->gotost[i + 1];
		if (context->hash < 0)
		{
			context->hash = -context->hash;
		}
		if (!context->identab[context->gotost[i] + 2])
		{
			error(context, label_not_declared);
		}
	}
	context->curid = 2; // все функции описываются на одном уровне
	context->tree[pred] = context->maxdispl; // + 1;?
	context->lg = -1;
	context->displ = olddispl;
}

int func_declarator(compiler_context *context, int level, int func_d, int firstdecl)
{
	// на 1 уровне это может быть определением функции или предописанием, на
	// остальных уровнях - только декларатором (без идентов)

	int loc_modetab[100];
	int locmd;
	int numpar = 0;
	int ident;
	int maybe_fun;
	int repeat = 1;
	int i;
	int wastype = 0;
	int old;

	loc_modetab[0] = MFUNCTION;
	loc_modetab[1] = firstdecl;
	loc_modetab[2] = 0;
	locmd = 3;

	while (repeat)
	{
		if (context->cur == LVOID || is_int(context, context->cur) || is_float(context, context->cur) ||
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
			context->type = gettype(context);
			if (context->next == LMULT)
			{
				maybe_fun = 1;
				scaner(context);
				context->type = context->type == LVOID ? LVOIDASTER : newdecl(context, MPOINT, context->type);
			}
			if (level)
			{
				if (context->next == IDENT)
				{
					scaner(context);
					ident = 1;
					context->functions[context->funcnum++] = REPRTAB_POS;
				}
			}
			else if (context->next == IDENT)
			{
				error(context, ident_in_declarator);
			}

			if (context->next == LEFTSQBR)
			{
				maybe_fun = 2;

				if (is_pointer(context, context->type) && ident == 0)
				{
					error(context, aster_with_row);
				}

				while (context->next == LEFTSQBR)
				{
					scaner(context);
					mustbe(context, RIGHTSQBR, wait_right_sq_br);
					context->type = newdecl(context, MARRAY, context->type);
				}
			}
		}
		if (context->cur == LVOID)
		{
			context->type = LVOID;
			wastype = 1;
			if (context->next != LEFTBR)
			{
				error(context, par_type_void_with_nofun);
			}
		}

		if (wastype)
		{
			numpar++;
			loc_modetab[locmd++] = context->type;

			if (context->next == LEFTBR)
			{
				scaner(context);
				mustbe(context, LMULT, wrong_fun_as_param);
				if (context->next == IDENT)
				{
					if (level)
					{
						scaner(context);
						if (ident == 0)
						{
							ident = 2;
						}
						else
						{
							error(context, two_idents_for_1_declarer);
						}
						context->functions[context->funcnum++] = -REPRTAB_POS;
					}
					else
					{
						error(context, ident_in_declarator);
					}
				}
				mustbe(context, RIGHTBR, no_right_br_in_paramfun);
				mustbe(context, LEFTBR, wrong_fun_as_param);
				scaner(context);
				if (maybe_fun == 1)
				{
					error(context, aster_before_func);
				}
				else if (maybe_fun == 2)
				{
					error(context, array_before_func);
				}

				old = context->func_def;
				loc_modetab[locmd - 1] = func_declarator(context, 0, 2, context->type);
				context->func_def = old;
			}
			if (func_d == 3)
			{
				func_d = ident > 0 ? 1 : 2;
			}
			else if (func_d == 2 && ident > 0)
			{
				error(context, wait_declarator);
			}
			else if (func_d == 1 && ident == 0)
			{
				error(context, wait_definition);
			}

			if (scaner(context) == COMMA)
			{
				scaner(context);
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
			error(context, wrong_param_list);
		}
	}
	context->func_def = func_d;
	loc_modetab[2] = numpar;

	context->modetab[context->md] = context->startmode;
	context->startmode = context->md++;
	for (i = 0; i < numpar + 3; i++)
	{
		context->modetab[context->md++] = loc_modetab[i];
	}

	return check_duplicates(context);
}

void ext_decl(compiler_context *context)
{
	int i;
	do // top level описания переменных и функций до конца файла
	{
		int repeat = 1;
		int funrepr;
		int first = 1;
		context->wasstructdef = 0;
		scaner(context);
		/*
			if (context->cur == SH_DEFINE)
			{
				mustbe(context, IDENT, no_ident_in_define);
				if (scaner(context) == LMINUS)
					scaner(context), k = -1;
				if (context->cur != NUMBER || context->ansttype != LINT)
					error(context, not_int_in_define);
				toidentab(-2, k * num);
				continue;
			}
		*/
		context->firstdecl = gettype(context);
		if (context->wasstructdef && context->next == SEMICOLON) // struct point {float x, y;};
		{
			scaner(context);
			continue;
		}

		context->func_def = 3; // context->func_def = 0 - (),
							   // 1 - определение функции,
							   // 2 - это предописание,
							   // 3 - не знаем или вообще не функция

		//	if (firstdecl == 0)
		//		firstdecl = LINT;

		do // описываемые объекты через ',' определение функции может быть только одно, никаких ','
		{
			context->type = context->firstdecl;
			if (context->next == LMULT)
			{
				scaner(context);
				context->type = context->firstdecl == LVOID ? LVOIDASTER : newdecl(context, MPOINT, context->firstdecl);
			}
			mustbe(context, IDENT, after_type_must_be_ident);

			if (context->next == LEFTBR) // определение или предописание функции
			{
				int oldfuncnum = context->funcnum++;
				int firsttype = context->type;
				funrepr = REPRTAB_POS;
				scaner(context);
				scaner(context);

				// выкушает все параметры до ) включительно
				context->type = func_declarator(context, first, 3, firsttype);

				if (context->next == BEGIN)
				{
					if (context->func_def == 0)
					{
						context->func_def = 1;
					}
				}
				else if (context->func_def == 0)
				{
					context->func_def = 2;
				}
				// теперь я точно знаю, это определение ф-ции или предописание
				// (context->func_def=1 или 2)
				REPRTAB_POS = funrepr;

				toidentab(context, oldfuncnum, context->type);

				if (context->next == BEGIN)
				{
					scaner(context);
					if (context->func_def == 2)
					{
						error(context, func_decl_req_params);
					}

					function_definition(context);
					goto ex;
				}
				else
				{
					if (context->func_def == 1)
					{
						error(context, function_has_no_body);
					}
				}
			}
			else if (context->firstdecl == LVOID)
			{
				error(context, only_functions_may_have_type_VOID);
			}

			// описания идентов-не-функций

			if (context->func_def == 3)
			{
				decl_id(context, context->type);
			}

			if (context->next == COMMA)
			{
				scaner(context);
				first = 0;
			}
			else if (context->next == SEMICOLON)
			{
				scaner(context);
				repeat = 0;
			}
			else
			{
				error(context, def_must_end_with_semicomma);
			}
		} while (repeat);

	ex:;
	} while (context->next != LEOF);

	if (context->wasmain == 0)
	{
		error(context, no_main_in_program);
	}
	for (i = 0; i <= context->prdf; i++)
	{
		if (context->predef[i])
		{
			error(context, predef_but_notdef);
		}
	}
	totree(context, TEnd);
}
