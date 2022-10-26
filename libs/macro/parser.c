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
static const size_t MAX_PATH_SIZE = 1024;


static bool parse_character(parser *const prs, char32_t character, const bool was_slash);
static char32_t parse_until(parser *const prs);


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


/**
 *	Skip single line comment after double slash read.
 *	All line breaks will be replaced by empty lines.
 *	Exit before @c '\n' without backslash or @c EOF character read.
 *
 *	@param prs			Parser structure
 */
static void skip_comment(parser *const prs)
{
	bool was_slash = false;
	char32_t character = uni_scan_char(prs->io);

	while (character != (char32_t)EOF && (was_slash || character != '\n'))
	{
		if (character == '\n')
		{
			loc_line_break(&prs->loc);
			uni_print_char(prs->io, character);
		}

		was_slash = character == '\\' || (was_slash && character == '\r');
		character = uni_scan_char(prs->io);
	}

	uni_unscan_char(prs->io, character);
}

/**
 *	Skip multi line comment after slash and star sequence.
 *	If it haven't line break comment will be saved.
 *	Otherwise it will be removed with @c #line mark generation.
 *
 *	@param prs			Parser structure
 */
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

static char32_t skip_string(parser *const prs, const char32_t quote)
{
	uni_unscan_char(prs->io, quote);
	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_scan_char(prs->io);

	char32_t character = uni_scan_char(prs->io);
	bool was_slash = false;

	while (was_slash || character != quote)
	{
		character = character == '\r' ? uni_scan_char(prs->io) : character;
		if (character == '\n')
		{
			loc_line_break(&prs->loc);
		}

		if (character == (char32_t)EOF || (!was_slash && character == '\n'))
		{
			parser_error(prs, &loc, STRING_UNTERMINATED, quote);
			break;
		}

		uni_print_char(prs->io, character);
		was_slash = !was_slash && character == '\\';
		character = uni_scan_char(prs->io);
	}

	return character;
}

static void skip_directive(parser *const prs)
{
	const bool is_recovery_disabled = prs->is_recovery_disabled;
	const bool was_error = prs->was_error;
	prs->is_recovery_disabled = true;
	prs->was_error = true;

	universal_io out = io_create();
	out_swap(prs->io, &out);
	parse_until(prs);
	out_swap(prs->io, &out);

	prs->was_error = was_error;
	prs->is_recovery_disabled = is_recovery_disabled;

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
					character = uni_scan_char(prs->io);
					break;
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
				uni_print_char(prs->io, character);
				character = uni_scan_char(prs->io);
				break;

			case '\r':
				character = uni_scan_char(prs->io);
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
		case '#':
			uni_unscan_char(prs->io, character);
			loc_search_from(&prs->loc);
			parser_error(prs, &prs->loc, HASH_STRAY);
			uni_print_char(prs->io, uni_scan_char(prs->io));
			return false;

		case '/':
			return true;

		case '\'':
			uni_print_char(prs->io, character);
			uni_print_char(prs->io, skip_string(prs, character));
			return false;
		case '"':
			uni_print_char(prs->io, character);
			uni_print_char(prs->io, skip_string(prs, character));
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

static char32_t parse_until(parser *const prs)
{
	char32_t character = uni_scan_char(prs->io);
	bool was_slash = parse_character(prs, character, false);
	bool was_backslash = false;

	while (was_backslash || (character != '\r' && character != '\n' && character != (char32_t)EOF))
	{
		was_backslash = character == '\\';
		character = uni_scan_char(prs->io);
		was_slash = parse_character(prs, character, was_slash);
	}

	return character;
}

static size_t parse_directive(parser *const prs)
{
	char32_t character = skip_until(prs, uni_scan_char(prs->io));
	uni_unscan_char(prs->io, character);
	if (character != '#')
	{
		return SIZE_MAX;
	}

	loc_search_from(&prs->loc);
	location loc = prs->loc;
	universal_io out = io_create();
	out_set_buffer(&out, MAX_COMMENT_SIZE);
	uni_print_char(&out, character);

	size_t keyword = storage_search(prs->stg, prs->io, &character);
	if (storage_last_read(prs->stg)[1] == '\0')
	{
		out_swap(prs->io, &out);
		character = skip_until(prs, character);
		out_swap(prs->io, &out);

		if (utf8_is_letter(character))
		{
			uni_unscan_char(prs->io, character);
			loc_search_from(&prs->loc);
			storage_search(prs->stg, prs->io, &character);

			universal_io directive = io_create();
			out_set_buffer(&directive, MAX_KEYWORD_SIZE);
			uni_printf(&directive, "#%s", storage_last_read(prs->stg));

			char *buffer = out_extract_buffer(&directive);
			keyword = storage_get_index(prs->stg, buffer);
			free(buffer);
		}
	}
	uni_unscan_char(prs->io, character);

	if (!kw_is_correct(keyword))
	{
		char *buffer = out_extract_buffer(&out);
		uni_printf(prs->io, "%s", buffer);

		const char *directive = storage_last_read(prs->stg);
		if (utf8_is_letter(utf8_convert(&directive[1])))
		{
			parser_error(prs, buffer[1] == '\0' ? &loc : &prs->loc, DIRECTIVE_INVALID, directive);
			uni_printf(prs->io, "%s", &directive[1]);
		}
		else
		{
			parser_error(prs, &loc, HASH_STRAY);
			uni_unscan(prs->io, &directive[1]);
		}

		free(buffer);
		return SIZE_MAX;
	}

	out_clear(&out);
	return keyword;
}


static location parse_location(parser *const prs)
{
	size_t position = in_get_position(prs->io);
	uni_unscan(prs->io, storage_last_read(prs->stg));
	if (uni_scan_char(prs->io) == '#')
	{
		uni_unscan_char(prs->io, '#');
	}

	loc_search_from(&prs->loc);
	in_set_position(prs->io, position);
	return prs->loc;
}

static void parse_line(parser *const prs)
{
	parse_location(prs);
	macro_warning(&prs->loc, DIRECTIVE_LINE_SKIPED);
	skip_directive(prs);
}

static void parse_include_path(parser *const prs, const char32_t quote)
{
	uni_unscan_char(prs->io, quote);
	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_scan_char(prs->io);

	universal_io out = io_create();
	out_set_buffer(&out, MAX_PATH_SIZE);
	out_swap(prs->io, &out);

	char32_t character = skip_string(prs, quote);
	if (character != quote)
	{
		parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
		out_swap(prs->io, &out);
		out_clear(&out);
		return;
	}

	char *path = out_extract_buffer(prs->io);
	size_t index = quote == '"'
		? linker_search_internal(prs->lk, path)
		: linker_search_external(prs->lk, path);
	free(path);

	if (index == SIZE_MAX)
	{
		parser_error(prs, &loc, INCLUDE_NO_SUCH_FILE);
		skip_directive(prs);
		out_swap(prs->io, &out);
		return;
	}

	character = skip_until(prs, uni_scan_char(prs->io));
	if (character != '\n')
	{
		uni_unscan_char(prs->io, character);
		loc_search_from(&prs->loc);
		macro_warning(&prs->loc, DIRECTIVE_EXTRA_TOKENS, storage_last_read(prs->stg));
		skip_directive(prs);
	}
	else
	{
		loc_line_break(&prs->loc);
	}

	out_swap(prs->io, &out);
	universal_io header = linker_add_header(prs->lk, index);

	loc = prs->loc;
	parser_preprocess(prs, &header);
	prs->loc = loc;

	in_clear(&header);
}

static void parse_include(parser *const prs)
{
	location loc = parse_location(prs);
	universal_io out = io_create();
	out_swap(prs->io, &out);

	char32_t character = skip_until(prs, uni_scan_char(prs->io));
	switch (character)
	{
		case '<':
			character = '>';
		case '"':
			out_swap(prs->io, &out);
			parse_include_path(prs, character);
			break;

		case '\n':
			parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			loc_line_break(&prs->loc);
			out_swap(prs->io, &out);
			break;
		default:
			uni_unscan_char(prs->io, character);
			loc_search_from(&prs->loc);
			parser_error(prs, &prs->loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			skip_directive(prs);
			out_swap(prs->io, &out);
			break;
	}

	uni_print_char(prs->io, '\n');
	loc_update(&prs->loc);
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
	if (!parser_is_correct(prs) || !in_is_correct(in))
	{
		return -1;
	}

	in_swap(prs->io, in);
	prs->loc = loc_create(prs->io);

	char32_t character = '\0';
	while (character != (char32_t)EOF)
	{
		const size_t keyword = parse_directive(prs);
		switch (keyword)
		{
			case KW_LINE:
				parse_line(prs);
				continue;
			case KW_INCLUDE:
				parse_include(prs);
				continue;

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

			default:
				break;
		}

		character = parse_until(prs);
	}

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
