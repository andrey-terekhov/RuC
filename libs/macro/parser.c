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
#include <stdlib.h>
#include "error.h"
#include "keywords.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include "utf8.h"


static const size_t MAX_COMMENT_SIZE = 4096;


static bool parse_character(parser *const prs, char32_t character, const bool was_slash);


/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_error(parser *const prs, location *const loc, error_t num, ...)
{
	if (prs->is_recovery_disabled && prs->was_error)
	{
		return;
	}

	va_list args;
	va_start(args, num);

	macro_verror(loc, num, args);
	prs->was_error = true;

	va_end(args);
}


static void skip_string(parser *const prs, const char32_t quote)
{
	uni_unscan_char(prs->io, quote);
	loc_search_from(&prs->loc);
	location loc = prs->loc;

	uni_print_char(prs->io, uni_scan_char(prs->io));
	char32_t character = '\0';
	bool was_slash = false;

	do
	{
		was_slash = !was_slash && character == '\\';
		character = uni_scan_char(prs->io);
		character = character == '\r' ? uni_scan_char(prs->io) : character;

		if (character == '\n')
		{
			loc_line_break(&prs->loc);
		}

		uni_print_char(prs->io, character);
		if (character == (char32_t)EOF || (!was_slash && character == '\n'))
		{
			parser_error(prs, &loc, STRING_UNTERMINATED, quote);
			break;
		}
	} while (was_slash || character != quote);
}

static void skip_comment(parser *const prs)
{
	char32_t character = '\0';
	bool was_slash = false;

	do
	{
		was_slash = character == '\\' || (was_slash && character == '\r');
		character = uni_scan_char(prs->io);

		if (character == '\n')
		{
			loc_line_break(&prs->loc);
			uni_print_char(prs->io, character);
		}
	} while (character != (char32_t)EOF && (was_slash || character != '\n'));
}

static void skip_multi_comment(parser *const prs)
{
	uni_unscan_char(prs->io, '*');
	uni_unscan_char(prs->io, '/');
	loc_search_from(&prs->loc);
	location loc = prs->loc;

	universal_io out = io_create();
	out_set_buffer(&out, MAX_COMMENT_SIZE);
	uni_print_char(&out, uni_scan_char(prs->io));
	uni_print_char(&out, uni_scan_char(prs->io));
	char32_t character = '\0';
	bool was_star = false;

	do
	{
		was_star = character == '*';
		character = uni_scan_char(prs->io);
		uni_print_char(&out, character);

		if (was_star && character == '/')
		{
			char *buffer = out_extract_buffer(&out);
			uni_printf(prs->io, "%s", buffer);
			free(buffer);
			return;
		}
	} while (character != '\r' && character != '\n' && character != (char32_t)EOF);

	out_clear(&out);

	while (!was_star || character != '/')
	{
		if (character == (char32_t)EOF)
		{
			parser_error(prs, &loc, COMMENT_UNTERMINATED);
			return;
		}
		else if (character == '\n')
		{
			loc_line_break(&prs->loc);
		}

		was_star = character == '*';
		character = uni_scan_char(prs->io);
	}

	uni_print_char(prs->io, '\n');
	loc_update(&prs->loc);
}

static void skip_directive(parser *const prs, char32_t character)
{
	universal_io out = io_create();
	out_swap(prs->io, &out);

	bool was_slash = parse_character(prs, character, false);
	bool was_backslash = false;

	while (was_backslash || (character != '\r' && character != '\n' && character != (char32_t)EOF))
	{
		was_backslash = character == '\\';
		character = uni_scan_char(prs->io);
		if (was_slash && character == '/')
		{
			skip_comment(prs);
			break;
		}

		was_slash = parse_character(prs, character, was_slash);
	}

	out_swap(prs->io, &out);
	uni_print_char(prs->io, '\n');
	loc_update(&prs->loc);
}

static char32_t skip_until(parser *const prs, char32_t character)
{
	while (true)
	{
		switch (character)
		{
			case '/':
				character = uni_scan_char(prs->io);
				if (character == '*')
				{
					skip_multi_comment(prs);
					character = uni_scan_char(prs->io);
					break;
				}
				else if (character == '/')
				{
					skip_comment(prs);
					return '\n';
				}
				else
				{
					uni_unscan_char(prs->io, character);
					return '/';
				}

			case '\\':
				character = uni_scan_char(prs->io);
				character = character == '\r' ? uni_scan_char(prs->io) : character;
				if (character == '\n')
				{
					loc_line_break(&prs->loc);
					character = uni_scan_char(prs->io);
					break;
				}
				else
				{
					uni_unscan_char(prs->io, character);
					return '\\';
				}

			case ' ':
			case '\t':
				character = uni_scan_char(prs->io);
				break;

			case '\r':
				character = uni_scan_char(prs->io);
			case '\n':
				loc_line_break(&prs->loc);
			default:
				return character;
		}
	}
}


static bool parse_character(parser *const prs, char32_t character, const bool was_slash)
{
	if (was_slash && character == '/')
	{
		skip_comment(prs);
		return false;
	}
	else if (was_slash && character == '*')
	{
		skip_multi_comment(prs);
		return false;
	}
	else if (was_slash)
	{
		uni_print_char(prs->io, '/');
	}

	switch (character)
	{
		case '/':
			return true;

		case '\'':
			skip_string(prs, character);
			return false;
		case '"':
			skip_string(prs, character);
			return false;

		case '\r':
			character = uni_scan_char(prs->io);
		case '\n':
			loc_line_break(&prs->loc);
		default:
			uni_print_char(prs->io, character);
			return false;
	}
}

static void parse_line(parser *const prs, char32_t character)
{
	uni_unscan_char(prs->io, character);
	uni_unscan(prs->io, storage_last_read(prs->stg));
	loc_search_from(&prs->loc);
	macro_warning(&prs->loc, DIRECTIVE_LINE_SKIPED);

	skip_directive(prs, uni_scan_char(prs->io));
}

static void parse_include(parser *const prs, char32_t character)
{
	uni_unscan_char(prs->io, character);
	uni_unscan(prs->io, storage_last_read(prs->stg));
	loc_search_from(&prs->loc);
	location loc = prs->loc;

	storage_search(prs->stg, prs->io, &character);
	universal_io out = io_create();
	out_swap(prs->io, &out);

	bool was_slash = false;
	while (character != '\n' && character != (char32_t)EOF)
	{
		if (was_slash && character == '*')
		{
			skip_multi_comment(prs);
			character = uni_scan_char(prs->io);
			was_slash = false;
			continue;
		}
		else if (was_slash && character == '/')
		{
			skip_comment(prs);
			parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
		}
		else if (was_slash)
		{
			uni_unscan_char(prs->io, character);
			uni_unscan_char(prs->io, '/');
			loc_search_from(&prs->loc);
			parser_error(prs, &prs->loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
		}

		switch (character)
		{
			case '/':
				was_slash = true;
				break;
			case '\\':
				character = uni_scan_char(prs->io);
				character = character == '\r' ? uni_scan_char(prs->io) : character;
				if (character != '\n')
				{
					uni_unscan_char(prs->io, character);
					uni_unscan_char(prs->io, '\\');
					loc_search_from(&prs->loc);
					parser_error(prs, &prs->loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
					break;
				}


		}
	}

	out_swap(prs->io, &out);
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
		const size_t keyword = storage_search(prs->stg, prs->io, &character);
		const char *last = storage_last_read(prs->stg);
		if (was_slash && last != NULL)
		{
			uni_print_char(prs->io, '/');
			was_slash = false;
		}

		switch (keyword)
		{
			case KW_LINE:
				parse_line(prs, character);
				continue;

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
				if (last != NULL)
				{
					uni_printf(prs->io, "%s", last);
				}
				break;

			default:
				break;
		}

		was_slash = parse_character(prs, character, was_slash);
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
