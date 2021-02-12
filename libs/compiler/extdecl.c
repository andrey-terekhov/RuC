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


int scanner(analyzer *context)
{
	context->cur = context->next;
	if (!context->buf_flag)
	{
		context->next = lex(context->lxr);
	}
	else
	{
		context->next = context->buf_cur;
		context->buf_flag--;
	}
	
	//	 if(context->kw)
	//			printf("scaner context->cur %i context->next %i buf_flag %i\n",
	//			context->cur, context->next, context->buf_flag);
	return context->cur;
}

item_t newdecl(syntax *const sx, const item_t type, const item_t element_type)
{
	item_t temp[2];
	temp[0] = type;
	temp[1] = element_type;
	return (item_t)mode_add(sx, temp, 2);
}

int double_to_tree(vector *const tree, const double num)
{
	int64_t num64;
	memcpy(&num64, &num, sizeof(int64_t));

	int32_t fst = num64 & 0x00000000ffffffff;
	int32_t snd = (num64 & 0xffffffff00000000) >> 32;

#if INT_MIN < ITEM_MIN || INT_MAX > ITEM_MAX
	if (fst < ITEM_MIN || fst > ITEM_MAX || snd < ITEM_MIN || snd > ITEM_MAX)
	{
		return -1;
	}
#endif

	size_t ret = vector_add(tree, fst);
	ret = ret != SIZE_MAX ? vector_add(tree, snd) : SIZE_MAX;
	return ret == SIZE_MAX;
}

double double_from_tree(vector *const tree)
{
	const size_t index = vector_size(tree) - 2;

	const int64_t fst = (int64_t)vector_get(tree, index) & 0x00000000ffffffff;
	const int64_t snd = (int64_t)vector_get(tree, index + 1) & 0x00000000ffffffff;
	int64_t num64 = (snd << 32) | fst;

	vector_resize(tree, index);

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
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
		{
			char buffer[MAXSTRINGL];
			repr_get_ident(context->sx, REPRTAB_POS, buffer);
			error(io, num, buffer);
		}
		break;
		case label_not_declared:
		{
			char buffer[MAXSTRINGL];
			repr_get_ident(context->sx, REPRTAB_POS, buffer);
			error(io, num, context->sx->hash, buffer);
		}
		break;
		default:
			error(io, num);
	}

	context->error_flag = 1;

	if (context->temp_tc > vector_size(&TREE))
	{
		vector_increase(&TREE, context->temp_tc - vector_size(&TREE));
	}
	else
	{
		vector_resize(&TREE, context->temp_tc);
	}

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

int is_function(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == mode_function;
}

int is_array(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == mode_array;
}

int is_string(syntax *const sx, const int t)
{
	return is_array(sx, t) && mode_get(sx, t + 1) == LCHAR;
}

int is_pointer(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == mode_pointer;
}

int is_struct(syntax *const sx, const int t)
{
	return t > 0 && mode_get(sx, t) == mode_struct;
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
	: type == LFLOAT ? 2 : (is_struct(context->sx, type)) ? (int)mode_get(context->sx, type + 1) : 1;
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
		scanner(context);
	}
}

void mustbe_complex(analyzer *context, int what, int e)
{
	if (scanner(context) != what)
	{
		context_error(context, e);
		context->error_flag = e;
	}
}

void totree(analyzer *context, item_t op)
{
	vector_add(&TREE, op);
}

void totreef(analyzer *context, item_t op)
{
	vector_add(&TREE, op);
	if (context->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		const size_t index = vector_size(&TREE) - 1;
		vector_set(&TREE, index, vector_get(&TREE, index) + 50);
	}
}

int toidentab(analyzer *context, int f, int type)
{
	const size_t ret = ident_add(context->sx, REPRTAB_POS, f, type, context->func_def);
	context->lastid = 0;

	if (ret == SIZE_MAX)
	{
		context_error(context, redefinition_of_main); //--
		context->error_flag = 5;
	}
	else if (ret == SIZE_MAX - 1)
	{
		context_error(context, repeated_decl);
		context->error_flag = 5;
	}
	else
	{
		context->lastid = (int)ret;
	}
	
	return context->lastid;
}

/** Генерация дерева */
void ext_decl(analyzer *context)
{
	get_char(context->lxr);
	get_char(context->lxr);
	context->next = lex(context->lxr);
	
	context->temp_tc = vector_size(&TREE);
	do // top levext_declel описания переменных и функций до конца файла
	{
		int repeat = 1;
		int funrepr;
		int first = 1;
		context->wasstructdef = 0;
		scanner(context);

		context->firstdecl = (int)gettype(context);
		if (context->error_flag == 3)
		{
			context->error_flag = 1;
			continue;
		}
		if (context->wasstructdef && context->next == SEMICOLON) // struct point {float x, y;};
		{
			scanner(context);
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
				scanner(context);
				context->type = context->firstdecl == LVOID ? LVOIDASTER : (int)newdecl(context->sx, mode_pointer, context->firstdecl);
			}
			mustbe_complex(context, IDENT, after_type_must_be_ident);
			if (context->error_flag == after_type_must_be_ident)
			{
				context->error_flag = 1;
				break;
			}

			if (context->next == LEFTBR) // определение или предописание функции
			{
				size_t oldfuncnum = vector_size(&context->sx->functions);
				vector_increase(&context->sx->functions, 1);
				int firsttype = context->type;
				funrepr = REPRTAB_POS;
				scanner(context);
				scanner(context);

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

				toidentab(context, (int) oldfuncnum, context->type);
				if (context->error_flag == 5)
				{
					context->error_flag = 1;
					break;
				}

				if (context->next == BEGIN)
				{
					scanner(context);
					if (context->func_def == 2)
					{
						context_error(context, func_decl_req_params);
						break;
					}

					function_definition(context);
					break;
				}
				else
				{
					if (context->func_def == 1)
					{
						context_error(context, function_has_no_body);
						break;
					}
				}
			}
			else if (context->firstdecl == LVOID)
			{
				context_error(context, only_functions_may_have_type_VOID);
				break;
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
				scanner(context);
				first = 0;
			}
			else if (context->next == SEMICOLON)
			{
				scanner(context);
				repeat = 0;
			}
			else
			{
				context_error(context, def_must_end_with_semicomma);
				context->cur = SEMICOLON;
				repeat = 0;
			}
		} while (repeat);

	} while (context->next != LEOF);
	totree(context, TEnd);
}
