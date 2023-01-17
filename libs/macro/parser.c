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
#define MAX_MASK_SIZE 51

#define MASK_SUFFIX			"_"
#define MASK_ARGUMENT		"__ARG_"
#define MASK_STRING			"__STR_"
// #define MASK_CHARACTER		"__CHR_"
#define MASK_TOKEN_PASTE	"#__TKP_"


static const size_t MAX_INCLUDE_DEPTH = 32;
static const size_t MAX_CALL_DAPTH = 256;
static const size_t MAX_ITERATION = 32768;

static const size_t MAX_COMMENT_SIZE = 4096;
static const size_t MAX_VALUE_SIZE = 4096;
static const size_t MAX_PATH_SIZE = 1024;


static char32_t parse_until(parser *const prs);
static keyword_t parse_directive(parser *const prs);
static location parse_location(parser *const prs);
static bool parse_next(parser *const prs, const keyword_t begin, const keyword_t next);
static bool parse_name(parser *const prs);
static char32_t parse_line(parser *const prs);


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

	macro_verror(in_is_file(prs->io) ? loc : prs->prev, num, args);
	prs->was_error = true;

	va_end(args);
}

/**
 *	Emit a warning from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Warning code
 */
static void parser_warning(parser *const prs, location *const loc, warning_t num, ...)
{
	if (prs->is_recovery_disabled)
	{
		return;
	}

	va_list args;
	va_start(args, num);

	macro_vwarning(in_is_file(prs->io) ? loc : prs->prev, num, args);

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
			loc_line_break(prs->loc);
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
	location loc = loc_copy(prs->loc);

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
			loc_line_break(prs->loc);
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
	location loc = loc_copy(prs->loc);
	uni_scan_char(prs->io);

	char32_t character = uni_scan_char(prs->io);
	bool was_slash = false;

	while (was_slash || character != quote)
	{
		character = character == '\r' ? uni_scan_char(prs->io) : character;
		if (character == '\n')
		{
			loc_line_break(prs->loc);
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
 *	Set @c #line directive requirement flag.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t skip_directive(parser *const prs)
{
	const bool is_recovery_disabled = prs->is_recovery_disabled;
	const bool was_error = prs->was_error;
	prs->is_recovery_disabled = true;
	prs->was_error = true;

	universal_io out = io_create();
	out_swap(prs->io, &out);
	char32_t character = parse_until(prs);
	out_swap(prs->io, &out);

	prs->was_error = was_error;
	prs->is_recovery_disabled = is_recovery_disabled;
	prs->is_line_required = true;
	return character;
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
					loc_line_break(prs->loc);
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

/**
 *	Skip lines without output until significant character.
 *	Works like @c skip_until but exclude line breaks.
 *	Stopped without last character read and processed.
 *
 *	@param	prs			Parser structure
 *
 *	@return	First significant character
 */
static char32_t skip_lines(parser *const prs)
{
	char32_t character = skip_until(prs, false);
	while (character == '\n')
	{
		uni_scan_char(prs->io);
		loc_line_break(prs->loc);
		character = skip_until(prs, false);
	}

	return character;
}

/**
 *	Skip @c #macro directive content line.
 *	Emit an error on no identifier or no expression.
 *	Produce a masked output for args.
 *	Erase output, if error occurred.
 *
 *	@param	prs			Parser structure
 *	@param	keyword		Recognized directive
 *
 *	@return	Last read character
 */
static char32_t skip_macro(parser *const prs, const keyword_t keyword)
{
	if (keyword == KW_LINE)
	{
		return parse_line(prs);
	}

	if (keyword == ERROR_KEYWORD)
	{
		out_clear(prs->io);
		return skip_directive(prs);
	}

	location loc;
	if (keyword != NON_KEYWORD)
	{
		loc = parse_location(prs);
		uni_printf(prs->io, "%s", storage_last_read(prs->stg));
	}

	size_t position = in_get_position(prs->io);
	char32_t character = skip_until(prs, false);
	if ((character == '\n' || character == (char32_t)EOF) && (keyword == KW_INCLUDE
		|| keyword == KW_IF || keyword == KW_ELIF || keyword == KW_WHILE || keyword == KW_EVAL))
	{
		out_clear(prs->io);
		parser_error(prs, &loc, keyword == KW_INCLUDE ? INCLUDE_EXPECTS_FILENAME
			: DIRECTIVE_NO_EXPRESSION, storage_last_read(prs->stg));
		return skip_directive(prs);
	}

	if ((keyword == KW_MACRO || keyword == KW_DEFINE || keyword == KW_SET || keyword == KW_UNDEF
		|| keyword == KW_IFDEF || keyword == KW_IFNDEF) && !parse_name(prs))
	{
		out_clear(prs->io);
		return skip_directive(prs);
	}

	while (character != '\n' && character != (char32_t)EOF)
	{
		uni_printf(prs->io, "%s", in_get_position(prs->io) != position ? " " : "");
		if (utf8_is_letter(character))
		{
			const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
			if (value != NULL)
			{
				uni_printf(prs->io, MASK_ARGUMENT "%s", value);
			}
			else
			{
				uni_printf(prs->io, "%s", storage_last_read(prs->stg));
			}
		}
		else if (character == '\'' || character == '"' || (character == '<' && keyword == KW_INCLUDE))
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
			character = character == '<' ? '>' : character;
			if (skip_string(prs, character) != character)
			{
				out_clear(prs->io);
				return skip_directive(prs);
			}
			uni_print_char(prs->io, character);
		}
		else if (character == '#' && keyword != KW_DEFINE && keyword != KW_SET
			&& keyword != KW_IF && keyword != KW_ELIF && keyword != KW_WHILE && keyword != KW_EVAL)
		{
			out_clear(prs->io);
			loc_search_from(prs->loc);
			parser_error(prs, prs->loc, HASH_STRAY);
			return skip_directive(prs);
		}
		else
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
		}

		position = in_get_position(prs->io);
		character = skip_until(prs, false);
	}

	return skip_directive(prs);
}

/**
 *	Skip multiline directive block.
 *	Produce a simplified masked output for @c #macro.
 *	Stop when the end / else token has reached.
 *	Erase output, if error occurred.
 *
 *	@param	prs			Parser structure
 *	@param	begin		Begin token
 *
 *	@return	End / else token, @c NON_KEYWORD on otherwise
 */
static keyword_t skip_block(parser *const prs, const keyword_t begin)
{
	universal_io out = io_create();
	const bool was_error = prs->was_error;
	const bool is_recovery_disabled = prs->is_recovery_disabled;
	const bool is_root_macro = begin == KW_MACRO && !prs->is_macro_processed;
	prs->is_macro_processed = is_root_macro ? true : prs->is_macro_processed;

	char32_t character = '\0';
	while (character != (char32_t)EOF)
	{
		if (!prs->is_macro_processed)
		{
			prs->is_recovery_disabled = true;
			prs->was_error = true;
		}

		out_swap(prs->io, &out);
		keyword_t keyword = parse_directive(prs);
		out_swap(prs->io, &out);

		if (!prs->is_macro_processed)
		{
			prs->is_recovery_disabled = is_recovery_disabled;
			prs->was_error = was_error;
		}

		if (keyword == KW_ELIF || keyword == KW_ELSE || keyword == KW_ENDIF
			|| keyword == KW_ENDW || keyword == KW_ENDM)
		{
			if (!parse_next(prs, begin, keyword))
			{
				out_clear(prs->io);
				character = skip_directive(prs);
				continue;
			}

			prs->is_macro_processed = is_root_macro ? false : prs->is_macro_processed;
			return keyword;
		}

		location loc;
		char directive[MAX_KEYWORD_SIZE];
		if (keyword != NON_KEYWORD)
		{
			loc = parse_location(prs);
			sprintf(directive, "%s", storage_last_read(prs->stg));
		}

		while (true)
		{
			uni_printf(prs->io, "%s", is_root_macro && character == '\0' ? "" : "\n");
			character = prs->is_macro_processed ? skip_macro(prs, keyword) : skip_directive(prs);

			if (keyword != KW_IFDEF && keyword != KW_IFNDEF && keyword != KW_IF && keyword != KW_ELIF
				&& keyword != KW_ELSE && keyword != KW_WHILE && keyword != KW_MACRO)
			{
				break;
			}

			keyword = skip_block(prs, keyword);
			if (keyword == NON_KEYWORD)
			{
				parser_error(prs, &loc, DIRECTIVE_UNTERMINATED, directive);
				break;
			}
		}
	}

	out_clear(prs->io);
	prs->is_macro_processed = is_root_macro ? false : prs->is_macro_processed;
	return NON_KEYWORD;
}


/**
 *	Parse read value of macro argument.
 *	Produce masked arguments for preprocessor operators.
 *	Creates preprocessed value, stringizing and token-pasting operators.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 *	@param	stg			Value storage
 *	@param	value		Read argument
 *	@param	arg			Argument number
 */
static void parse_values(parser *const prs, const size_t index, storage *const stg
	, char *const value, const size_t arg)
{
	char mask[MAX_MASK_SIZE];
	sprintf(mask, MASK_TOKEN_PASTE "%zu" MASK_SUFFIX "%zu", index, arg);
	storage_set_by_index(stg, storage_add(stg, mask), value);

	universal_io io = io_create();
	in_set_buffer(&io, value);
	out_set_buffer(&io, MAX_VALUE_SIZE);
	out_swap(prs->io, &io);
	parser_preprocess(prs, &io);
	out_swap(prs->io, &io);

	char *buffer = out_extract_buffer(&io);
	sprintf(mask, MASK_ARGUMENT "%zu" MASK_SUFFIX "%zu", index, arg);
	storage_set_by_index(stg, storage_add(stg, mask), buffer);
	free(buffer);

	in_set_position(&io, 0);
	out_set_buffer(&io, MAX_VALUE_SIZE);
	uni_print_char(&io, '"');
	for (char32_t ch = uni_scan_char(&io); ch != (char32_t)EOF; ch = uni_scan_char(&io))
	{
		if (ch == '\\')
		{
			ch = uni_scan_char(&io);
			uni_printf(&io, "%s", ch == '"' ? "\\\\" : ch != (char32_t)EOF ? "\\" : "");
		}

		uni_printf(&io, "%s", ch == '"' ? "\\" : "");
		uni_print_char(&io, ch);
	}
	uni_print_char(&io, '"');
	in_clear(&io);

	buffer = out_extract_buffer(&io);
	sprintf(mask, MASK_STRING "%zu" MASK_SUFFIX "%zu", index, arg);
	storage_set_by_index(stg, storage_add(stg, mask), buffer);
	free(buffer);
}

/**
 *	Parse macro argument values from brackets.
 *	Produce values for replacement to storage.
 *	Stopped without closing bracket read.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 *	@param	stg			Value storage
 *
 *	@return	Number of read values, @c SIZE_MAX on failure
 */
static size_t parse_brackets(parser *const prs, const size_t index, storage *const stg)
{
	size_t arg = 0;
	char32_t character = '\0';
	location loc = loc_copy(prs->loc);

	universal_io out = io_create();
	out_swap(prs->io, &out);

	while (character != ')' && character != (char32_t)EOF)
	{
		uni_scan_char(prs->io);
		character = skip_lines(prs);
		size_t position = in_get_position(prs->io);
		size_t brackets = 0;

		out_set_buffer(prs->io, MAX_VALUE_SIZE);
		while (brackets != 0 || (character != ',' && character != ')' && character != (char32_t)EOF))
		{
			uni_printf(prs->io, "%s", in_get_position(prs->io) != position ? " " : "");
			uni_print_char(prs->io, uni_scan_char(prs->io));
			if (character == '\'' || character == '"')
			{
				arg = skip_string(prs, character) == character ? arg : SIZE_MAX;
				uni_print_char(prs->io, character);
			}

			brackets += character != '(' ? character == ')' ? -1 : 0 : 1;
			position = in_get_position(prs->io);
			character = skip_lines(prs);
			if (character == (char32_t)EOF)
			{
				parser_error(prs, &loc, ARGS_UNTERMINATED, storage_to_string(prs->stg, index));
				arg = SIZE_MAX;
			}
		}

		if (arg != SIZE_MAX)
		{
			char *buffer = out_extract_buffer(prs->io);
			parse_values(prs, index, stg, buffer, arg++);
			free(buffer);
		}
	}

	out_swap(prs->io, &out);
	out_clear(&out);
	return arg;
}

/**
 *	Parse macro replacement value.
 *	Apply arguments and operators to comply observation area.
 *	After that, preprocess the result value.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 *	@param	stg			Value storage
 */
static void parse_observation(parser *const prs, const size_t index, storage *const stg)
{
	universal_io *io = prs->io;
	universal_io value = io_create();
	in_set_buffer(&value, storage_get_by_index(prs->stg, index));
	out_set_buffer(&value, MAX_VALUE_SIZE);
	prs->io = &value;

	location *loc = prs->loc;
	prs->loc = NULL;

	for (char32_t ch = skip_until(prs, true); ch != (char32_t)EOF; ch = skip_until(prs, true))
	{
		if (ch == '#' || utf8_is_letter(ch))
		{
			const size_t current = storage_search(stg, prs->io);
			uni_printf(prs->io, "%s", kw_is_correct(current) || current == SIZE_MAX
				? storage_last_read(stg) : storage_get_by_index(stg, current));
		}
		else if (ch == '\'' || ch == '"')
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
			uni_print_char(prs->io, skip_string(prs, ch));
		}
		else
		{
			uni_print_char(prs->io, uni_scan_char(prs->io));
		}
	}

	prs->loc = loc;
	prs->io = io;

	char *buffer = out_extract_buffer(&value);
	in_set_buffer(&value, buffer);
	parser_preprocess(prs, &value);
	in_clear(&value);
	free(buffer);
}

/**
 *	Parse macro replacement call.
 *	Analyze arguments count and parse brackets.
 *	Nearest empty brackets will be proceed, if macro has no arguments.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 */
static void parse_replacement(parser *const prs, const size_t index)
{
	const size_t expected = storage_get_args_by_index(prs->stg, index);
	const size_t position = in_get_position(prs->io);
	location loc = loc_copy(prs->loc);

	if (expected == 0)
	{
		if (skip_lines(prs) != '(' || uni_scan_char(prs->io) != '('
			|| skip_lines(prs) != ')' || uni_scan_char(prs->io) != ')')
		{
			*prs->loc = loc;
			in_set_position(prs->io, position);
		}

		universal_io value = io_create();
		in_set_buffer(&value, storage_get_by_index(prs->stg, index));
		parser_preprocess(prs, &value);
		in_clear(&value);
		return;
	}

	if (skip_lines(prs) != '(')
	{
		*prs->loc = loc;
		in_set_position(prs->io, position);
		parser_error(prs, prs->prev, ARGS_NON, storage_to_string(prs->stg, index));
		return;
	}

	storage stg = storage_create();
	const size_t actual = parse_brackets(prs, index, &stg);
	if (expected == actual)
	{
		parse_observation(prs, index, &stg);
	}
	else if (actual != SIZE_MAX)
	{
		loc_search_from(prs->loc);
		parser_error(prs, prs->loc, expected > actual ? ARGS_REQUIRES : ARGS_PASSED
			, storage_to_string(prs->stg, index), expected, actual);
	}

	uni_scan_char(prs->io);
	storage_clear(&stg);
}

/**
 *	Parse and replace identifier, if macro name recognized.
 *	Emit an error on @c MAX_CALL_DAPTH reached.
 *	Skip non macro identifiers.
 *
 *	@param	prs			Parser structure
 */
static void parse_identifier(parser *const prs)
{
	const size_t begin = in_get_position(prs->io);
	const size_t index = storage_search(prs->stg, prs->io);

	if (index == SIZE_MAX)
	{
		uni_printf(prs->io, "%s", storage_last_read(prs->stg));
		return;
	}

	if (prs->call >= MAX_CALL_DAPTH)
	{
		loc_search_from(prs->loc);
		parser_error(prs, prs->loc, CALL_DEPTH);
		uni_printf(prs->io, "%s", storage_last_read(prs->stg));
		return;
	}

	prs->call++;
	if (in_is_file(prs->io))
	{
		const size_t end = in_get_position(prs->io);
		in_set_position(prs->io, begin);

		location loc = loc_copy(prs->loc);
		prs->prev = &loc;

		uni_print_char(prs->io, '\n');
		loc_update_begin(prs->loc);
		in_set_position(prs->io, end);

		parse_replacement(prs, index);
		prs->prev = NULL;

		uni_print_char(prs->io, '\n');
		loc_update_end(prs->loc);
	}
	else
	{
		parse_replacement(prs, index);
	}
	prs->call--;
}

/**
 *	Parse usual content until line break.
 *	Only macro replacement identifiers allowed.
 *	Paste line break, if @c EOF occurred after significant character.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_until(parser *const prs)
{
	const size_t position = in_get_position(prs->io);
	char32_t character = '\0';

	while (character != '\n' && character != (char32_t)EOF)
	{
		character = skip_until(prs, true);

		if (utf8_is_letter(character) && out_is_correct(prs->io))
		{
			parse_identifier(prs);
		}
		else if (character == '#')
		{
			loc_search_from(prs->loc);
			parser_error(prs, prs->loc, HASH_STRAY);
			uni_print_char(prs->io, uni_scan_char(prs->io));
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

	if (character != (char32_t)EOF)
	{
		loc_line_break(prs->loc);
	}
	else if (prs->prev == NULL && in_get_position(prs->io) != position)
	{
		uni_print_char(prs->io, '\n');
	}

	return character;
}

/**
 *	Parse line beginning until first significant character.
 *	Buffer read characters, if a hash character occurred.
 *	Produce @c #line directives to output, if required.
 *	It also skip empty lines.
 *
 *	@param	prs			Parser structure
 *	@param	out			IO for buffering
 *
 *	@return	First significant character
 */
static char32_t parse_hash(parser *const prs, universal_io *const out)
{
	out_swap(prs->io, out);
	out_set_buffer(prs->io, MAX_COMMENT_SIZE);

	while (true) {
		if (prs->is_line_required)
		{
			out_set_buffer(prs->io, MAX_COMMENT_SIZE);
			loc_update(prs->loc);
		}

		char32_t character = skip_until(prs, true);
		if (character != '\n')
		{
			out_swap(prs->io, out);
			if (character != '#')
			{
				char *buffer = out_extract_buffer(out);
				uni_printf(prs->io, "%s", character != (char32_t)EOF ? buffer : "");
				free(buffer);
			}

			prs->is_line_required = false;
			return character;
		}

		uni_print_char(prs->io, uni_scan_char(prs->io));
		loc_line_break(prs->loc);
	}
}

/**
 *	Parse directive keyword and paste @c #line marks.
 *	Unsupport directive name splitting by backslash.
 *	But hash separation is acceptable.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Recognized keyword token,
 *			@c ERROR_KEYWORD on failure,
 *			@c NON_KEYWORD on otherwise
 */
static keyword_t parse_directive(parser *const prs)
{
	universal_io out = io_create();
	if (parse_hash(prs, &out) != '#')
	{
		return NON_KEYWORD;
	}

	location loc = loc_copy(prs->loc);
	uni_print_char(&out, '#');

	size_t keyword = storage_search(prs->stg, prs->io);
	if (storage_last_read(prs->stg)[1] == '\0')
	{
		out_swap(prs->io, &out);
		if (utf8_is_letter(skip_until(prs, true)))
		{
			loc = loc_copy(prs->loc);
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
			parser_error(prs, &loc, HASH_STRAY);
			uni_unscan(prs->io, &directive[1]);
		}

		free(buffer);
		return ERROR_KEYWORD;
	}

	out_clear(&out);
	return keyword;
}


/**
 *	Parse directive name location.
 *	Returns name only, if hash has been split.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Location of directive name
 */
static location parse_location(parser *const prs)
{
	const size_t position = in_get_position(prs->io);
	uni_unscan(prs->io, storage_last_read(prs->stg));
	if (uni_scan_char(prs->io) == '#')
	{
		uni_unscan_char(prs->io, '#');
	}

	location loc = loc_copy(prs->loc);
	in_set_position(prs->io, position);
	return loc;
}

/**
 *	Parse @c #line directive.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_line(parser *const prs)
{
	parse_location(prs);
	parser_warning(prs, prs->loc, DIRECTIVE_LINE_SKIPED);
	return skip_directive(prs);
}

/**
 *	Parse extra tokens after the directive.
 *	Emit a warning, if occurred.
 *
 *	@param	prs			Parser structure
 *	@param	directive	Directive name
 */
static void parse_extra(parser *const prs, const char *const directive)
{
	if (skip_until(prs, false) != '\n')
	{
		loc_search_from(prs->loc);
		parser_warning(prs, prs->loc, DIRECTIVE_EXTRA_TOKENS, directive);
	}
}

/**
 *	Parse the filename path and link it.
 *	Extra tokens after closing quote not accepted.
 *
 *	@param	prs			Parser structure
 *	@param	quote		Expected closing quote
 */
static void parse_path(parser *const prs, const char32_t quote)
{
	location loc = loc_copy(prs->loc);
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

	parse_extra(prs, storage_last_read(prs->stg));
	universal_io header = linker_add_header(prs->lk, index);
	parser_preprocess(prs, &header);
	in_clear(&header);
}

/**
 *	Parse @c #include directive.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_include(parser *const prs)
{
	location loc = parse_location(prs);
	if (prs->include >= MAX_INCLUDE_DEPTH)
	{
		parser_error(prs, &loc, INCLUDE_DEPTH);
		return skip_directive(prs);
	}

	prs->include++;
	char32_t character = skip_until(prs, false);
	switch (character)
	{
		case '<':
			character = '>';
		case '"':
			parse_path(prs, character);
			break;

		case '\n':
			parser_error(prs, &loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
		default:
			loc_search_from(prs->loc);
			parser_error(prs, prs->loc, INCLUDE_EXPECTS_FILENAME, storage_last_read(prs->stg));
			break;
	}

	prs->include--;
	return skip_directive(prs);
}


/**
 *	Parse macro name after directive.
 *	Check that name is valid or emit an error.
 *	Exit without reading the name.
 *
 *	@param	prs			Parser structure
 *
 *	@return	@c true on valid name, @c false on failure
 */
static bool parse_name(parser *const prs)
{
	location loc = parse_location(prs);
	char32_t character = skip_until(prs, false);
	if (utf8_is_letter(character))
	{
		return true;
	}

	if (character == '\n' || character == (char32_t)EOF)
	{
		parser_error(prs, &loc, DIRECTIVE_NAME_NON, storage_last_read(prs->stg));
	}
	else
	{
		loc_search_from(prs->loc);
		parser_error(prs, prs->loc, MACRO_NAME_FIRST_CHARACTER);
	}

	return false;
}

/**
 *	Parse macro definition arguments.
 *	Save the scanned arguments to the parser's storage.
 *	Use their indexes as the value.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 *
 *	@return	Number of arguments, @c SIZE_MAX on failure
 */
static size_t parse_args(parser *const prs, const size_t index)
{
	const size_t position = in_get_position(prs->io);
	if (skip_until(prs, false) != '(' || position != in_get_position(prs->io))
	{
		return 0;
	}

	location loc = loc_copy(prs->loc);
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

		loc_search_from(prs->loc);
		if (!utf8_is_letter(character))
		{
			parser_error(prs, prs->loc, ARGS_EXPECTED_NAME, character, prs->io);
			break;
		}

		const size_t argument = storage_add_by_io(prs->stg, prs->io);
		if (argument == SIZE_MAX)
		{
			parser_error(prs, prs->loc, ARGS_DUPLICATE, storage_last_read(prs->stg));
			break;
		}

		char buffer[MAX_INDEX_SIZE];
		sprintf(buffer, "%zu" MASK_SUFFIX "%zu", index, i);
		storage_set_by_index(prs->stg, argument, buffer);

		character = skip_until(prs, false);
		if (character == ',')
		{
			uni_scan_char(prs->io);
			character = skip_until(prs, false);
		}
		else if (character != ')' && character != '\n' && character != (char32_t)EOF)
		{
			loc_search_from(prs->loc);
			parser_error(prs, prs->loc, ARGS_EXPECTED_COMMA, character, prs->io);
			break;
		}
	}

	return SIZE_MAX;
}

/**
 *	Parse preprocessor operators of argument.
 *	Produce a masked replacement for argument operators.
 *
 *	@param	prs			Parser structure
 *	@param	was_space	Set, if separator required
 *
 *	@return	@c true on success, @c false on failure
 */
static bool parse_operator(parser *const prs, const bool was_space)
{
	location loc = loc_copy(prs->loc);
	uni_scan_char(prs->io);

	char32_t character = uni_scan_char(prs->io);
	if (character == '#')
	{
		character = skip_until(prs, false);
		if (character == '\n' || character == (char32_t)EOF)
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

		uni_printf(prs->io, MASK_TOKEN_PASTE "%s", value);
		return true;
	}

	uni_unscan_char(prs->io, character);
	skip_until(prs, false);

	const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
	if (value == NULL)
	{
		parser_error(prs, &loc, HASH_NOT_FOLLOWED, "#");
		return false;
	}

	uni_printf(prs->io, "%s" MASK_STRING "%s", was_space ? " " : "", value);
	return true;
}

/**
 *	Parse macro content and prepare value for saving.
 *	All separator sequences will be replaced by a single space.
 *	Return an empty string for the correct macro without value.
 *	Return value require to call @c free() function.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Macro value, @c NULL on failure
 */
static char *parse_content(parser *const prs)
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
			if (!parse_operator(prs, in_get_position(prs->io) != position))
			{
				out_swap(prs->io, &out);
				out_clear(&out);
				return NULL;
			}
		}
		else
		{
			uni_printf(prs->io, "%s", in_get_position(prs->io) != position ? " " : "");
			if (utf8_is_letter(character))
			{
				const char *value = storage_get_by_index(prs->stg, storage_search(prs->stg, prs->io));
				if (value != NULL)
				{
					uni_printf(prs->io, MASK_ARGUMENT "%s", value);
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

/**
 *	Parse the context of the saved macro.
 *	Create temporary storage for arguments.
 *	Set a velue for the parsed macro name.
 *
 *	@param	prs			Parser structure
 *	@param	index		Index of macro
 */
static void parse_context(parser *const prs, const size_t index)
{
	storage stg = storage_create();
	storage *origin = prs->stg;
	prs->stg = &stg;

	const size_t args = parse_args(prs, index);
	if (args != SIZE_MAX)
	{
		if (skip_until(prs, false) == '#')
		{
			loc_search_from(prs->loc);
			uni_scan_char(prs->io);
			char32_t character = uni_scan_char(prs->io);
			if (character == '#')
			{
				parser_error(prs, prs->loc, HASH_ON_EDGE);
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

		char *value = parse_content(prs);
		if (value != NULL)
		{
			storage_set_args_by_index(origin, index, args);
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

/**
 *	Parse @c #define directive.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_define(parser *const prs)
{
	if (parse_name(prs))
	{
		const size_t position = in_get_position(prs->io);
		size_t index = storage_add_by_io(prs->stg, prs->io);

		if (index == SIZE_MAX)
		{
			in_set_position(prs->io, position);
			loc_search_from(prs->loc);
			parser_warning(prs, prs->loc, MACRO_NAME_REDEFINE, storage_last_read(prs->stg));
			index = storage_search(prs->stg, prs->io);
		}

		parse_context(prs, index);
	}

	return skip_directive(prs);
}

/**
 *	Parse @c #set directive.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_set(parser *const prs)
{
	if (parse_name(prs))
	{
		const size_t position = in_get_position(prs->io);
		size_t index = storage_search(prs->stg, prs->io);

		if (index == SIZE_MAX)
		{
			in_set_position(prs->io, position);
			loc_search_from(prs->loc);
			parser_warning(prs, prs->loc, MACRO_NAME_UNDEFINED, storage_last_read(prs->stg));
			index = storage_add_by_io(prs->stg, prs->io);
		}

		parse_context(prs, index);
	}

	return skip_directive(prs);
}

/**
 *	Parse @c #undef directive.
 *
 *	@param	prs			Parser structure
 *
 *	@return	Last read character
 */
static char32_t parse_undef(parser *const prs)
{
	if (parse_name(prs))
	{
		storage_remove_by_index(prs->stg, storage_search(prs->stg, prs->io));
	}

	return skip_directive(prs);
}


/**
 *	Parse @c #macro directive.
 *
 *	@param	prs			Parser structure
 */
static void parse_macro(parser *const prs)
{
	location loc = parse_location(prs);
	char directive[MAX_KEYWORD_SIZE];
	sprintf(directive, "%s", storage_last_read(prs->stg));

	size_t index = SIZE_MAX;
	if (parse_name(prs))
	{
		const size_t position = in_get_position(prs->io);
		index = storage_add_by_io(prs->stg, prs->io);

		if (index == SIZE_MAX)
		{
			in_set_position(prs->io, position);
			loc_search_from(prs->loc);
			parser_warning(prs, prs->loc, MACRO_NAME_REDEFINE, storage_last_read(prs->stg));
			index = storage_search(prs->stg, prs->io);
		}
	}

	storage *origin = prs->stg;
	storage stg = storage_create();
	prs->stg = &stg;

	skip_until(prs, false);
	const size_t args = index != SIZE_MAX ? parse_args(prs, index) : SIZE_MAX;
	universal_io out = io_create();
	if (args != SIZE_MAX)
	{
		out_set_buffer(&out, MAX_VALUE_SIZE);
		parse_extra(prs, directive);
	}

	skip_directive(prs);
	out_swap(prs->io, &out);
	if (skip_block(prs, KW_MACRO) != KW_ENDM)
	{
		parser_error(prs, &loc, DIRECTIVE_UNTERMINATED, directive);
	}
	out_swap(prs->io, &out);
	parse_extra(prs, directive);
	skip_directive(prs);

	storage_clear(&stg);
	prs->stg = origin;

	char *value = out_extract_buffer(&out);
	if (value != NULL)
	{
		storage_set_args_by_index(prs->stg, index, args);
		storage_set_by_index(prs->stg, index, value);
		free(value);
	}
	else
	{
		storage_remove_by_index(prs->stg, index);
	}
}


/**
 *	Parse the next read token and check its corresponding the begin one.
 *	Emit an error for an invalid token.
 *
 *	@param	prs			Parser structure
 *	@param	begin		Begin token
 *	@param	next		End / else token
 *
 *	@return	@c true on success, @c false on failure
 */
static bool parse_next(parser *const prs, const keyword_t begin, const keyword_t next)
{
	if (((begin == KW_IFDEF || begin == KW_IFNDEF || begin == KW_IF || begin == KW_ELIF)
		&& (next == KW_ELIF || next == KW_ELSE || next == KW_ENDIF)) || (begin == KW_ELSE && next == KW_ENDIF)
		|| (begin == KW_WHILE && next == KW_ENDW) || (begin == KW_MACRO && next == KW_ENDM))
	{
		return true;
	}

	location loc = parse_location(prs);
	parser_error(prs, &loc, begin == KW_ELSE && (next == KW_ELIF || next == KW_ELSE)
		? DIRECTIVE_AFTER : DIRECTIVE_WITHOUT, storage_last_read(prs->stg));
	return false;
}

/**
 *	Parse multiline directive block.
 *	Stop at the end / else token if a begin was set.
 *	By default parse until EOF.
 *	Available begin tokens - @c #if @c #elif @c #else @c #while
 *
 *	@param	prs			Parser structure
 *	@param	begin		Begin token
 *
 *	@return	End / else token, @c NON_KEYWORD on otherwise
 */
static keyword_t parse_block(parser *const prs, const keyword_t begin)
{
	char32_t character = '\0';
	while (character != (char32_t)EOF)
	{
		const keyword_t keyword = parse_directive(prs);
		switch (keyword)
		{
			case KW_LINE:
				character = parse_line(prs);
				break;
			case KW_INCLUDE:
				character = parse_include(prs);
				break;

			case KW_DEFINE:
				character = parse_define(prs);
				break;
			case KW_SET:
				character = parse_set(prs);
				break;
			case KW_UNDEF:
				character = parse_undef(prs);
				break;

			case KW_MACRO:
				parse_macro(prs);
				break;
			case KW_IFDEF:
			case KW_IFNDEF:

			case KW_IF:
			case KW_WHILE:
			case KW_EVAL:
				character = skip_directive(prs);
				break;

			case NON_KEYWORD:
			case ERROR_KEYWORD:
				character = parse_until(prs);
				break;
			default:
				if (!parse_next(prs, begin, keyword))
				{
					character = skip_directive(prs);
					break;
				}
				return keyword;
		}
	}

	return NON_KEYWORD;
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
	prs.prev = NULL;
	prs.loc = NULL;

	prs.include = 0;
	prs.call = 0;

	prs.is_recovery_disabled = false;
	prs.is_line_required = false;
	prs.is_macro_processed = false;
	prs.was_error = false;

	return prs;
}


int parser_preprocess(parser *const prs, universal_io *const in)
{
	if (!parser_is_correct(prs) || !in_is_correct(in))
	{
		return -1;
	}

	universal_io *io = prs->io;
	out_swap(io, in);
	prs->io = in;

	prs->is_line_required = true;
	location current = loc_search(prs->io);
	location *loc = prs->loc;
	prs->loc = &current;

	parse_block(prs, NON_KEYWORD);

	prs->io = io;
	prs->loc = loc;
	out_swap(io, in);
	return prs->was_error;
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
