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


#define MAX_INDEX_SIZE 21

#define MASK_POSTFIX		"__"
#define MASK_ARGUMENT		"__ARG_%zu_"
#define MASK_STRING			"__STR_%zu_"
// #define MASK_CHARACTER		"__CHR_%zu_"
#define MASK_TOKEN_PASTE	"#__TKP_%zu_"


static const size_t MAX_COMMENT_SIZE = 4096;
static const size_t MAX_VALUE_SIZE = 4096;
static const size_t MAX_PATH_SIZE = 1024;


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
 *	@param	prs			Parser structure
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
 *	If it haven't line break then the comment will be saved.
 *	Otherwise it will be removed with @c #line mark generation.
 *
 *	@param	prs			Parser structure
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
	size_t begin = 0;

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
			uni_print_char(prs->io, character);
			begin = in_get_position(prs->io);
		}

		was_star = character == '*';
		character = uni_scan_char(prs->io);
	}

	const size_t end = in_get_position(prs->io);
	in_set_position(prs->io, begin);
	while (in_get_position(prs->io) != end)
	{
		uni_print_char(prs->io, uni_scan_char(prs->io) == '\t' ? '\t' : ' ');
	}
}

/**
 *	Write string content to output after quote.
 *	Stop when read the closing quote, without printing.
 *	Also stopped after @c '\n' read without backslash.
 *
 *	@param	prs			Parser structure
 *	@param	quote		Expected closing quote
 *
 *	@return	Closing quote or other last character read
 */
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

/**
 *	Skip the current directive processing until next line.
 *	All backslash line breaks and multiline comments will be skipped too.
 *
 *	@param	prs			Parser structure
 */
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
	prs->is_line_required = true;
}

/**
 *	Skip all comments, space and tab characters until first significant character.
 *	Line break is also a significant character.
 *	Stopped without last character read and processed.
 *
 *	@param	prs			Parser structure
 *	@param	fill		Set to produce output
 *
 *	@return	First significant character
 */
static char32_t skip_until(parser *const prs, const bool fill)
{
	universal_io out = io_create();
	out_swap(prs->io, fill ? NULL : &out);

	while (true)
	{
		char32_t character = uni_scan_char(prs->io);
		switch (character)
		{
			case '/':
				character = uni_scan_char(prs->io);
				if (character == '*')
				{
					skip_multi_comment(prs);
					continue;
				}
				else if (character == '/')
				{
					skip_comment(prs);
					continue;
				}
				else
				{
					uni_unscan_char(prs->io, character);
					character = '/';
					break;
				}

			case '\\':
				character = uni_scan_char(prs->io);
				character = character == '\r' ? uni_scan_char(prs->io) : character;
				if (character == '\n')
				{
					uni_printf(prs->io, "\\\n");
					loc_line_break(&prs->loc);
					continue;
				}
				else
				{
					uni_unscan_char(prs->io, character);
					character = '\\';
					break;
				}

			case ' ':
			case '\t':
				uni_print_char(prs->io, character);
				continue;

			case '\r':
				character = uni_scan_char(prs->io);
				break;
		}

		uni_unscan_char(prs->io, character);
		out_swap(prs->io, fill ? NULL : &out);
		return character;
	}
}


static void parce_replace(parser *const prs)
{
	loc_search_from(&prs->loc);
	const size_t index = storage_search(prs->stg, prs->io);
	if (storage_last_read(prs->stg)[0] == '#'
		&& (index == SIZE_MAX || kw_is_correct(index) || in_is_file(prs->io)))
	{
		parser_error(prs, &prs->loc, CHARACTER_STRAY, '#');
		uni_printf(prs->io, "%s", storage_last_read(prs->stg));
		return;
	}

	if (index == SIZE_MAX)
	{
		uni_printf(prs->io, "%s", storage_last_read(prs->stg));
		return;
	}
}

static char32_t parse_until(parser *const prs)
{
	char32_t character = '\0';
	while (character != '\n' && character != (char32_t)EOF)
	{
		character = skip_until(prs, true);

		if ((character == '#' || utf8_is_letter(character)) && out_is_correct(prs->io))
		{
			parce_replace(prs);
		}
		else if (character == '\'' || character == '"')
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
			uni_print_char(prs->io, skip_string(prs, character));
		}
		else
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
		}
	}

	loc_line_break(&prs->loc);
	return character;
}

static char32_t parse_hash(parser *const prs, universal_io *const out)
{
	out_swap(prs->io, out);
	while (true) {
		out_set_buffer(prs->io, MAX_COMMENT_SIZE);
		if (prs->is_line_required)
		{
			uni_print_char(prs->io, '\n');
			loc_update(&prs->loc);
		}

		char32_t character = skip_until(prs, true);
		if (character != '\n')
		{
			out_swap(prs->io, out);
			if (character != '#')
			{
				char *buffer = out_extract_buffer(out);
				uni_printf(prs->io, "%s", buffer);
				free(buffer);
			}

			prs->is_line_required = false;
			return character;
		}

		uni_scan_char(prs->io);
		loc_line_break(&prs->loc);
	}
}

static size_t parse_directive(parser *const prs)
{
	universal_io out = io_create();
	if (parse_hash(prs, &out) != '#')
	{
		return SIZE_MAX;
	}

	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_print_char(&out, '#');

	size_t keyword = storage_search(prs->stg, prs->io);
	if (storage_last_read(prs->stg)[1] == '\0')
	{
		out_swap(prs->io, &out);
		if (utf8_is_letter(skip_until(prs, true)))
		{
			loc_search_from(&prs->loc);
			loc = prs->loc;
			storage_search(prs->stg, prs->io);

			universal_io directive = io_create();
			out_set_buffer(&directive, MAX_KEYWORD_SIZE);
			uni_printf(&directive, "#%s", storage_last_read(prs->stg));

			char *buffer = out_extract_buffer(&directive);
			keyword = storage_get_index(prs->stg, buffer);
			free(buffer);
		}
		out_swap(prs->io, &out);
	}

	if (!kw_is_correct(keyword))
	{
		char *buffer = out_extract_buffer(&out);
		uni_printf(prs->io, "%s", buffer);

		const char *directive = storage_last_read(prs->stg);
		if (utf8_is_letter(utf8_convert(&directive[1])))
		{
			parser_error(prs, &loc, DIRECTIVE_INVALID, directive);
			uni_printf(prs->io, "%s", &directive[1]);
		}
		else
		{
			parser_error(prs, &loc, CHARACTER_STRAY, '#');
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
	const size_t position = in_get_position(prs->io);
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
	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_scan_char(prs->io);

	universal_io out = io_create();
	out_set_buffer(&out, MAX_PATH_SIZE);
	out_swap(prs->io, &out);
	char32_t character = skip_string(prs, quote);
	out_swap(prs->io, &out);

	if (character != quote)
	{
		parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
		out_clear(&out);
		return;
	}

	char *path = out_extract_buffer(&out);
	size_t index = quote == '"'
		? linker_search_internal(prs->lk, path)
		: linker_search_external(prs->lk, path);
	free(path);

	if (index == SIZE_MAX)
	{
		parser_error(prs, &loc, INCLUDE_NO_SUCH_FILE);
		return;
	}

	if (skip_until(prs, false) != '\n')
	{
		loc_search_from(&prs->loc);
		macro_warning(&prs->loc, DIRECTIVE_EXTRA_TOKENS, storage_last_read(prs->stg));
	}

	loc = prs->loc;
	universal_io header = linker_add_header(prs->lk, index);
	parser_preprocess(prs, &header);
	in_clear(&header);
	prs->loc = loc;
}

static void parse_include(parser *const prs)
{
	location loc = parse_location(prs);
	char32_t character = skip_until(prs, false);
	switch (character)
	{
		case '<':
			character = '>';
		case '"':
			parse_include_path(prs, character);
			break;

		case '\n':
			parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
		default:
			loc_search_from(&prs->loc);
			parser_error(prs, &prs->loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
	}

	skip_directive(prs);
}


static bool parse_name(parser *const prs)
{
	location loc = parse_location(prs);
	char32_t character = skip_until(prs, false);
	if (utf8_is_letter(character))
	{
		return true;
	}

	if (character == '\n')
	{
		parser_error(prs, &loc, DIRECTIVE_NAME_NON, storage_last_read(prs->stg));
	}
	else
	{
		loc_search_from(&prs->loc);
		parser_error(prs, &prs->loc, MACRO_NAME_FIRST_CHARACTER);
	}

	return false;
}

static size_t parse_args(parser *const prs)
{
	const size_t position = in_get_position(prs->io);
	if (skip_until(prs, false) != '(' || position != in_get_position(prs->io))
	{
		return 0;
	}

	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_scan_char(prs->io);
	char32_t character = skip_until(prs, false);

	for (size_t i = 0; ; i++)
	{
		if (character == ')')
		{
			uni_scan_char(prs->io);
			return i;
		}
		else if (character == '\n' || character == (char32_t)EOF)
		{
			parser_error(prs, &loc, ARGS_EXPECTED_BRACKET);
			break;
		}

		loc_search_from(&prs->loc);
		if (!utf8_is_letter(character))
		{
			parser_error(prs, &prs->loc, ARGS_EXPECTED_NAME, character, prs->io);
			break;
		}

		const size_t index = storage_add_by_io(prs->stg, prs->io);
		if (index == SIZE_MAX)
		{
			parser_error(prs, &prs->loc, ARGS_DUPLICATE, storage_last_read(prs->stg));
			break;
		}

		char buffer[MAX_INDEX_SIZE];
		sprintf(buffer, "%zu", i);
		storage_set_by_index(prs->stg, index, buffer);

		character = skip_until(prs, false);
		if (character == ',')
		{
			uni_scan_char(prs->io);
			character = skip_until(prs, false);
		}
		else if (character != ')' && character != '\n' && character != (char32_t)EOF)
		{
			loc_search_from(&prs->loc);
			parser_error(prs, &prs->loc, ARGS_EXPECTED_COMMA, character, prs->io);
			break;
		}
	}

	return SIZE_MAX;
}

static bool parse_operator(parser *const prs, const size_t index, const bool was_space)
{
	loc_search_from(&prs->loc);
	location loc = prs->loc;
	uni_scan_char(prs->io);

	char32_t character = uni_scan_char(prs->io);
	if (character == '#')
	{
		character = skip_until(prs, false);
		if (character == '\n')
		{
			parser_error(prs, &loc, HASH_ON_EDGE);
			return false;
		}

		const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
		if (value == NULL)
		{
			parser_error(prs, &loc, HASH_NOT_FOLLOWED, "##");
			return false;
		}

		uni_printf(prs->io, MASK_TOKEN_PASTE "%s" MASK_POSTFIX, index, value);
		return true;
	}

	uni_unscan_char(prs->io, character);
	character = skip_until(prs, false);

	const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
	if (value == NULL)
	{
		parser_error(prs, &loc, HASH_NOT_FOLLOWED, "#");
		return false;
	}

	uni_printf(prs->io, "%s" MASK_STRING "%s" MASK_POSTFIX, was_space ? " " : "", index, value);
	return true;
}

static char *parse_content(parser *const prs, const size_t index)
{
	universal_io out = io_create();
	out_set_buffer(&out, MAX_VALUE_SIZE);
	out_swap(prs->io, &out);

	char32_t character = skip_until(prs, false);
	size_t position = in_get_position(prs->io);

	while (character != '\n' && character != (char32_t)EOF)
	{
		if (character == '#')
		{
			if (!parse_operator(prs, index, in_get_position(prs->io) != position))
			{
				out_swap(prs->io, &out);
				out_clear(&out);
				return NULL;
			}
		}
		else
		{
			uni_print_char(prs->io, in_get_position(prs->io) == position ? (char32_t)EOF : ' ');
			if (utf8_is_letter(character))
			{
				const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
				if (value != NULL)
				{
					uni_printf(prs->io, MASK_ARGUMENT "%s" MASK_POSTFIX, index, value);
				}
				else
				{
					uni_printf(prs->io, "%s", storage_last_read(prs->stg));
				}
			}
			else if (character == '\'' || character == '"')
			{
				uni_print_char(prs->io, uni_scan_char(prs->io));
				if (skip_string(prs, character) != character)
				{
					out_swap(prs->io, &out);
					out_clear(&out);
					return NULL;
				}
				uni_print_char(prs->io, character);
			}
			else
			{
				uni_print_char(prs->io, uni_scan_char(prs->io));
			}
		}

		position = in_get_position(prs->io);
		character = skip_until(prs, false);
	}

	out_swap(prs->io, &out);
	return out_extract_buffer(&out);
}

static void parse_context(parser *const prs, const size_t index)
{
	storage stg = storage_create();
	storage *origin = prs->stg;
	prs->stg = &stg;

	const size_t args = parse_args(prs);
	if (args != SIZE_MAX)
	{
		storage_set_args_by_index(origin, index, args);

		if (skip_until(prs, false) == '#')
		{
			loc_search_from(&prs->loc);
			uni_scan_char(prs->io);
			char32_t character = uni_scan_char(prs->io);
			if (character == '#')
			{
				parser_error(prs, &prs->loc, HASH_ON_EDGE);
				storage_remove_by_index(origin, index);
				storage_clear(&stg);
				prs->stg = origin;
				return;
			}
			else
			{
				uni_unscan_char(prs->io, character);
				uni_unscan_char(prs->io, '#');
			}
		}

		char *value = parse_content(prs, index);
		if (value != NULL)
		{
			storage_set_by_index(origin, index, value);
			free(value);

			storage_clear(&stg);
			prs->stg = origin;
			return;
		}
	}

	storage_remove_by_index(origin, index);
	storage_clear(&stg);
	prs->stg = origin;
}

static void parse_define(parser *const prs)
{
	if (parse_name(prs))
	{
		loc_search_from(&prs->loc);
		const size_t index = storage_add_by_io(prs->stg, prs->io);
		if (index == SIZE_MAX)
		{
			parser_error(prs, &prs->loc, MACRO_NAME_REDEFINE, storage_last_read(prs->stg));
		}
		else
		{
			parse_context(prs, index);
		}
	}

	skip_directive(prs);
}

static void parse_set(parser *const prs)
{
	if (parse_name(prs))
	{
		loc_search_from(&prs->loc);
		const size_t position = in_get_position(prs->io);
		size_t index = storage_search(prs->stg, prs->io);

		if (index == SIZE_MAX)
		{
			macro_warning(&prs->loc, MACRO_NAME_UNDEFINED, storage_last_read(prs->stg));
			in_set_position(prs->io, position);
			index = storage_add_by_io(prs->stg, prs->io);
		}

		parse_context(prs, index);
	}

	skip_directive(prs);
}

static void parse_undef(parser *const prs)
{
	if (parse_name(prs))
	{
		storage_remove_by_index(prs->stg, storage_search(prs->stg, prs->io));
	}

	skip_directive(prs);
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
	prs.is_line_required = false;
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
				parse_define(prs);
				continue;
			case KW_SET:
				parse_set(prs);
				continue;
			case KW_UNDEF:
				parse_undef(prs);
				continue;

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
