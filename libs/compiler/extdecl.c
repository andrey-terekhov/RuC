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
#include "scanner.h"
#include <limits.h>
#include <string.h>


void exprassnval(analyzer *);
void expr(analyzer *context, int level);
void exprassn(analyzer *context, int);

void struct_init(analyzer *context, int);
int gettype(analyzer *context);

// если b=1, то это просто блок,
// b = 2 - блок нити,
// b = -1 - блок в switch, иначе
// b = 0 - это блок функции
void block(analyzer *context, int b);


int newdecl(syntax *const sx, const int type, const int element_type)
{
	int temp[2];
	temp[0] = type;
	temp[1] = element_type;
	return (int)mode_add(sx, temp, 2);
}


void context_error(analyzer *const context, const int num) // Вынесено из errors.c
{
	const universal_io *const io = context->io;

	switch (num)
	{
		case not_primary:
			error(io, num, context->cur);
			break;
		case bad_toval:
			error(io, num, context->ansttype);
			break;
		case wrong_printf_param_type:
		case printf_unknown_format_placeholder:
			error(io, num, context->bad_printf_placeholder);
			break;
		case repeated_decl:
		case ident_is_not_declared:
		case repeated_label:
		case no_field:
		case predef_but_notdef:
			error(io, num, REPRTAB, REPRTAB_POS);
			break;
		case label_not_declared:
			error(io, num, context->hash, REPRTAB, REPRTAB_POS);
			break;
		default:
			error(io, num);
	}

	context->error_flag = 1;
	context->sx->tc = context->temp_tc;

	/*if (!context->new_line_flag && context->curchar != EOF)
	{
		while (context->curchar != '\n' && context->curchar != EOF)
		{
			nextch(context);
		}

		if (context->curchar != EOF)
		{
			scaner(context);
		}
	}

	if (context->curchar != EOF)
	{
		scaner(context);
	}*/
}


int has_token_set(const unsigned int left, const unsigned int right)
{
	return left & right;
}

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	context		Universal io structure
 *	@param	tokens		Specified tokens
 */
void skip_until(analyzer *const context, const unsigned int tokens)
{
	while (1)
	{
		switch (context->next)
		{
			case LEOF:
				return;
				
			case LEFTBR:
				scaner(context);
				skip_until(context, RIGHTBR);
				break;
				
			case RIGHTBR:
				if (has_token_set(tokens, RIGHTBR))
					return;
				else
				{
					scaner(context);
					break;
				}
				
			case LEFTSQBR:
				scaner(context);
				skip_until(context, RIGHTSQBR);
				break;
				
			case RIGHTSQBR:
				if (has_token_set(tokens, RIGHTSQBR))
					return;
				else
				{
					scaner(context);
					break;
				}
				
			case BEGIN:
				scaner(context);
				skip_until(context, END);
				break;
				
			case END:
				if (has_token_set(tokens, END))
					return;
				else
				{
					scaner(context);
					break;
				}
				
			case QUEST:
				scaner(context);
				skip_until(context, COLON);
				break;
				
			case COLON:
				if (has_token_set(tokens, COLON))
					return;
				else
				{
					scaner(context);
					break;
				}
				
			case SEMICOLON:
				if (has_token_set(tokens, SEMICOLON))
					return;
				else
				{
					scaner(context);
					break;
				}
				
			default:
				scaner(context);
				break;
		}
	}
}

int evaluate_params(analyzer *context, int num, int formatstr[], int formattypes[], int placeholders[])
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
					context_error(context, too_many_printf_params);
					return 0;
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
					formattypes[numofparams++] = newdecl(context->sx, MARRAY, LCHAR);
					break;

				case '%':
					break;

				case 0:
					context_error(context, printf_no_format_placeholder);
					return 0;

				default:
					context->bad_printf_placeholder = fsi;
					context_error(context, printf_unknown_format_placeholder);
					return 0;
			}
		}
	}

	return numofparams;
}

int is_function(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == MFUNCTION;
}

int is_array(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == MARRAY;
}

int is_string(syntax *const sx, const int t)
{
	return is_array(sx, t) && mode_get(sx, t + 1) == LCHAR;
}

int is_pointer(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == MPOINT;
}

int is_struct(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == MSTRUCT;
}

int is_float(const int t)
{
	return t == LFLOAT || t == LDOUBLE;
}

int is_int(const int t)
{
	return t == LINT || t == LLONG || t == LCHAR;
}

int szof(analyzer *context, int type)
{
	return context->next == LEFTSQBR ? 1
	: type == LFLOAT ? 2 : (is_struct(context->sx, type)) ? mode_get(context->sx, type + 1) : 1;
}

void mustbe(analyzer *context, int what, int e)
{
	if (context->next != what)
	{
		context_error(context, e);
		context->cur = what;
	}
	else
	{
		scaner(context);
	}
}

void mustbe_complex(analyzer *context, int what, int e)
{
	if (scaner(context) != what)
	{
		context_error(context, e);
		context->error_flag = e;
	}
}

void totree(analyzer *context, int op)
{
	context->sx->tree[context->sx->tc++] = op;
}

void totreef(analyzer *context, int op)
{
	context->sx->tree[context->sx->tc++] = op;
	if (context->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		context->sx->tree[context->sx->tc - 1] += 50;
	}
}

int getstatic(analyzer *context, int type)
{
	int olddispl = context->displ;
	context->displ += context->lg * szof(context, type); // lg - смещение от l (+1) или от g (-1)
	if (context->lg > 0)
	{
		context->maxdispl = (context->displ > context->maxdispl) ? context->displ : context->maxdispl;
	}
	else
	{
		context->sx->maxdisplg = -context->displ;
	}
	return olddispl;
}

int toidentab(analyzer *context, int f, int type)
{
	// f =  0, если не ф-ция, f=1, если метка, f=funcnum,
	// если описание ф-ции,
	// f = -1, если ф-ция-параметр, f>=1000, если это описание типа
	// f = -2, #define

	// printf("\n f= %i context->repr %i rtab[context->repr] %i
	// rtab[context->repr+1] %i rtab[context->repr+2] %i\n", f,
	// context->repr, context->sx->reprtab[context->repr],
	// context->sx->reprtab[context->repr+1], context->sx->reprtab[context->repr+2]);
	int pred;


	context->lastid = context->sx->id;
	if (REPRTAB[REPRTAB_POS + 1] == 0) // это может быть только MAIN
	{
		if (context->sx->wasmain)
		{
			context_error(context, more_than_1_main); //--
			context->error_flag = 5;
			return 0; // 1
		}
		context->sx->wasmain = context->sx->id;
	}

	// ссылка на описание с таким же
	// представлением в предыдущем блоке
	pred = context->sx->identab[context->sx->id] = REPRTAB[REPRTAB_POS + 1];
	if (pred)
	{
		// pred == 0 только для main, эту ссылку портить нельзя
		// ссылка на текущее описание с этим представлением
		// (это в reprtab)
		REPRTAB[REPRTAB_POS + 1] = context->sx->id;
	}

	if (f != 1 && pred >= context->curid) // один  и тот же идент м.б. переменной и меткой
	{
		if (context->func_def == 3 ? 1 : context->sx->identab[pred + 1] > 0 ? 1 : context->func_def == 1 ? 0 : 1)
		{
			context_error(context, repeated_decl);
			context->error_flag = 5;
			return 0; // 1
					  // только определение функции может иметь 2
					  // описания, т.е. иметь предописание
		}
	}

	context->sx->identab[context->sx->id + 1] = REPRTAB_POS; // ссылка на представление
	if (f == -2)									 // #define
	{
		context->sx->identab[context->sx->id + 2] = 1;
		context->sx->identab[context->sx->id + 3] = type; // это целое число, определенное по #define
	}
	else // дальше тип или ссылка на modetab (для функций и структур)
	{
		context->sx->identab[context->sx->id + 2] = type; // тип -1 int, -2 char, -3 float, -4 long, -5 double,
												  // если тип > 0, то это ссылка на modetab
		if (f == 1)
		{
			context->sx->identab[context->sx->id + 2] = 0; // 0, если первым встретился goto, когда встретим метку,
												   // поставим 1
			context->sx->identab[context->sx->id + 3] = 0; // при генерации кода когда встретим метку, поставим pc
		}
		else if (f >= 1000)
		{
			context->sx->identab[context->sx->id + 3] = f; // это описание типа, если f > 1000, то f-1000 - это номер
												   // иниц проц
		}
		else if (f)
		{
			if (f < 0)
			{
				context->sx->identab[context->sx->id + 3] = -(context->displ++);
				context->maxdispl = context->displ;
			}
			else // identtab[context->lastid+3] - номер функции, если < 0, то
				 // это функция-параметр
			{
				context->sx->identab[context->sx->id + 3] = f;
				if (context->func_def == 2)
				{
					context->sx->identab[context->lastid + 1] *= -1; //это предописание
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
			context->sx->identab[context->sx->id + 3] = getstatic(context, type);
		}
	}
	context->sx->id += 4;
	return context->lastid;
}

void binop(analyzer *context, int sp)
{
	int op = context->stackop[sp];
	int rtype = context->stackoperands[context->sopnd--];
	int ltype = context->stackoperands[context->sopnd];

	if (is_pointer(context->sx, ltype) || is_pointer(context->sx, rtype))
	{
		context_error(context, operand_is_pointer);
		context->error_flag = 5;
		return; // 1
	}
	if ((op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND || op == LSHL || op == LSHR ||
		 op == LREM) &&
		(is_float(ltype) || is_float(rtype)))
	{
		context_error(context, int_op_for_float);
		context->error_flag = 5;
		return; // 1
	}
	if (is_int(ltype) && is_float(rtype))
	{
		totree(context, WIDEN1);
	}
	if (is_int(rtype) && is_float(ltype))
	{
		totree(context, WIDEN);
	}
	if (is_float(ltype) || is_float(rtype))
	{
		context->ansttype = LFLOAT;
	}
	if (op == LOGOR || op == LOGAND)
	{
		totree(context, op);
		context->sx->tree[context->stacklog[sp]] = context->sx->tc++;
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

void toval(analyzer *context)
{
	// надо значение положить на стек,
	// например, чтобы передать параметром

	if (context->anst == VAL || context->anst == NUMBER)
	{
		;
	}
	else if (is_struct(context->sx, context->ansttype))
	{
		if (!context->inass)
		{
			if (context->anst == IDENT)
			{
				context->sx->tc -= 2;
				totree(context, COPY0ST);
				totree(context, context->anstdispl);
			}
			else // тут может быть только ADDR
			{
				totree(context, COPY1ST);
			}
			totree(context, mode_get(context->sx, context->ansttype + 1));
			context->anst = VAL;
		}
	}
	else
	{
		if (context->anst == IDENT)
		{
			context->sx->tree[context->sx->tc - 2] = is_float(context->ansttype) ? TIdenttovald : TIdenttoval;
		}

		if (!(is_array(context->sx, context->ansttype) || is_pointer(context->sx, context->ansttype)))
		{
			if (context->anst == ADDR)
			{
				totree(context, is_float(context->ansttype) ? TAddrtovald : TAddrtoval);
			}
		}
		context->anst = VAL;
	}
}

void insertwiden(analyzer *context)
{
	context->sx->tc--;
	totree(context, WIDEN);
	totree(context, TExprend);
}

void applid(analyzer *context)
{
	context->lastid = REPRTAB[REPRTAB_POS + 1];
	if (context->lastid == 1)
	{
		context_error(context, ident_is_not_declared);
		context->error_flag = 5;
	}
}


void exprval(analyzer *context);
void unarexpr(analyzer *context);

void actstring(int type, analyzer *context)
{
	int n = 0;
	int adn;
	scaner(context);
	totree(context, type == LFLOAT ? TStringd : TString);
	adn = context->sx->tc++;
	do
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 1;
			return; // 1
		}
		if (context->sx->tree[context->sx->tc - 3] == TConstd)
		{
			context->sx->tree[context->sx->tc - 3] = context->sx->tree[context->sx->tc - 2];
			context->sx->tree[context->sx->tc - 2] = context->sx->tree[context->sx->tc - 1];
			--context->sx->tc;
		}
		else if (context->sx->tree[context->sx->tc - 2] == TConst)
		{
			context->sx->tree[context->sx->tc - 2] = context->sx->tree[context->sx->tc - 1];
			--context->sx->tc;
		}
		else
		{
			context_error(context, wrong_init_in_actparam);
			context->error_flag = 1;
			return; // 1
		}
		++n;
	} while (scaner(context) == COMMA ? scaner(context), 1 : 0);

	context->sx->tree[adn] = n;
	if (context->cur != END)
	{
		context_error(context, no_comma_or_end);
		context->error_flag = 1;
		return; // 1
	}
	context->ansttype = newdecl(context->sx, MARRAY, type);
	context->anst = VAL;
}

void mustbestring(analyzer *context)
{
	scaner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (!(is_string(context->sx, context->ansttype)))
	{
		context_error(context, not_string_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustbepointstring(analyzer *context)
{
	scaner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (!(is_pointer(context->sx, context->ansttype) &&
		  is_string(context->sx, mode_get(context->sx, context->ansttype + 1))))
	{
		context_error(context, not_point_string_in_stanfunc);
		context->error_flag = 5;
		return; // 1
	}
}

void mustberow(analyzer *context)
{
	scaner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;

	if (!is_array(context->sx, context->ansttype))
	{
		context_error(context, not_array_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustbeint(analyzer *context)
{
	scaner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (context->ansttype != LINT && context->ansttype != LCHAR)
	{
		context_error(context, not_int_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustberowofint(analyzer *context)
{
	if (scaner(context) == BEGIN)
	{
		actstring(LINT, context), totree(context, TExprend);
		if (context->error_flag == 2)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
		toval(context);
		context->sopnd--;
		if (context->ansttype == LINT || context->ansttype == LCHAR)
		{
			totree(context, ROWING);
			context->ansttype = newdecl(context->sx, MARRAY, LINT);
		}
	}
	if (!(is_array(context->sx, context->ansttype) &&
		  is_int(mode_get(context->sx, context->ansttype + 1))))
	{
		context_error(context, not_rowofint_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustberowoffloat(analyzer *context)
{
	if (scaner(context) == BEGIN)
	{
		actstring(LFLOAT, context), totree(context, TExprend);
		if (context->error_flag == 2)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
		toval(context);
		context->sopnd--;
		if (context->ansttype == LFLOAT)
		{
			totree(context, ROWINGD);
			context->ansttype = newdecl(context->sx, MARRAY, LFLOAT);
		}
	}

	if (!(is_array(context->sx, context->ansttype) &&
		  mode_get(context->sx, context->ansttype + 1) == LFLOAT))
	{
		context_error(context, not_rowoffloat_in_stanfunc);
		context->error_flag = 5;
	}
}

void primaryexpr(analyzer *context)
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

		context->ansttype = newdecl(context->sx, MARRAY, LCHAR); // теперь пишем context->ansttype в
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
		if (context->error_flag == 5)
		{
			context->error_flag = 4;
			return; // 1
		}

		totree(context, TIdent);
		totree(context, context->anstdispl = context->sx->identab[context->lastid + 3]);
		context->stackoperands[++context->sopnd] = context->ansttype = context->sx->identab[context->lastid + 2];
		context->anst = IDENT;
	}
	else if (context->cur == LEFTBR)
	{
		if (context->next == LVOID)
		{
			scaner(context);
			mustbe(context, LMULT, no_mult_in_cast);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (!is_pointer(context->sx, context->ansttype))
			{
				context_error(context, not_pointer_in_cast);
				context->error_flag = 4;
				return; // 1
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
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
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
			context_error(context, no_leftbr_in_stand_func);
			context->buf_cur = context->next;
			context->next = context->cur;
			context->cur = LEFTBR;
			context->buf_flag++;
		}
		if (func == ASSERT)
		{
			mustbeint(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
			mustbestring(context);
		}
		else if (func <= STRCPY && func >= STRLEN) // функции работы со строками
		{
			if (func >= STRNCAT)
			{
				mustbepointstring(context);
			}
			else
			{
				mustbestring(context);
			}
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func != STRLEN)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustbestring(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (func == STRNCPY || func == STRNCAT || func == STRNCMP)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
				}
			}
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func < STRNCAT)
			{
				context->stackoperands[++context->sopnd] = context->ansttype = LINT;
			}
		}
		else if (func >= RECEIVE_STRING && func <= SEND_INT)
		{
			context->notrobot = 0; // новые функции Фадеева
			mustbeint(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func == SEND_INT || func == SEND_STRING)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				// scaner(context);
				mustberowofint(context);
			}
			else if (func == SEND_FLOAT)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				// scaner(context);
				mustberowoffloat(context);
			}
			else
			{
				context->stackoperands[++context->sopnd] = context->ansttype =
					func == RECEIVE_INT ? LINT : func == RECEIVE_FLOAT ? LFLOAT : newdecl(context->sx, MARRAY, LCHAR);
			}
		}
		else if (func >= ICON && func <= WIFI_CONNECT) // функции Фадеева
		{
			context->notrobot = 0;
			if (func <= PIXEL && func >= ICON)
			{
				// scaner(context);
				mustberowofint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (func != CLEAR)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				}

				if (func == LINE || func == RECTANGLE || func == ELLIPS)
				{
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
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
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					if (func == ICON)
					{
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
					}
				}
				else if (func == DRAW_NUMBER || func == DRAW_STRING)
				{
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);

					if (func == DRAW_STRING)
					{
						mustbestring(context);
					}
					else // DRAW_NUMBER
					{
						scaner(context);
						exprassn(context, 1);
						if (context->error_flag == 6)
						{
							context->error_flag = 4;
							return; // 1
						}
						toval(context);
						context->sopnd--;
						if (is_int(context->ansttype))
						{
							totree(context, WIDEN);
						}
						else if (context->ansttype != LFLOAT)
						{
							context_error(context, not_float_in_stanfunc);
							context->error_flag = 4;
							return; // 1
						}
					}
				}
			}
			else if (func == SETSIGNAL)
			{
				mustbeint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustberowofint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
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
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
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
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
					}
					else if (func == BLYNK_PROPERTY)
					{
						mustbestring(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbestring(context);
					}
					else // BLYNK_LCD
					{
						mustbeint(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
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
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
			mustberow(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
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
						context_error(context, act_param_not_ident);
						context->error_flag = 4;
						return; // 1
					}
					applid(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					if (context->sx->identab[context->lastid + 2] != 15 ||
						context->error_flag == 5) // 15 - это аргумент типа void* (void*)
					{
						context_error(context, wrong_arg_in_create);
						context->error_flag = 4;
						return; // 1
					}

					context->stackoperands[context->sopnd] = context->ansttype = LINT;
					dn = context->sx->identab[context->lastid + 3];
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
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);

					if (func == TMSGSEND)
					{
						if (context->ansttype != 2) // 2 - это аргумент типа msg_info (struct{int numTh; int data;})
						{
							context_error(context, wrong_arg_in_send);
							context->error_flag = 4;
							return; // 1
						}
						--context->sopnd;
					}
					else
					{
						if (!is_int(context->ansttype))
						{
							context_error(context, param_threads_not_int);
							context->error_flag = 4;
							return; // 1
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
		else if (func == ROUND)
		{
			scaner(context);
			exprassn(context, 1);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			toval(context);
			context->ansttype = context->stackoperands[context->sopnd] = LINT;
		}
		else
		{
			scaner(context);
			exprassn(context, 1);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			toval(context);

			// GETDIGSENSOR int(int port, int pins[]),
			// GETANSENSOR int (int port, int pin),
			// SETMOTOR и VOLTAGE void (int port, int volt)
			if (func == GETDIGSENSOR || func == GETANSENSOR || func == SETMOTOR || func == VOLTAGE)
			{
				context->notrobot = 0;
				if (!is_int(context->ansttype))
				{
					context_error(context, param_setmotor_not_int);
					context->error_flag = 4;
					return; // 1
				}
				mustbe(context, COMMA, no_comma_in_setmotor);
				if (func == GETDIGSENSOR)
				{
					mustberowofint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					context->ansttype = context->stackoperands[++context->sopnd] = LINT;
				}
				else
				{
					scaner(context);
					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);
					if (!is_int(context->ansttype))
					{
						context_error(context, param_setmotor_not_int);
						context->error_flag = 4;
						return; // 1
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
			}
			else if (func == ABS && is_int(context->ansttype))
			{
				func = ABSI;
			}
			else
			{
				if (is_int(context->ansttype))
				{
					totree(context, WIDEN);
					context->ansttype = context->stackoperands[context->sopnd] = LFLOAT;
				}
				if (!is_float(context->ansttype))
				{
					context_error(context, bad_param_in_stand_func);
					context->error_flag = 4;
					return; // 1
				}
			}
		}
		if (context->error_flag == 5)
		{
			context->error_flag = 4;
			return; // 1
		}
		totree(context, 9500 - func);
		mustbe(context, RIGHTBR, no_rightbr_in_stand_func);
	}
	else
	{
		context_error(context, not_primary);
		context->error_flag = 4;
		return; // 1
	}
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
}

void index_check(analyzer *context)
{
	if (!is_int(context->ansttype))
	{
		context_error(context, index_must_be_int);
		context->error_flag = 5;
	}
}

int find_field(analyzer *context, int stype)
{
	// выдает смещение до найденного поля или ошибку

	int i;
	int flag = 1;
	int select_displ = 0;
	int record_length = mode_get(context->sx, stype + 2);

	scaner(context);
	mustbe(context, IDENT, after_dot_must_be_ident);

	for (i = 0; i < record_length; i += 2) // тут хранится удвоенное n
	{
		int field_type = mode_get(context->sx, stype + 3 + i);

		if (mode_get(context->sx, stype + 4 + i) == REPRTAB_POS)
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
		context_error(context, no_field);
		context->error_flag = 5;
		return 0; // 1
	}
	return select_displ;
}

void selectend(analyzer *context)
{
	while (context->next == DOT)
	{
		context->anstdispl += find_field(context, context->ansttype);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
	}

	totree(context, context->anstdispl);
	if (is_array(context->sx, context->ansttype) || is_pointer(context->sx, context->ansttype))
	{
		totree(context, TAddrtoval);
	}
}

void array_init(analyzer *context, int t);

void postexpr(analyzer *context)
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
		if (!is_function(context->sx, leftansttyp))
		{
			context_error(context, call_not_from_function);
			context->error_flag = 4;
			return; // 1
		}

		n = mode_get(context->sx, leftansttyp + 2); // берем количество аргументов функции

		totree(context, TCall1);
		totree(context, n);
		j = leftansttyp + 3;
		for (i = 0; i < n; i++) // фактические параметры
		{
			int mdj = context->leftansttype = mode_get(context->sx, j); // это вид формального параметра, в
																   // context->ansttype будет вид фактического
																   // параметра
			scaner(context);
			if (is_function(context->sx, mdj))
			{
				// фактическим параметром должна быть функция, в С - это только идентификатор

				if (context->cur != IDENT)
				{
					context_error(context, act_param_not_ident);
					context->error_flag = 4;
					return; // 1
				}
				applid(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (context->sx->identab[context->lastid + 2] != mdj)
				{
					context_error(context, diff_formal_param_type_and_actual);
					context->error_flag = 4;
					return; // 1
				}
				dn = context->sx->identab[context->lastid + 3];
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
				if (context->cur == BEGIN && is_array(context->sx, mdj))
				{
					actstring(mode_get(context->sx, mdj + 1), context), totree(context, TExprend);
					if (context->error_flag == 2)
					{
						context->error_flag = 4;
						return; // 1
					}
				}
				else
				{
					context->inass = 0;
					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);
					totree(context, TExprend);

					if (mdj > 0 && mdj != context->ansttype)
					{
						context_error(context, diff_formal_param_type_and_actual);
						context->error_flag = 4;
						return; // 1
					}

					if (is_int(mdj) && is_float(context->ansttype))
					{
						context_error(context, float_instead_int);
						context->error_flag = 4;
						return; // 1
					}

					if (is_float(mdj) && is_int(context->ansttype))
					{
						insertwiden(context);
					}
					--context->sopnd;
				}
			}
			if (i < n - 1 && scaner(context) != COMMA)
			{
				context_error(context, no_comma_in_act_params);
				context->error_flag = 4;
				return; // 1
			}
			j++;
		}
		context->inass = oldinass;
		mustbe(context, RIGHTBR, wrong_number_of_params);
		totree(context, TCall2);
		totree(context, lid);
		context->stackoperands[context->sopnd] = context->ansttype = mode_get(context->sx, leftansttyp + 1);
		context->anst = VAL;
		if (is_struct(context->sx, context->ansttype))
		{
			context->x -= mode_get(context->sx, context->ansttype + 1) - 1;
		}
	}

	while (context->next == LEFTSQBR || context->next == ARROW || context->next == DOT)
	{
		while (context->next == LEFTSQBR) // вырезка из массива (возможно, многомерного)
		{
			int elem_type;
			if (was_func)
			{
				context_error(context, slice_from_func);
				context->error_flag = 4;
				return; // 1
			}
			if (!is_array(context->sx, context->ansttype)) // вырезка не из массива
			{
				context_error(context, slice_not_from_array);
				context->error_flag = 4;
				return; // 1
			}

			elem_type = mode_get(context->sx, context->ansttype + 1);

			scaner(context);
			scaner(context);

			if (context->anst == IDENT) // a[i]
			{
				context->sx->tree[context->sx->tc - 2] = TSliceident;
				context->sx->tree[context->sx->tc - 1] = context->anstdispl;
			}
			else // a[i][j]
			{
				totree(context, TSlice);
			}

			totree(context, elem_type);
			exprval(context);
			if (context->error_flag == 4)
			{
				return; // 1
			}
			index_check(context); // проверка, что индекс int или char
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}

			mustbe(context, RIGHTSQBR, no_rightsqbr_in_slice);

			context->stackoperands[--context->sopnd] = context->ansttype = elem_type;
			context->anst = ADDR;
		}

		while (context->next == ARROW)
		{
			// это выборка поля из указателя на структуру, если после
			// -> больше одной точки подряд, схлопываем в 1 select
			// перед выборкой мог быть вызов функции или вырезка элемента массива

			if (!is_pointer(context->sx, context->ansttype) ||
				!is_struct(context->sx, mode_get(context->sx, context->ansttype + 1)))
			{
				context_error(context, get_field_not_from_struct_pointer);
				context->error_flag = 4;
				return; // 1
			}

			if (context->anst == IDENT)
			{
				context->sx->tree[context->sx->tc - 2] = TIdenttoval;
			}
			context->anst = ADDR;
			// pointer  мог быть значением функции (VAL) или, может быть,
			totree(context, TSelect); // context->anst уже был ADDR, т.е. адрес
									  // теперь уже всегда на верхушке стека

			context->ansttype = mode_get(context->sx, context->ansttype + 1);
			context->anstdispl = find_field(context, context->ansttype);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			selectend(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
		}
		if (context->next == DOT)

		{
			if (!is_struct(context->sx, context->ansttype))
			{
				context_error(context, select_not_from_struct);
				context->error_flag = 4;
				return; // 1
			}
			if (context->anst == VAL) // структура - значение функции
			{
				int len1 = szof(context, context->ansttype);
				context->anstdispl = 0;
				while (context->next == DOT)
				{
					context->anstdispl += find_field(context, context->ansttype);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
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
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
				}
				context->sx->tree[context->sx->tc - 1] = context->anstdispl;
			}
			else // ADDR
			{
				totree(context, TSelect);
				context->anstdispl = 0;
				selectend(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
			}
		}
	}
	if (context->next == INC || context->next == DEC) // a++, a--
	{
		int op;

		if (!is_int(context->ansttype) && !is_float(context->ansttype))
		{
			context_error(context, wrong_operand);
			context->error_flag = 4;
			return; // 1
		}

		if (context->anst != IDENT && context->anst != ADDR)
		{
			context_error(context, unassignable_inc);
			context->error_flag = 4;
			return; // 1
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
			totree(context, context->sx->identab[lid + 3]);
		}
		context->anst = VAL;
	}
}

void unarexpr(analyzer *context)
{
	int op = context->cur;
	if (context->cur == LNOT || context->cur == LOGNOT || context->cur == LPLUS || context->cur == LMINUS ||
		context->cur == LAND || context->cur == LMULT || context->cur == INC || context->cur == DEC)
	{
		if (context->cur == INC || context->cur == DEC)
		{
			scaner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				return; // 1
			}
			if (context->anst != IDENT && context->anst != ADDR)
			{
				context_error(context, unassignable_inc);
				context->error_flag = 7;
				return; // 1
			}
			if (context->anst == ADDR)
			{
				op += 4;
			}
			totreef(context, op);
			if (context->anst == IDENT)
			{
				totree(context, context->sx->identab[context->lastid + 3]);
			}
			context->anst = VAL;
		}
		else
		{
			scaner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				return; // 1
			}

			if (op == LAND)
			{
				if (context->anst == VAL)
				{
					context_error(context, wrong_addr);
					context->error_flag = 7;
					return; // 1
				}

				if (context->anst == IDENT)
				{
					context->sx->tree[context->sx->tc - 2] = TIdenttoaddr; // &a
				}

				context->stackoperands[context->sopnd] = context->ansttype =
					newdecl(context->sx, MPOINT, context->ansttype);
				context->anst = VAL;
			}
			else if (op == LMULT)
			{
				if (!is_pointer(context->sx, context->ansttype))
				{
					context_error(context, aster_not_for_pointer);
					context->error_flag = 7;
					return; // 1
				}

				if (context->anst == IDENT)
				{
					context->sx->tree[context->sx->tc - 2] = TIdenttoval; // *p
				}

				context->stackoperands[context->sopnd] = context->ansttype = mode_get(context->sx, context->ansttype + 1);
				context->anst = ADDR;
			}
			else
			{
				toval(context);
				if ((op == LNOT || op == LOGNOT) && context->ansttype == LFLOAT)
				{
					context_error(context, int_op_for_float);
					context->error_flag = 7;
					return; // 1
				}
				else if (op == LMINUS)
				{
					if (context->sx->tree[context->sx->tc - 2] == TConst)
					{
						context->sx->tree[context->sx->tc - 1] *= -1;
					}
					else if (context->sx->tree[context->sx->tc - 3] == TConstd)
					{
						double d;
						memcpy(&d, &context->sx->tree[context->sx->tc - 2], sizeof(double));
						d = -d;
						memcpy(&context->sx->tree[context->sx->tc - 2], &d, sizeof(double));
					}
					else
					{
						totreef(context, UNMINUS);
					}
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
		if (context->error_flag == 4)
		{
			context->error_flag = 7;
			return; // 1
		}
	}

	postexpr(context); // 0
	context->stackoperands[context->sopnd] = context->ansttype;
	if (context->error_flag == 4)
	{
		context->error_flag = 7;
		return; // 1
	}
}

void exprinbrkts(analyzer *context, int er)
{
	mustbe(context, LEFTBR, er);
	scaner(context);
	exprval(context);
	if (context->error_flag == 4)
	{
		context->error_flag = 3;
		return; // 1
	}
	mustbe(context, RIGHTBR, er);
}

void exprassninbrkts(analyzer *context, int er)
{
	mustbe(context, LEFTBR, er);
	scaner(context);
	exprassnval(context);
	if (context->error_flag == 4)
	{
		context->error_flag = 3;
		return; // 1
	}
	mustbe(context, RIGHTBR, er);
}

int prio(int op)
{
	// возвращает 0, если не операция
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

void subexpr(analyzer *context)
{
	int oldsp = context->sp;
	int wasop = 0;
	int ad = 0;

	int p = prio(context->next);
	while (p)
	{
		wasop = 1;
		toval(context);
		while (context->sp > oldsp && context->stack[context->sp - 1] >= p)
		{
			binop(context, --context->sp);
			if (context->error_flag == 5)
			{
				return;
			}
		}

		if (p <= 2)
		{
			totree(context, p == 1 ? ADLOGOR : ADLOGAND);
			ad = context->sx->tc++;
		}

		context->stack[context->sp] = p;
		context->stacklog[context->sp] = ad;
		context->stackop[context->sp++] = context->next;
		scaner(context);
		scaner(context);
		unarexpr(context);
		if (context->error_flag == 7)
		{
			context->error_flag = 5;
			return; // 1
		}
		p = prio(context->next);
	}
	if (wasop)
	{
		toval(context);
	}
	while (context->sp > oldsp)
	{
		binop(context, --context->sp);
		if (context->error_flag == 5)
		{
			return;
		}
	}
}

int intopassn(int next)
{
	return next == REMASS || next == SHLASS || next == SHRASS || next == ANDASS || next == EXORASS || next == ORASS;
}

int opassn(analyzer *context)
{
	return (context->next == ASS || context->next == MULTASS || context->next == DIVASS || context->next == PLUSASS ||
			context->next == MINUSASS || intopassn(context->next))
			   ? context->op = context->next
			   : 0;
}

void condexpr(analyzer *context)
{
	int globtype = 0;
	int adif = 0;
	int r;

	subexpr(context); // logORexpr();
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
	if (context->next == QUEST)
	{
		while (context->next == QUEST)
		{
			toval(context);
			if (!is_int(context->ansttype))
			{
				context_error(context, float_in_condition);
				context->error_flag = 4;
				return; // 1
			}
			totree(context, TCondexpr);
			scaner(context);
			scaner(context);
			context->sopnd--;
			exprval(context); // then
			if (context->error_flag == 4)
			{
				return; // 1
			}
			if (!globtype)
			{
				globtype = context->ansttype;
			}
			context->sopnd--;
			if (is_float(context->ansttype))
			{
				globtype = LFLOAT;
			}
			else
			{
				context->sx->tree[context->sx->tc] = adif;
				adif = context->sx->tc++;
			}
			mustbe(context, COLON, no_colon_in_cond_expr);
			scaner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
				return; // 1
			}
			subexpr(context); // logORexpr();	else or elif
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
		}
		toval(context);
		totree(context, TExprend);
		if (is_float(context->ansttype))
		{
			globtype = LFLOAT;
		}
		else
		{
			context->sx->tree[context->sx->tc] = adif;
			adif = context->sx->tc++;
		}

		while (adif != 0)
		{
			r = context->sx->tree[adif];
			context->sx->tree[adif] = TExprend;
			context->sx->tree[adif - 1] = is_float(globtype) ? WIDEN : NOP;
			adif = r;
		}

		context->stackoperands[context->sopnd] = context->ansttype = globtype;
	}
	else
	{
		context->stackoperands[context->sopnd] = context->ansttype;
	}
}

void inition(analyzer *context, int decl_type)
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

void struct_init(analyzer *context, int decl_type)
{
	// сейчас modetab[decl_type] равен MSTRUCT

	int next_field = decl_type + 3;
	int i;
	int nf = mode_get(context->sx, decl_type + 2) / 2;

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
	for (i = 0; i < nf; i++)
	{
		scaner(context);
		inition(context, mode_get(context->sx, next_field));
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
				scaner(context);
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
		scaner(context);
	}
	else
	{
		context_error(context, wait_end);
		context->cur = END;
	}
	context->leftansttype = decl_type;
}

void exprassnvoid(analyzer *context)
{
	int t = context->sx->tree[context->sx->tc - 2] < 9000 ? context->sx->tc - 3 : context->sx->tc - 2;
	int tt = context->sx->tree[t];
	if ((tt >= ASS && tt <= DIVASSAT) || (tt >= POSTINC && tt <= DECAT) || (tt >= ASSR && tt <= DIVASSATR) ||
		(tt >= POSTINCR && tt <= DECATR))
	{
		context->sx->tree[t] += 200;
	}
	--context->sopnd;
}

void exprassn(analyzer *context, int level)
{
	int leftanst;
	int leftanstdispl;
	int ltype;
	int rtype;
	int lnext;

	if (context->cur == BEGIN)
	{
		if (is_struct(context->sx, context->leftansttype))
		{
			struct_init(context, context->leftansttype);
		}
		else if (is_array(context->sx, context->leftansttype))
		{
			// пока в RuC присваивать массивы нельзя
			array_init(context, context->leftansttype);
			if (context->error_flag == 7)
			{
				context->error_flag = 6;
				return; // 1
			}
		}
		else
		{
			context_error(context, init_not_struct);
			context->error_flag = 6;
			return; // 1
		}
		context->stackoperands[++context->sopnd] = context->ansttype = context->leftansttype;
		context->anst = VAL;
	}
	else
	{
		unarexpr(context);
	}
	if (context->error_flag == 7)
	{
		context->error_flag = 6;
		return; // 1
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
		if (context->error_flag == 6)
		{
			return; // 1
		}
		context->inass = 0;

		if (leftanst == VAL)
		{
			context_error(context, unassignable);
			context->error_flag = 6;
			return; // 1
		}
		rtype = context->stackoperands[context->sopnd--]; // снимаем типы
														  // операндов со стека
		ltype = context->stackoperands[context->sopnd];

		if (intopassn(lnext) && (is_float(ltype) || is_float(rtype)))
		{
			context_error(context, int_op_for_float);
			context->error_flag = 6;
			return; // 1
		}

		if (is_array(context->sx, ltype)) // присваивать массив в массив в си нельзя
		{
			context_error(context, array_assigment);
			context->error_flag = 6;
			return; // 1
		}

		if (is_struct(context->sx, ltype)) // присваивание в структуру
		{
			if (ltype != rtype) // типы должны быть равны
			{
				context_error(context, type_missmatch);
				context->error_flag = 6;
				return; // 1
			}
			if (opp != ASS) // в структуру можно присваивать только с помощью =
			{
				context_error(context, wrong_struct_ass);
				context->error_flag = 6;
				return; // 1
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
			totree(context, mode_get(context->sx, ltype + 1)); // длина
			context->anst = leftanst;
			context->anstdispl = leftanstdispl;
		}
		else // оба операнда базового типа или указатели
		{
			if (is_pointer(context->sx, ltype) && opp != ASS) // в указатель можно присваивать только с помощью =
			{
				context_error(context, wrong_struct_ass);
				context->error_flag = 6;
				return; // 1
			}

			if (is_int(ltype) && is_float(rtype))
			{
				context_error(context, assmnt_float_to_int);
				context->error_flag = 6;
				return; // 1
			}

			toval(context);
			if (is_int(rtype) && is_float(ltype))
			{
				totree(context, WIDEN);
				context->ansttype = LFLOAT;
			}
			if (is_pointer(context->sx, ltype) && is_pointer(context->sx, rtype) && ltype != rtype)
			{
				// проверка нужна только для указателей
				context_error(context, type_missmatch);
				context->error_flag = 6;
				return; // 1
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
		if (context->error_flag == 4)
		{
			context->error_flag = 6;
			return; // 1
		}
	}
	// в виде unarexpr уже выкушано
}

void expr(analyzer *context, int level)
{
	exprassn(context, level);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	while (context->next == COMMA)
	{
		exprassnvoid(context);
		context->sopnd--;
		scaner(context);
		scaner(context);
		exprassn(context, level);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	if (level == 0)
	{
		totree(context, TExprend);
	}
}

void exprval(analyzer *context)
{
	expr(context, 1);
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
	toval(context);
	totree(context, TExprend);
}

void exprassnval(analyzer *context)
{
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 4;
		return; // 1
	}
	toval(context);
	totree(context, TExprend);
}

void array_init(analyzer *context, int decl_type)
{
	// сейчас modetab[decl_type] равен MARRAY

	int ad;
	int all = 0;

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
			ad = context->sx->tc++;
			do
			{
				scaner(context);
				all++;
				array_init(context, mode_get(context->sx, decl_type + 1));
				if (context->error_flag == 7)
				{
					return; // 1
				}
			} while (scaner(context) == COMMA);

			if (context->cur == END)
			{
				context->sx->tree[ad] = all;
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

int arrdef(analyzer *context, int t)
{
	// вызывается при описании массивов и структур из массивов сразу после idorpnt

	context->arrdim = 0;
	context->usual = 1; // описание массива без пустых границ
	if (is_pointer(context->sx, t))
	{
		context_error(context, pnt_before_array);
		context->error_flag = 5;
		return 0; // 1
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
				context_error(context, empty_init); // границы определять по инициализации можно
				context->error_flag = 5;
				return 0; // 1
			}
			// только по последнему изм.
			context->usual = 0;
		}
		else
		{
			scaner(context);
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
		t = newdecl(context->sx, MARRAY, t); // Меняем тип в identtab (увеличиваем размерность массива)
	}
	return t;
}

void decl_id(analyzer *context, int decl_type)
{
	// вызывается из block и extdecl, только эта процедура реально отводит память
	// если встретятся массивы (прямо или в структурах), их размеры уже будут в стеке

	int oldid = toidentab(context, 0, decl_type);
	int elem_type;
	int all; // all - место в дереве, где будет общее количество выражений в инициализации, для массивов - только
			 // признак (1) наличия инициализации
	int adN = 0; // warning C4701: potentially uninitialized local variable used

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
		adN = context->sx->tc++;
		decl_type = context->sx->identab[oldid + 2] =
			arrdef(context, decl_type); // Меняем тип (увеличиваем размерность массива)
		context->sx->tree[adN] = context->arrdim;
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
	totree(context, context->sx->identab[oldid + 3]);													  // displ
	totree(context, elem_type);																		  // elem_type
	totree(context, context->arrdim);																  // N
	context->sx->tree[all = context->sx->tc++] = 0;															  // all
	context->sx->tree[context->sx->tc++] = is_pointer(context->sx, decl_type) ? 0 : context->was_struct_with_arr; // proc
	totree(context, context->usual);																  // context->usual
	totree(context, 0); // массив не в структуре

	if (context->next == ASS)
	{
		scaner(context);
		scaner(context);
		context->sx->tree[all] = szof(context, decl_type);
		if (is_array(context->sx, decl_type)) // инициализация массива
		{
			context->onlystrings = 2;
			if (!context->usual)
			{
				context->sx->tree[adN]--; // это уменьшение N в Declarr
			}
			array_init(context, decl_type);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
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
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
}


void parse_statement(analyzer *const context);

/**
 *	Parse labeled statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		identifier ':' statement
 */
void parse_labeled_statement(analyzer *const context)
{
	int id;
	int i;
	int flag = 1;
	totree(context, TLabel);
	for (i = 0; flag && i < context->pgotost - 1; i += 2)
	{
		flag = context->sx->identab[context->gotost[i] + 1] != REPRTAB_POS;
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
		REPRTAB_POS = context->sx->identab[id + 1];
		if (context->gotost[i - 1] < 0)
		{
			context_error(context, repeated_label);
		}
		else
		{
			context->gotost[i - 1] = -context->line;
		}
		totree(context, id);
	}
	
	context->sx->identab[id + 2] = 1;
		
	scaner(context);
	parse_statement(context);
}

/**
 *	Parse case statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 */
void parse_case_statement(analyzer *const context)
{
	totree(context, TCase);
	
	if (!context->inswitch)
	{
		context_error(context, case_not_in_switch);
	}
	else if (context->wasdefault)
	{
		// FIXME: C99 такого не запрещает
		context_error(context, case_after_default);
	}
	
	scaner(context);
	unarexpr(context);
	condexpr(context);
	toval(context);
	
	totree(context, TExprend);
	if (context->ansttype == LFLOAT)
	{
		context_error(context, float_in_switch);
	}
	context->sopnd--;
	
	mustbe(context, COLON, no_colon_in_case);
	parse_statement(context);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 */
void parse_default_statement(analyzer *const context)
{
	totree(context, TDefault);
	context->wasdefault = 1;
	
	mustbe(context, COLON, no_colon_in_default);
	parse_statement(context);
	
	if (!context->inswitch)
		context_error(context, default_not_in_switch);
}

typedef enum _block_type
{ REGBLOCK = 1, THREAD = 2, SWITCH = -1, FUNCBODY = 0 } block_type;

/**
 *	Parse '{}' block [C99 6.8.2]
 *
 *	compound-statement:
 *  	{ block-item-list[opt] }
 *
 *	block-item-list:
 *		block-item
 *		block-item-list block-item
 *
 *	block-item:
 *		declaration
 *		statement
 */
void parse_compound_statement(analyzer *const context, block_type type)
{
	// FIXME: это временное решение
	block(context, type);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 */
void parse_expression_statement(analyzer *const context)
{
	expr(context, 0);
	exprassnvoid(context);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 */
void parse_if_statement(analyzer *const context)
{
	totree(context, TIf);
	int else_ref = context->sx->tc++;
	
	exprinbrkts(context, cond_must_be_in_brkts);
	context->sopnd--;
	parse_statement(context);
	
	if (context->next == LELSE)
	{
		scaner(context);
		context->sx->tree[else_ref] = context->sx->tc;
		parse_statement(context);
	}
	else
	{
		context->sx->tree[else_ref] = 0;
	}
}

/**
 *	Parse switch statement [C99 6.8.4.2]
 *
 *	switch-statement:
 *		'switch' '(' expression ')' statement
 */
void parse_switch_statement(analyzer *const context)
{
	totree(context, TSwitch);
	
	exprinbrkts(context, cond_must_be_in_brkts);
	if (!is_int(context->ansttype))
	{
		context_error(context, float_in_switch);
	}
	
	context->sopnd--;
	scaner(context);
	context->inswitch = 1;
	parse_compound_statement(context, SWITCH);
	context->wasdefault = 0;
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 */
void parse_while_statement(analyzer *const context)
{
	int old_inloop = context->inloop;
	totree(context, TWhile);
	
	context->inloop = 1;
	exprinbrkts(context, cond_must_be_in_brkts);
	parse_statement(context);
	context->inloop = old_inloop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 */
void parse_do_statement(analyzer *const context)
{
	int old_inloop = context->inloop;
	totree(context, TDo);
	
	context->inloop = 1;
	parse_statement(context);
	if (context->next == LWHILE)
	{
		scaner(context);
		exprinbrkts(context, cond_must_be_in_brkts);
		context->sopnd--;
	}
	else
	{
		context_error(context, wait_while_in_do_stmt);
		skip_until(context, SEMICOLON);
	}
	
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
	context->inloop = old_inloop;
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 */
void parse_for_statement(analyzer *const context)
{
	int inition_ref;
	int condition_ref;
	int increment_ref;
	int statement_ref;
	int old_inloop = context->inloop;
	
	totree(context, TFor);
	inition_ref = context->sx->tc++;
	condition_ref = context->sx->tc++;
	increment_ref = context->sx->tc++;
	statement_ref = context->sx->tc++;
	
	mustbe(context, LEFTBR, no_leftbr_in_for);
	
	if (scaner(context) == SEMICOLON)
	{
		context->sx->tree[inition_ref] = 0;
	}
	else
	{
		context->sx->tree[inition_ref] = context->sx->tc;
		// TODO: в С99 тут может быть объявление
		expr(context, 0);
		exprassnvoid(context);
		mustbe(context, SEMICOLON, no_semicolon_in_for);
	}
	
	if (scaner(context) == SEMICOLON)
	{
		context->sx->tree[condition_ref] = 0;
	}
	else
	{
		context->sx->tree[condition_ref] = context->sx->tc;
		exprval(context);
		context->sopnd--;
		mustbe(context, SEMICOLON, no_semicolon_in_for);
		context->sopnd--;
	}
	
	if (scaner(context) == RIGHTBR)
	{
		context->sx->tree[increment_ref] = 0;
	}
	else
	{
		context->sx->tree[increment_ref] = context->sx->tc;
		expr(context, 0);
		exprassnvoid(context);
		mustbe(context, RIGHTBR, no_rightbr_in_for);
	}
	
	context->sx->tree[statement_ref] = context->sx->tc;
	context->inloop = 1;
	parse_statement(context);
	context->inloop = old_inloop;
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 */
void parse_goto_statement(analyzer *const context)
{
	int i;
	int flag = 1;
	totree(context, TGoto);
	
	mustbe(context, IDENT, no_ident_after_goto);
	for (i = 0; flag && i < context->pgotost - 1; i += 2)
	{
		flag = context->sx->identab[context->gotost[i] + 1] != REPRTAB_POS;
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
			return;
		}
		totree(context, context->gotost[context->pgotost++] = id);
	}
	context->gotost[context->pgotost++] = context->line;
}

/**
 *	Parse continue statement [C99 6.8.6.2]
 *
 *	jump-statement:
 *		'continue' ';'
 */
void parse_continue_statement(analyzer *const context)
{
	totree(context, TContinue);
	
	if (!context->inloop)
	{
		context_error(context, continue_not_in_loop);
	}
	
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 */
void parse_break_statement(analyzer *const context)
{
	totree(context, TBreak);
	
	if (!(context->inloop || context->inswitch))
	{
		context_error(context, break_not_in_loop_or_switch);
	}
	
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *		'return' braced-init-list ';'
 */
void parse_return_statement(analyzer *const context)
{
	int return_type = mode_get(context->sx, context->functype + 1);
	context->wasret = 1;
	if (context->next == SEMICOLON)
	{
		totree(context, TReturnvoid);
		scaner(context);
		if (return_type != LVOID)
			context_error(context, no_ret_in_func);
	}
	else
	{
		if (return_type != LVOIDASTER)
		{
			if (return_type == LVOID)
			{
				context_error(context, notvoidret_in_void_func);
			}
			totree(context, TReturnval);
			totree(context, szof(context, return_type));
			scaner(context);
			expr(context, 1);
			toval(context);
			context->sopnd--;
			
			if (return_type == LFLOAT && context->ansttype == LINT)
			{
				totree(context, WIDEN);
			}
			else if (return_type != context->ansttype)
			{
				context_error(context, bad_type_in_ret);
			}
			totree(context, TExprend);
			mustbe(context, SEMICOLON, no_semicolon_after_stmt);
		}
	}
}

/**	Parse t_create_direct statement [RuC] */
void parse_create_direct_statement(analyzer *const context)
{
	totree(context, CREATEDIRECTC);
	parse_compound_statement(context, THREAD);
	totree(context, EXITC);
}

/**	Parse printid statement [RuC] */
void parse_printid_statement(analyzer *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_printid);
	
	while (context->next != RIGHTBR)
	{
		if (context->next == IDENT)
		{
			scaner(context);
			applid(context);
			totree(context, TPrintid);
			totree(context, context->lastid);
		}
		else
		{
			context_error(context, no_ident_in_printid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next == COMMA)
		{
			scaner(context);
		}
		else
		{
			break;
		}
	}
	
	mustbe(context, RIGHTBR, no_rightbr_in_printid);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
	
}

/**	Parse printf statement [RuC] */
void parse_printf_statement(analyzer *const context)
{
	int formatstr[MAXSTRINGL];
	int formattypes[MAXPRINTFPARAMS];
	int placeholders[MAXPRINTFPARAMS];
	int sumsize = 0;
	
	mustbe(context, LEFTBR, no_leftbr_in_printf);
	
	if (context->next != STRING)
	{
		context_error(context, wrong_first_printf_param);
		skip_until(context, SEMICOLON);
		return;
	}
	scaner(context);	// Для форматирующей строки
	
	for (int i = 0; i <= context->num; i++)
	{
		formatstr[i] = context->lexstr[i];
	}
	// FIXME: lexstr должна заканчиваться 0
	formatstr[context->num] = 0;
	
	int expected_param_number = evaluate_params(context, context->num, formatstr, formattypes, placeholders);
	int actual_param_number = 0;
	//for (int i = 0; scaner(context) == COMMA; i++)
	while (context->next != RIGHTBR && actual_param_number != expected_param_number)
	{
		scaner(context);
		
		scaner(context);
		exprassn(context, 1);
		toval(context);
		totree(context, TExprend);
		
		if (formattypes[actual_param_number] == LFLOAT && context->ansttype == LINT)
		{
			insertwiden(context);
		}
		else if (formattypes[actual_param_number] != context->ansttype)
		{
			// FIXME: для этого заводится отдельное поле в analyzer
			context->bad_printf_placeholder = placeholders[actual_param_number];
			context_error(context, wrong_printf_param_type);
		}
		
		sumsize += szof(context, formattypes[actual_param_number]);
		--context->sopnd;
		actual_param_number++;
		if (context->next != COMMA)
		{
			break;
		}
	}
	
	mustbe(context, RIGHTBR, no_rightbr_in_printf);
	
	if (actual_param_number != expected_param_number)
	{
		context_error(context, wrong_printf_param_number);
	}
	
	totree(context, TString);
	totree(context, context->num);
	
	for (int i = 0; i < context->num; i++)
	{
		totree(context, formatstr[i]);
	}
	totree(context, TExprend);
	
	totree(context, TPrintf);
	totree(context, sumsize);
}

/**	Parse print statement [RuC] */
void parse_print_statement(analyzer *const context)
{
	exprassninbrkts(context, print_without_br);
	context->sx->tc--;
	totree(context, TPrint);
	totree(context, context->ansttype);
	totree(context, TExprend);
	
	if (is_pointer(context->sx, context->ansttype))
	{
		context_error(context, pointer_in_print);
	}
	context->sopnd--;
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**	Parse scanf statement [RuC] */
void parse_scanf_statement(analyzer *const context);

/**	Parse getid statement [RuC] */
void parse_getid_statement(analyzer *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_getid);
	
	while (context->next != RIGHTBR)
	{
		if (context->next == IDENT)
		{
			scaner(context);
			applid(context);
			totree(context, TGetid);
			totree(context, context->lastid);
		}
		else
		{
			context_error(context, no_ident_in_getid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next == COMMA)
		{
			scaner(context);
		}
		else
		{
			break;
		}
	}
	
	mustbe(context, RIGHTBR, no_rightbr_in_getid);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse statement [C99 6.8]
 *
 *	statement:
 *		labeled-statement
 *		compound-statement
 *		expression-statement
 *		selection-statement
 *		iteration-statement
 *		jump-statement
 */
void parse_statement(analyzer *const context)
{
	int old_wasdefault = context->wasdefault;
	int old_inswitch = context->inswitch;
	int old_inloop = context->inloop;

	context->wasdefault = 0;
	scaner(context);
	if ((is_int(context->cur) || is_float(context->cur) || context->cur == LVOID ||
		 context->cur == LSTRUCT) &&
		context->blockflag)
	{
		context_error(context, decl_after_strmt);
	}
	else
	{
		context->blockflag = 1;

		switch (context->cur)
		{
			case SEMICOLON:
				totree(context, NOP);
				break;
				
			case LCASE:
				parse_case_statement(context);
				break;
				
			case LDEFAULT:
				parse_default_statement(context);
				break;
				
			case BEGIN:
				parse_compound_statement(context, REGBLOCK);
				break;
				
			case LIF:
				parse_if_statement(context);
				break;
				
			case LSWITCH:
				parse_switch_statement(context);
				break;
				
			case LWHILE:
				parse_while_statement(context);
				break;
				
			case LDO:
				parse_do_statement(context);
				break;
				
			case LFOR:
				parse_for_statement(context);
				break;
				
			case LGOTO:
				parse_goto_statement(context);
				break;
				
			case LCONTINUE:
				parse_continue_statement(context);
				break;
				
			case LBREAK:
				parse_break_statement(context);
				break;
				
			case LRETURN:
				parse_return_statement(context);
				break;
				
			case TCREATEDIRECT:
				parse_create_direct_statement(context);
				break;
				
			case PRINTID:
				parse_printid_statement(context);
				break;
				
			case PRINTF:
				parse_printf_statement(context);
				break;
				
			case PRINT:
				parse_print_statement(context);
				break;
				
			case GETID:
				parse_getid_statement(context);
				break;
				
			case IDENT:
				if (context->next == COLON)
				{
					parse_labeled_statement(context);
					break;
				}
				
			default:
				parse_expression_statement(context);
		}
	}

	context->wasdefault = old_wasdefault;
	context->inswitch = old_inswitch;
	context->inloop = old_inloop;
}

int idorpnt(analyzer *context, int e, int t)
{
	if (context->next == LMULT)
	{
		scaner(context);
		t = t == LVOID ? LVOIDASTER : newdecl(context->sx, MPOINT, t);
	}
	mustbe_complex(context, IDENT, e);
	return t;
}

int struct_decl_list(analyzer *context)
{
	int field_count = 0;
	int t;
	int elem_type;
	int curdispl = 0;
	int wasarr = 0;
	int tstrbeg;
	int loc_modetab[100];
	int locmd = 3;

	loc_modetab[0] = MSTRUCT;
	tstrbeg = context->sx->tc;
	totree(context, TStructbeg);
	context->sx->tree[context->sx->tc++] = 0; // тут будет номер иниц процедуры

	scaner(context);
	scaner(context);

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
			int adN;
			int all;
			totree(context, TDeclarr);
			adN = context->sx->tc++;
			t = arrdef(context, elem_type); // Меняем тип (увеличиваем размерность массива)
			if (context->error_flag == 5)
			{
				context->error_flag = 3;
				return 0;
			}
			context->sx->tree[adN] = context->arrdim;

			totree(context, TDeclid);
			totree(context, curdispl);
			totree(context, elem_type);
			totree(context, context->arrdim);							 // N
			context->sx->tree[all = context->sx->tc++] = 0;						 // all
			context->sx->tree[context->sx->tc++] = context->was_struct_with_arr; // proc
			totree(context, context->usual);							 // context->usual
			totree(context, 1); // признак, что массив в структуре
			wasarr = 1;
			if (context->next == ASS)
			{
				scaner(context);
				scaner(context);
				if (is_array(context->sx, t)) // инициализация массива
				{
					context->onlystrings = 2;
					context->sx->tree[all] = 1;
					if (!context->usual)
					{
						context->sx->tree[adN]--; // это уменьшение N в Declarr
					}
					array_init(context, t);
					if (context->error_flag == 7)
					{
						context->error_flag = 3;
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
		if (scaner(context) != SEMICOLON)
		{
			context_error(context, no_semicomma_in_struct);
			context->buf_cur = context->next;
			context->next = context->cur;
			context->cur = BEGIN;
			context->buf_flag++;
		}
	} while (scaner(context) != END);

	if (wasarr)
	{
		totree(context, TStructend);
		totree(context, tstrbeg);
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

int gettype(analyzer *context)
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
			scaner(context);
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
				context->sx->identab[lid + 2] = struct_decl_list(context);
				context->sx->identab[lid + 3] = 1000 + context->was_struct_with_arr;
				return context->sx->identab[lid + 2];
			}
			else // struct key это применение типа
			{
				if (l == 1)
				{
					context_error(context, ident_is_not_declared);
					context->error_flag = 3;
					return 0; // 1
				}
				context->was_struct_with_arr = context->sx->identab[l + 3] - 1000;
				return (context->sx->identab[l + 2]);
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

		if (context->sx->identab[context->lastid + 3] < 1000)
		{
			context_error(context, ident_not_type);
			context->error_flag = 3;
			return 0; // 1
		}

		context->was_struct_with_arr = context->sx->identab[context->lastid + 3] - 1000;
		return context->sx->identab[context->lastid + 2];
	}

	context_error(context, not_decl);
	context->error_flag = 3;
	return 0; // 1
}

void block(analyzer *context, int b)
{
	// если b=1, то это просто блок,
	// b = 2 - блок нити,
	// b = -1 - блок в switch, иначе
	// b = 0 - это блок функции

	int oldinswitch = context->inswitch;
	int notended = 1;
	int i;
	int olddispl = 0; // warning C4701: potentially uninitialized local variable used
	int oldlg = context->lg;
	int firstdecl;

	context->inswitch = b < 0;
	totree(context, TBegin);
	if (b)
	{
		olddispl = context->displ;
		context->curid = context->sx->id;
	}
	context->blockflag = 0;

	while (is_int(context->next) || is_float(context->next) || context->next == LSTRUCT ||
		   context->next == LVOID)
	{
		int repeat = 1;
		scaner(context);
		firstdecl = gettype(context);
		if (context->error_flag == 3)
		{
			context->error_flag = 1;
			continue;
		}
		if (context->wasstructdef && context->next == SEMICOLON)
		{
			scaner(context);
			continue;
		}
		do
		{
			int temp = idorpnt(context, after_type_must_be_ident, firstdecl);

			if (context->error_flag == after_type_must_be_ident)
			{
				context->error_flag = 1;
				break;
			}

			decl_id(context, temp);
			if (context->error_flag == 4)
			{
				context->error_flag = 1;
				break;
			}
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
				context_error(context, def_must_end_with_semicomma);
				context->cur = SEMICOLON;
				repeat = 0;
			}
		} while (repeat);
	}

	// кончились описания, пошли операторы до }

	do
	{
		if (context->next == LEOF)
		{
			context_error(context, wait_end);
			return;
		}
		if (b == 2 ? context->next == TEXIT : context->next == END)
		{
			scaner(context);
			notended = 0;
		}
		else
		{
			parse_statement(context);
			if (context->cur == LEOF && context->error_flag)
			{
				return;
			}
		}
	} while (notended);

	if (b)
	{
		for (i = context->sx->id - 4; i >= context->curid; i -= 4)
		{
			REPRTAB[context->sx->identab[i + 1] + 1] = context->sx->identab[i];
		}
		context->displ = olddispl;
	}
	context->inswitch = oldinswitch;
	context->lg = oldlg;
	totree(context, TEnd);
}

void function_definition(analyzer *context)
{
	int fn = context->sx->identab[context->lastid + 3];
	int i;
	int pred;
	int oldrepr = REPRTAB_POS;
	int ftype;
	int n;
	int fid = context->lastid;
	int olddispl = context->displ;

	context->pgotost = 0;
	context->functype = context->sx->identab[context->lastid + 2];
	ftype = mode_get(context->sx, context->functype + 1);
	n = mode_get(context->sx, context->functype + 2);
	context->wasret = 0;
	context->displ = 3;
	context->maxdispl = 3;
	context->lg = 1;
	if ((pred = context->sx->identab[context->lastid]) > 1) // был прототип
	{
		if (context->functype != context->sx->identab[pred + 2])
		{
			context_error(context, decl_and_def_have_diff_type);
			return; // 1
		}
		context->sx->identab[pred + 3] = fn;
	}
	context->curid = context->sx->id;
	for (i = 0; i < n; i++)
	{
		context->type = mode_get(context->sx, context->functype + i + 3);
		int temp = func_get(context->sx, fn + i + 1);
		if (temp == INT_MAX)
		{
			context->error_flag = 1;
			return;
		}

		REPRTAB_POS = temp;
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
	func_set(context->sx, fn, context->sx->tc);
	totree(context, TFuncdef);
	totree(context, fid);
	pred = context->sx->tc++;
	REPRTAB_POS = oldrepr;

	block(context, 0);

	// if (ftype == LVOID && context->sx->tree[context->sx->tc - 1] != TReturnvoid)
	// {
	context->sx->tc--;
	totree(context, TReturnvoid);
	totree(context, TEnd);
	// }
	if (ftype != LVOID && !context->wasret)
	{
		context_error(context, no_ret_in_func);
		context->error_flag = 1;
		return; // 1
	}
	for (i = context->sx->id - 4; i >= context->curid; i -= 4)
	{
		REPRTAB[context->sx->identab[i + 1] + 1] = context->sx->identab[i];
	}

	for (i = 0; i < context->pgotost - 1; i += 2)
	{
		REPRTAB_POS = context->sx->identab[context->gotost[i] + 1];
		context->hash = context->gotost[i + 1];
		if (context->hash < 0)
		{
			context->hash = -context->hash;
		}
		if (!context->sx->identab[context->gotost[i] + 2])
		{
			context_error(context, label_not_declared);
			context->error_flag = 1;
			return; // 1
		}
	}
	context->curid = 2; // все функции описываются на одном уровне
	context->sx->tree[pred] = context->maxdispl; // + 1;?
	context->lg = -1;
	context->displ = olddispl;
}

int func_declarator(analyzer *context, int level, int func_d, int firstdecl)
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
			context->type = gettype(context);
			if (context->error_flag == 3)
			{
				context->error_flag = 2;
				return 0;
			}
			if (context->next == LMULT)
			{
				maybe_fun = 1;
				scaner(context);
				context->type = context->type == LVOID ? LVOIDASTER : newdecl(context->sx, MPOINT, context->type);
			}
			if (level)
			{
				if (context->next == IDENT)
				{
					scaner(context);
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
					scaner(context);
					mustbe(context, RIGHTSQBR, wait_right_sq_br);
					context->type = newdecl(context->sx, MARRAY, context->type);
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
				scaner(context);
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

/** Генерация дерева */
void ext_decl(analyzer *context)
{
	getnext(context);
	nextch(context);
	context->next = scan(context);
	
	int i;
	context->temp_tc = context->sx->tc;
	do // top levext_declel описания переменных и функций до конца файла
	{
		int repeat = 1;
		int funrepr;
		int first = 1;
		context->wasstructdef = 0;
		scaner(context);

		context->firstdecl = gettype(context);
		if (context->error_flag == 3)
		{
			context->error_flag = 1;
			continue;
		}
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
				context->type = context->firstdecl == LVOID ? LVOIDASTER : newdecl(context->sx, MPOINT, context->firstdecl);
			}
			mustbe_complex(context, IDENT, after_type_must_be_ident);
			if (context->error_flag == after_type_must_be_ident)
			{
				context->error_flag = 1;
				break;
			}

			if (context->next == LEFTBR) // определение или предописание функции
			{
				int oldfuncnum = context->sx->funcnum++;
				int firsttype = context->type;
				funrepr = REPRTAB_POS;
				scaner(context);
				scaner(context);

				// выкушает все параметры до ) включительно
				context->type = func_declarator(context, first, 3, firsttype);
				if (context->error_flag == 2)
				{
					context->error_flag = 1;
					break;
				}

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
				if (context->error_flag == 5)
				{
					context->error_flag = 1;
					break;
				}

				if (context->next == BEGIN)
				{
					scaner(context);
					if (context->func_def == 2)
					{
						context_error(context, func_decl_req_params);
						goto ex;
					}

					function_definition(context);
					goto ex;
				}
				else
				{
					if (context->func_def == 1)
					{
						context_error(context, function_has_no_body);
						goto ex;
					}
				}
			}
			else if (context->firstdecl == LVOID)
			{
				context_error(context, only_functions_may_have_type_VOID);
				goto ex;
			}

			// описания идентов-не-функций

			if (context->func_def == 3)
			{
				decl_id(context, context->type);
			}

			if (context->error_flag == 4)
			{
				context->error_flag = 1;
				break;
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
				context_error(context, def_must_end_with_semicomma);
				context->cur = SEMICOLON;
				repeat = 0;
			}
		} while (repeat);

	ex:;
	} while (context->next != LEOF);

	if (context->sx->wasmain == 0)
	{
		context_error(context, no_main_in_program);
	}
	for (i = 0; i <= context->prdf; i++)
	{
		if (context->predef[i])
		{
			context_error(context, predef_but_notdef);
		}
	}
	totree(context, TEnd);
}
/*
				printf(
				context->buf_cur = context->next;
				context->next = context->cur;
				context->cur = BEGIN;
				context->buf_flag++;
*/
