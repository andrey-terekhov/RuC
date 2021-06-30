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
#include "codes.h"
#include "tree.h"


static const char *const DEFAULT_TREE = "tree.txt";

static const size_t MAX_LABELS = 10000;


/** Check if the set of tokens has token in it */
static inline int token_check(const uint8_t tokens, const token_t token)
{
	return (tokens & token) != 0;
}

/**
 *	Create parser structure
 *
 *	@param	sx		Syntax structure
 *	@param	lxr		Lexer structure
 *
 *	@return	Parser structure
 */
static inline parser parser_create(syntax *const sx, lexer *const lxr)
{
	parser prs;
	prs.sx = sx;
	prs.lxr = lxr;

	prs.was_error = false;

	prs.labels = vector_create(MAX_LABELS);
	token_consume(&prs);

	return prs;
}

static inline void parser_clear(parser *const prs)
{
	vector_clear(&prs->labels);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int parse(const workspace *const ws, universal_io *const io, syntax *const sx)
{
	if (!ws_is_correct(ws) || !in_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	lexer lxr = create_lexer(ws, io, sx);
	parser prs = parser_create(sx, &lxr);
	node root = node_get_root(&sx->tree);

	do
	{
		parse_declaration_external(&prs, &root);
	} while (prs.token != TK_EOF);

	parser_clear(&prs);

#ifndef NDEBUG
	tables_and_tree(DEFAULT_TREE, &sx->identifiers, &sx->types, &sx->tree);
#endif

	return prs.was_error || prs.lxr->was_error || !sx_is_correct(sx);
}


void parser_error(parser *const prs, error_t num, ...)
{
	if (prs->lxr->is_recovery_disabled && (prs->lxr->was_error || prs->was_error))
	{
		return;
	}

	va_list args;
	va_start(args, num);

	verror(prs->lxr->io, num, args);
	prs->was_error = true;

	va_end(args);
}

void semantics_error(parser *const prs, const location_t loc, error_t num, ...)
{
	if (prs->lxr->is_recovery_disabled && (prs->lxr->was_error || prs->was_error))
	{
		return;
	}

	va_list args;
	va_start(args, num);

	const size_t prev_loc = in_get_position(prs->lxr->io);
	in_set_position(prs->lxr->io, loc.begin);

	verror(prs->lxr->io, num, args);
	prs->was_error = true;

	va_end(args);

	in_set_position(prs->lxr->io, prev_loc);
}

location_t token_consume(parser *const prs)
{
	const size_t token_start = prs->lxr->location;
	prs->token = lex(prs->lxr);
	return (location_t){ token_start, in_get_position(prs->lxr->io) };
}

int token_try_consume(parser *const prs, const token_t expected)
{
	if (prs->token == expected)
	{
		token_consume(prs);
		return 1;
	}

	return 0;
}

void token_expect_and_consume(parser *const prs, const token_t expected, const error_t err)
{
	if (!token_try_consume(prs, expected))
	{
		parser_error(prs, err);
	}
}

void token_skip_until(parser *const prs, const uint8_t tokens)
{
	while (prs->token != TK_EOF)
	{
		switch (prs->token)
		{
			case TK_L_PAREN:
				token_consume(prs);
				token_skip_until(prs, TK_R_PAREN);
				break;

			case TK_L_SQUARE:
				token_consume(prs);
				token_skip_until(prs, TK_R_SQUARE);
				break;

			case TK_L_BRACE:
				token_consume(prs);
				token_skip_until(prs, TK_R_BRACE);
				break;

			case TK_QUESTION:
				token_consume(prs);
				token_skip_until(prs, TK_COLON);
				break;

			case TK_R_PAREN:
			case TK_R_SQUARE:
			case TK_R_BRACE:
			case TK_COLON:
			case TK_SEMICOLON:
				if (token_check(tokens, prs->token))
				{
					return;
				}
				else
				{
					token_consume(prs);
					break;
				}

			default:
				token_consume(prs);
				break;
		}
	}
}


size_t to_identab(parser *const prs, const size_t repr, const item_t type, const item_t mode)
{
	const size_t ret = ident_add(prs->sx, repr, type, mode, prs->func_def);

	if (ret == SIZE_MAX)
	{
		parser_error(prs, redefinition_of_main);
	}
	else if (ret == SIZE_MAX - 1)
	{
		parser_error(prs, repeated_decl, repr_get_name(prs->sx, repr));
	}

	return ret;
}
