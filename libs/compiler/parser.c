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


/**	Check if the set of tokens has token in it*/
int has_token_set(const unsigned int tokens, const token token)
{
	return (tokens & token) != 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int parse(parser *const parser)
{
	get_char(parser->lexer);
	get_char(parser->lexer);
	consume_token(parser);

	do
	{
		parse_external_declaration(parser);
	} while (parser->next_token != eof);

	tree_add(parser->sx, TEnd);

	return parser->was_error || parser->lexer->was_error;
}


void parser_error(parser *const parser, const int num, ...)
{
	parser->was_error = 1;

	va_list args;
	va_start(args, num);

	error(parser->io, num, args);
}

void consume_token(parser *const parser)
{
	parser->curr_token = parser->next_token;
	parser->next_token = lex(parser->lexer);
}

int try_consume_token(parser *const parser, const token expected)
{
	if (parser->next_token == expected)
	{
		consume_token(parser);
		return 1;
	}

	return 0;
}

void expect_and_consume_token(parser *const parser, const token expected, const enum ERROR err)
{
	if (!try_consume_token(parser, expected))
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

			case l_square:
				consume_token(parser);
				skip_until(parser, r_square);
				break;

			case l_brace:
				consume_token(parser);
				skip_until(parser, r_brace);
				break;

			case question:
				consume_token(parser);
				skip_until(parser, colon);
				break;

			case r_paren:
			case r_square:
			case r_brace:
			case colon:
			case semicolon:
				if (has_token_set(tokens, parser->next_token))
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


int is_function(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_function;
}

int is_array(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_array;
}

int is_string(syntax *const sx, const item_t mode)
{
	return is_array(sx, mode) && mode_get(sx, (size_t)mode + 1) == mode_character;
}

int is_pointer(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_pointer;
}

int is_struct(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_struct;
}

int is_float(const item_t mode)
{
	return mode == mode_float;
}

int is_int(const item_t mode)
{
	return mode == mode_integer;
}

int is_void(const item_t mode)
{
	return mode == mode_void;
}

int is_undefined(const item_t mode)
{
	return mode == mode_undefined;
}


size_t to_identab(parser *const parser, const size_t repr, const item_t type, const item_t mode)
{
	const size_t ret = ident_add(parser->sx, repr, type, mode, parser->func_def);
	parser->lastid = 0;

	if (ret == SIZE_MAX)
	{
		parser_error(parser, redefinition_of_main);
	}
	else if (ret == SIZE_MAX - 1)
	{
		char buffer[MAXSTRINGL];
		repr_get_ident(parser->sx, repr, buffer);
		parser_error(parser, repeated_decl, buffer);
	}
	else
	{
		parser->lastid = (int)ret;
	}

	return ret;
}

item_t to_modetab(parser *const parser, const item_t mode, const item_t element)
{
	item_t temp[2];
	temp[0] = mode;
	temp[1] = element;
	return (item_t)mode_add(parser->sx, temp, 2);
}
