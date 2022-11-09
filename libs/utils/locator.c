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

#include "locator.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "uniprinter.h"
#include "uniscanner.h"
#include "utf8.h"


#define MAX_PATH 1024


static const size_t FIRST_LINE = 1;
static const size_t FIRST_SYMBOL = 1;

static const char *const PREFIX = "#line";
static const char *const COMMENT = "//";
static const char SEPARATOR = ' ';
static const char QUOTE = '"';


static void line_to_begin(universal_io *const io, size_t *const symbol, size_t *const filler)
{
	*symbol = FIRST_SYMBOL;
	*filler = FIRST_SYMBOL;

	size_t position = in_get_position(io);
	if (position == 0)
	{
		return;
	}

	char ch = '\0';
	in_set_position(io, --position);
	uni_scanf(io, "%c", &ch);

	while (ch != '\n')
	{
		*symbol += 2 - utf8_symbol_size(ch);
		*filler = ch == ' ' || ch == '\t' ? (*filler) + 1 : FIRST_SYMBOL;

		if (position == 0)
		{
			in_set_position(io, 0);
			break;
		}

		in_set_position(io, --position);
		uni_scanf(io, "%c", &ch);
	}
}

static inline void line_to_end(universal_io *const io)
{
	char32_t character = uni_scan_char(io);
	while (character != '\n' && character != (char32_t)EOF)
	{
		character = uni_scan_char(io);
	}
}

static inline bool mark_compare(universal_io *const io, const char *const str)
{
	char ch = '\0';
	for (size_t i = 0; str[i] != '\0'; i++)
	{
		uni_scanf(io, "%c", &ch);
		if (ch != str[i])
		{
			return false;
		}
	}

	uni_scanf(io, "%c", &ch);
	return ch == SEPARATOR;
}

static bool mark_recognize(universal_io *const io
	, size_t *const line, size_t *const path, size_t *const code)
{
	if (!mark_compare(io, PREFIX))
	{
		return false;
	}

	uni_scanf(io, "%zu", line);
	*path = SIZE_MAX;
	*code = SIZE_MAX;

	char ch = '\0';
	uni_scanf(io, "%c", &ch);
	if (ch != SEPARATOR)
	{
		return true;
	}

	uni_scanf(io, "%c", &ch);
	*path = in_get_position(io);
	while (uni_scanf(io, "%c", &ch) == 1 && ch != QUOTE);

	uni_scanf(io, "%c", &ch);
	if (ch == SEPARATOR && mark_compare(io, COMMENT))
	{
		*code = in_get_position(io);
	}

	return true;
}

static bool mark_reverse_recognize(universal_io *const io
	, size_t *const line, size_t *const path, size_t *const code, size_t *const filler)
{
	size_t position = in_get_position(io);
	if (position == 0)
	{
		return false;
	}

	size_t candidate;
	in_set_position(io, position - 1);
	line_to_begin(io, &position, &candidate);
	position = in_get_position(io);

	bool found = mark_recognize(io, line, path, code);
	if (!found)
	{
		*filler = candidate;
	}

	in_set_position(io, position);
	return found;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


location loc_create(universal_io *const io)
{
	location loc = loc_search(io);
	loc_update(&loc);
	return loc;
}

location loc_create_begin(universal_io *const io)
{
	location loc = loc_search(io);
	loc_update_begin(&loc);
	return loc;
}

location loc_create_end(universal_io *const io)
{
	location loc = loc_search(io);
	loc_update_end(&loc);
	return loc;
}


int loc_update(location *const loc)
{
	char path[MAX_PATH];
	if (loc_search_from(loc) || !loc_get_path(loc, path) || !out_is_correct(loc->io))
	{
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu%c%c%s%c\n", PREFIX, SEPARATOR, loc->line
		, SEPARATOR, QUOTE, path, QUOTE);

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);
	for (size_t i = FIRST_SYMBOL; i < loc->symbol; i++)
	{
		uni_print_char(loc->io, uni_scan_char(loc->io) == '\t' ? '\t' : ' ');
	}

	in_set_position(loc->io, position);
	return 0;
}

int loc_update_begin(location *const loc)
{
	char path[MAX_PATH];
	if (loc_search_from(loc) || !loc_get_path(loc, path) || !out_is_correct(loc->io))
	{
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu%c%c%s%c%c%s%c", PREFIX, SEPARATOR, loc->line
		, SEPARATOR, QUOTE, path, QUOTE, SEPARATOR, COMMENT, SEPARATOR);

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);

	char32_t character = uni_scan_char(loc->io);
	while (character != '\n' && character != (char32_t)EOF)
	{
		uni_print_char(loc->io, character);
		character = uni_scan_char(loc->io);
	}

	uni_print_char(loc->io, '\n');

	in_set_position(loc->io, loc->code);
	for (size_t i = FIRST_SYMBOL; i < loc->symbol; i++)
	{
		uni_print_char(loc->io, uni_scan_char(loc->io) == '\t' ? '\t' : ' ');
	}

	in_set_position(loc->io, position);
	return 0;
}

int loc_update_end(location *const loc)
{
	char path[MAX_PATH];
	if (loc_search_from(loc) || !loc_get_path(loc, path) || !out_is_correct(loc->io))
	{
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu\n", PREFIX, SEPARATOR, loc->line);

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);
	for (size_t i = FIRST_SYMBOL; i < loc->symbol; i++)
	{
		uni_print_char(loc->io, uni_scan_char(loc->io) == '\t' ? '\t' : ' ');
	}

	in_set_position(loc->io, position);
	return 0;
}


location loc_search(universal_io *const io)
{
	location loc;

	loc.io = io;
	loc.path = SIZE_MAX;
	loc.code = 0;
	loc.line = FIRST_LINE;
	loc.symbol = FIRST_SYMBOL;

	if (loc_search_from(&loc))
	{
		loc.io = NULL;
	}

	return loc;
}

int loc_search_from(location *const loc)
{
	if (!loc_is_correct(loc))
	{
		return -1;
	}

	const size_t position = in_get_position(loc->io);
	size_t filler = FIRST_SYMBOL;
	line_to_begin(loc->io, &loc->symbol, &filler);
	const size_t code = in_get_position(loc->io);

	line_to_end(loc->io);
	size_t line = FIRST_LINE;
	size_t path = SIZE_MAX;
	size_t comment = SIZE_MAX;
	if (mark_recognize(loc->io, &line, &path, &comment) && comment != SIZE_MAX)
	{
		loc->line = line;
		loc->path = path;
		loc->code = comment;
		in_set_position(loc->io, position);
		return 0;
	}

	size_t diff = 0;
	size_t begin = code;
	in_set_position(loc->io, code);
	while (begin != 0 && begin != loc->code)
	{
		if (!mark_reverse_recognize(loc->io, &line, &path, &comment, &filler))
		{
			begin = in_get_position(loc->io);
			diff++;
		}
		else if (comment != SIZE_MAX)	// macro begin
		{
			loc->symbol = filler;
			loc->line = line;
			loc->path = path;
			loc->code = comment;
			in_set_position(loc->io, position);
			return 0;
		}
		else if (path != SIZE_MAX)		// usual
		{
			loc->line = line;
			loc->path = path;
			break;
		}
		else							// macro end
		{
			loc->line = line + diff;
			while (!mark_reverse_recognize(loc->io, &line, &path, &comment, &filler)
				&& in_get_position(loc->io) != 0);
			
			loc->path = path;
			loc->code = loc->line == line && comment != SIZE_MAX ? comment : code;
			in_set_position(loc->io, position);
			return 0;
		}
	}

	if (begin == 0)
	{
		loc->path = SIZE_MAX;
		loc->line = FIRST_LINE;
	}

	loc->line += diff;
	loc->code = code;

	in_set_position(loc->io, position);
	return 0;
}


int loc_line_break(location *const loc)
{
	if (!loc_is_correct(loc))
	{
		return -1;
	}

	loc->code = in_get_position(loc->io);
	loc->line++;
	loc->symbol = FIRST_SYMBOL;

	return 0;
}

bool loc_is_correct(const location *const loc)
{
	return loc != NULL && in_is_correct(loc->io);
}


size_t loc_get_tag(location *const loc, char *const buffer)
{
	const size_t size = loc_get_path(loc, buffer);
	if (size == 0)
	{
		return 0;
	}

	return size + sprintf(&buffer[size], ":%zu:%zu", loc->line, loc->symbol);
}

size_t loc_get_code_line(location *const loc, char *const buffer)
{
	if (!loc_is_correct(loc) || buffer == NULL)
	{
		return 0;
	}

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);

	size_t size = 0;
	char32_t character = uni_scan_char(loc->io);
	while (character != '\n' && character != (char32_t)EOF)
	{
		size += utf8_to_string(&buffer[size], character);
		character = uni_scan_char(loc->io);
	}

	in_set_position(loc->io, position);
	return size;
}

size_t loc_get_path(location *const loc, char *const buffer)
{
	if (!loc_is_correct(loc) || buffer == NULL)
	{
		return 0;
	}

	if (loc->path == SIZE_MAX)
	{
		return in_get_path(loc->io, buffer);
	}

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->path);

	size_t size = 0;
	char32_t character = uni_scan_char(loc->io);
	while (character != (char32_t)QUOTE && size < MAX_PATH)
	{
		size += utf8_to_string(&buffer[size], character);
		character = uni_scan_char(loc->io);
	}

	in_set_position(loc->io, position);
	return size;
}

size_t loc_get_line(const location *const loc)
{
	return loc_is_correct(loc) ? loc->line : FIRST_LINE;
}

size_t loc_get_symbol(const location *const loc)
{
	return loc_is_correct(loc) ? loc->symbol : FIRST_SYMBOL;
}

size_t loc_get_index(location *const loc)
{
	if (!loc_is_correct(loc))
	{
		return 0;
	}

	const size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);

	for (size_t i = FIRST_SYMBOL; i < loc->symbol; i++)
	{
		uni_scan_char(loc->io);
	}

	const size_t index = in_get_position(loc->io) - loc->code;
	in_set_position(loc->io, position);
	return index;
}
