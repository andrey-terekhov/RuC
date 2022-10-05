/*
 *	Copyright 2022 Andrey Terekhov, Victor Y. Fadeev
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
#include "error.h"
#include "keywords.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include "utf8.h"


/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_error(parser *const prs, const error_t num, ...)
{
	if (prs->is_recovery_disabled && prs->was_error)
	{
		return;
	}

	va_list args;
	va_start(args, num);

	loc_search_from(&prs->loc);
	macro_verror(&prs->loc, num, args);
	prs->was_error = true;

	va_end(args);
}


static void skip_string(parser *const prs, const char32_t quote)
{
	const size_t position = in_get_position(prs->io) - utf8_size(quote);
	uni_print_char(prs->io, quote);

	size_t lines = 0;
	bool was_slash = false;
	char32_t character = '\0';
	do
	{
		was_slash = !was_slash && character == '\\';
		character = uni_scan_char(prs->io);
		uni_print_char(prs->io, character);

		lines += character == '\n' ? 1 : 0;

		if (character == (char32_t)EOF || (!was_slash && character == '\n'))
		{
			const size_t end = in_get_position(prs->io);
			in_set_position(prs->io, position);
			parser_error(prs, PARSER_MISSING_TERMINATION, quote);
			in_set_position(prs->io, end);
			break;
		}
	} while (was_slash || character != quote);

	for (size_t i = 0; i < lines; i++)
	{
		loc_line_break(&prs->loc);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


parser parser_create(linker *const lk, storage *const stg, universal_io *const out)
{
	parser prs;
	if (!linker_is_correct(lk) || !storage_is_correct(stg) || !out_is_correct(out))
	{
		prs.lk = NULL;
		return prs;
	}

	prs.lk = lk;
	prs.stg = stg;
	prs.io = out;

	prs.is_recovery_disabled = false;
	prs.is_if_block = false;
	prs.was_error = false;

	return prs;
}


int parser_preprocess(parser *const prs, universal_io *const in)
{
	if (!parser_is_correct(prs)|| !in_is_correct(in))
	{
		return -1;
	}

	in_swap(prs->io, in);
	prs->loc = loc_create(prs->io);

	bool was_slash = false;
	char32_t character = '\0';
	do
	{
		switch (storage_search(prs->stg, prs->io, &character))
		{
			case KW_INCLUDE:

			case KW_DEFINE:
			case KW_SET:
			case KW_UNDEF:

			case KW_EVAL:

			case KW_IFDEF:
			case KW_IFNDEF:
			case KW_IF:

			case KW_MACRO:
			case KW_WHILE:
				break;

			case KW_ELIF:
			case KW_ELSE:
			case KW_ENDIF:

			case KW_ENDM:
			case KW_ENDW:
				/* error */
				break;

			case SIZE_MAX:
			{
				const char *last = storage_last_read(prs->stg);
				if (last != NULL)
				{
					if (was_slash)
					{
						uni_print_char(prs->io, '/');
						was_slash = false;
					}
					uni_printf(prs->io, "%s", last);
				}
			}
			break;

			default:
				break;
		}

		switch (character)
		{
			case '\'':
				skip_string(prs, character);
				break;
			case '"':
				skip_string(prs, character);
				break;
			case '/':
				if (was_slash)
				{
					// skip_comment(prs);
				}
				was_slash = !was_slash;
				break;
			case '*':
				if (was_slash)
				{
					// skip_multi_comment(prs);
					was_slash = false;
				}
				else
				{
					uni_print_char(prs->io, character);
				}
				break;

			case '\n':
				loc_line_break(&prs->loc);
			default:
				if (was_slash)
				{
					uni_print_char(prs->io, '/');
					was_slash = false;
				}
				uni_print_char(prs->io, character);
				break;
		}
	} while (character != (char32_t)EOF);

	in_swap(prs->io, in);
	return 0;
}


int parser_disable_recovery(parser *const prs, const bool status)
{
	if (!parser_is_correct(prs))
	{
		return -1;
	}

	prs->is_recovery_disabled = status;
	return 0;
}

bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && storage_is_correct(prs->stg) && out_is_correct(prs->io);
}


int parser_clear(parser *const prs)
{
	(void)prs;
	return 0;
}
