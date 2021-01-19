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

#include "parser.h"
#include "errors.h"
#include "defs.h"
#include "lexer.h"
#include <string.h>

int has_token_set(const unsigned int left, const unsigned int right)
{
	return (left & right) != 0;
}

/**
 *	Read tokens until one of the specified tokens
 *
 *	@param	context		Parser structure
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
				scanner(context);
				skip_until(context, RIGHTBR);
				break;

			case RIGHTBR:
				if (has_token_set(tokens, RIGHTBR))
				{
					return;
				}
				else
				{
					scanner(context);
					break;
				}

			case LEFTSQBR:
				scanner(context);
				skip_until(context, RIGHTSQBR);
				break;

			case RIGHTSQBR:
				if (has_token_set(tokens, RIGHTSQBR))
				{
					return;
				}
				else
				{
					scanner(context);
					break;
				}

			case BEGIN:
				scanner(context);
				skip_until(context, END);
				break;

			case END:
				if (has_token_set(tokens, END))
				{
					return;
				}
				else
				{
					scanner(context);
					break;
				}

			case QUEST:
				scanner(context);
				skip_until(context, COLON);
				break;

			case COLON:
				if (has_token_set(tokens, COLON))
				{
					return;
				}
				else
				{
					scanner(context);
					break;
				}

			case SEMICOLON:
				if (has_token_set(tokens, SEMICOLON))
				{
					return;
				}
				else
				{
					scanner(context);
					break;
				}

			default:
				scanner(context);
				break;
		}
	}
}


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
			error(io, num, REPRTAB, REPRTAB_POS);
			break;
		case label_not_declared:
			error(io, num, context->sx->hash, REPRTAB, REPRTAB_POS);
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

int evaluate_params(analyzer *context, int num, char32_t formatstr[], int formattypes[], char32_t placeholders[])
{
	int num_of_params = 0;
	int i = 0;
	char32_t fsi;

	//	for (i=0; i<num; i++)
	//		printf("%c %i\n", formatstr[i], formatstr[i]);

	for (i = 0; i < num; i++)
	{
		if (formatstr[i] == '%')
		{
			if (fsi = formatstr[++i], fsi != '%')
			{
				if (num_of_params == MAXPRINTFPARAMS)
				{
					context_error(context, too_many_printf_params);
					return 0;
				}

				placeholders[num_of_params] = fsi;
			}
			switch (fsi) // Если добавляется новый спецификатор -- не забыть
						 // внести его в switch в bad_printf_placeholder
			{
				case 'i':
				case 1094: // 'ц'
					formattypes[num_of_params++] = LINT;
					break;

				case 'c':
				case 1083: // л
					formattypes[num_of_params++] = LCHAR;
					break;

				case 'f':
				case 1074: // в
					formattypes[num_of_params++] = LFLOAT;
					break;

				case 's':
				case 1089: // с
					formattypes[num_of_params++] = newdecl(context->sx, MARRAY, LCHAR);
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

	return num_of_params;
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

void applid(analyzer *context)
{
	context->lastid = REPRTAB[REPRTAB_POS + 1];
	if (context->lastid == 1)
	{
		context_error(context, ident_is_not_declared);
		context->error_flag = 5;
	}

}
