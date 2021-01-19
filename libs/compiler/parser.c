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


/**	Check if the set of tokens has token in it*/
int has_token_set(const unsigned int tokens, const TOKEN token)
{
	return (tokens & token) != 0;
}


int scanner(parser *context)
{
	context->curr_token = context->next_token;
	if (!context->buf_flag)
	{
		context->next_token = lex(context->lxr);
	}
	else
	{
		context->next_token = context->buf_cur;
		context->buf_flag--;
	}
	
	//	 if(context->kw)
	//			printf("scaner context->cur %i context->next %i buf_flag %i\n",
	//			context->cur, context->next, context->buf_flag);
	return context->curr_token;
}

int newdecl(syntax *const sx, const int type, const int element_type)
{
	int temp[2];
	temp[0] = type;
	temp[1] = element_type;
	return (int)mode_add(sx, temp, 2);
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

int szof(parser *context, int type)
{
	return context->next_token == LEFTSQBR ? 1
	: type == LFLOAT ? 2 : (is_struct(context->sx, type)) ? mode_get(context->sx, type + 1) : 1;
}

void mustbe(parser *context, int what, int e)
{
	if (context->next_token != what)
	{
		parser_error(context, e);
		context->curr_token = what;
	}
	else
	{
		scanner(context);
	}
}

void mustbe_complex(parser *context, int what, int e)
{
	if (scanner(context) != what)
	{
		parser_error(context, e);
		context->was_error = e;
	}
}

void totree(parser *context, int op)
{
	context->sx->tree[context->sx->tc++] = op;
}

void totreef(parser *context, int op)
{
	context->sx->tree[context->sx->tc++] = op;
	if (context->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		context->sx->tree[context->sx->tc - 1] += 50;
	}
}

int toidentab(parser *context, int f, int type)
{
	const size_t ret = ident_add(context->sx, REPRTAB_POS, f, type, context->func_def);
	context->lastid = 0;

	if (ret == SIZE_MAX)
	{
		parser_error(context, redefinition_of_main); //--
		context->was_error = 5;
	}
	else if (ret == SIZE_MAX - 1)
	{
		parser_error(context, repeated_decl);
		context->was_error = 5;
	}
	else
	{
		context->lastid = (int)ret;
	}
	
	return context->lastid;
}

void applid(parser *context)
{
	context->lastid = REPRTAB[REPRTAB_POS + 1];
	if (context->lastid == 1)
	{
		parser_error(context, ident_is_not_declared);
		context->was_error = 5;
	}

}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parser_error(parser *const context, const enum ERROR err)
{
	context->was_error = 1;
	switch (err)
	{
		case not_primary:
			error(context->io, err, context->curr_token);
			break;
		case bad_toval:
			error(context->io, err, context->ansttype);
			break;
		case wrong_printf_param_type:
		case printf_unknown_format_placeholder:
			error(context->io, err, context->bad_printf_placeholder);
			break;
		case repeated_decl:
		case ident_is_not_declared:
		case repeated_label:
		case no_field:
			error(context->io, err, REPRTAB, REPRTAB_POS);
			break;
		case label_not_declared:
			error(context->io, err, context->sx->hash, REPRTAB, REPRTAB_POS);
			break;
		default:
			error(context->io, err);
	}
}

void consume_token(parser *const parser)
{
	parser->curr_token = parser->next_token;
	parser->next_token = lex(parser->lxr);
}

void try_consume_token(parser *const parser, const TOKEN expected, const enum ERROR err)
{
	if (parser->next_token == expected)
	{
		consume_token(parser);
	}
	else
	{
		parser_error(parser, err);
	}
}

void skip_until(parser *const parser, const unsigned int tokens)
{
	while (parser->next_token != eof)
	{
		switch (parser->next_token)
		{
			case l_paren:
				consume_token(parser);
				skip_until(parser, r_paren);
				break;

			case r_paren:
				if (has_token_set(tokens, r_paren))
				{
					return;
				}
				else
				{
					consume_token(parser);
					break;
				}

			case l_square:
				consume_token(parser);
				skip_until(parser, r_square);
				break;

			case r_square:
				if (has_token_set(tokens, r_square))
				{
					return;
				}
				else
				{
					consume_token(parser);
					break;
				}

			case l_brace:
				consume_token(parser);
				skip_until(parser, r_brace);
				break;

			case r_brace:
				if (has_token_set(tokens, r_brace))
				{
					return;
				}
				else
				{
					consume_token(parser);
					break;
				}

			case question:
				consume_token(parser);
				skip_until(parser, colon);
				break;

			case COLON:
				if (has_token_set(tokens, colon))
				{
					return;
				}
				else
				{
					consume_token(parser);
					break;
				}

			case semicolon:
				if (has_token_set(tokens, semicolon))
				{
					return;
				}
				else
				{
					consume_token(parser);
					break;
				}

			default:
				consume_token(parser);
				break;
		}
	}
}
