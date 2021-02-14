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


/** Check if the set of tokens has token in it */
int has_token_set(const unsigned int tokens, const token_t token)
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


int parse(parser *const prs)
{
	get_char(prs->lxr);
	get_char(prs->lxr);
	consume_token(prs);

	do
	{
		parse_external_declaration(prs);
	} while (prs->next_token != eof);

	tree_add(prs->sx, TEnd);

	return prs->was_error || prs->lxr->was_error;
}


void parser_error(parser *const prs, const int num, ...)
{
	prs->was_error = 1;

	va_list args;
	va_start(args, num);

	error(prs->io, num, args);
}

void consume_token(parser *const prs)
{
	prs->curr_token = prs->next_token;
	prs->next_token = lex(prs->lxr);
}

int try_consume_token(parser *const prs, const token_t expected)
{
	if (prs->next_token == expected)
	{
		consume_token(prs);
		return 1;
	}

	return 0;
}

void expect_and_consume_token(parser *const prs, const token_t expected, const error_t err)
{
	if (!try_consume_token(prs, expected))
	{
		parser_error(prs, err);
	}
}

void skip_until(parser *const prs, const uint8_t tokens)
{
	while (prs->next_token != eof)
	{
		switch (prs->next_token)
		{
			case l_paren:
				consume_token(prs);
				skip_until(prs, r_paren);
				break;

			case l_square:
				consume_token(prs);
				skip_until(prs, r_square);
				break;

			case l_brace:
				consume_token(prs);
				skip_until(prs, r_brace);
				break;

			case question:
				consume_token(prs);
				skip_until(prs, colon);
				break;

			case r_paren:
			case r_square:
			case r_brace:
			case colon:
			case semicolon:
				if (has_token_set(tokens, prs->next_token))
				{
					return;
				}
				else
				{
					consume_token(prs);
					break;
				}

			default:
				consume_token(prs);
				break;
		}
	}
}


int mode_is_function(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_function;
}

int mode_is_array(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_array;
}

int mode_is_string(syntax *const sx, const item_t mode)
{
	return mode_is_array(sx, mode) && mode_get(sx, (size_t)mode + 1) == mode_character;
}

int mode_is_pointer(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_pointer;
}

int mode_is_struct(syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_struct;
}

int mode_is_float(const item_t mode)
{
	return mode == mode_float;
}

int mode_is_int(const item_t mode)
{
	return mode == mode_integer || mode == mode_character;
}

int mode_is_void(const item_t mode)
{
	return mode == mode_void;
}

int mode_is_undefined(const item_t mode)
{
	return mode == mode_undefined;
}


size_t to_identab(parser *const prs, const size_t repr, const item_t type, const item_t mode)
{
	const size_t ret = ident_add(prs->sx, repr, type, mode, prs->func_def);
	prs->lastid = 0;

	if (ret == SIZE_MAX)
	{
		parser_error(prs, redefinition_of_main);
	}
	else if (ret == SIZE_MAX - 1)
	{
		char buffer[MAXSTRINGL];
		repr_get_ident(prs->sx, repr, buffer);
		parser_error(prs, repeated_decl, buffer);
	}
	else
	{
		prs->lastid = (int)ret;
	}

	return ret;
}

item_t to_modetab(parser *const prs, const item_t mode, const item_t element)
{
	item_t temp[2];
	temp[0] = mode;
	temp[1] = element;
	return (item_t)mode_add(prs->sx, temp, 2);
}
