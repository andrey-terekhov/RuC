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
#include "utf8.h"


#define MAX_PATH 1024


static const char *const PREFIX = "#line";
static const char SEPARATOR = ' ';
static const char *const COMMENT = "//";
static const char FILLER = ' ';


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
	if (loc_search_from(loc) || !loc_get_path(loc, path))
	{
		loc->io = NULL;
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu%c\"%s\"\n", PREFIX, SEPARATOR, loc->line, SEPARATOR, path);
	return 0;
}

int loc_update_begin(location *const loc)
{
	char path[MAX_PATH];
	if (loc_search_from(loc) || !loc_get_path(loc, path))
	{
		loc->io = NULL;
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu%c\"%s\"%c%s%c", PREFIX, SEPARATOR, loc->line, SEPARATOR, path
		, SEPARATOR, COMMENT, SEPARATOR);

	size_t position = in_get_position(loc->io);
	in_set_position(loc->io, loc->code);

	char32_t character = uni_scan_char(loc->io);
	while (character != '\n' && character != EOF)
	{
		uni_print_char(loc->io, character);
		character = uni_scan_char(loc->io);
	}

	uni_print_char(loc->io, '\n');
	in_set_position(loc->io, position);

	for (size_t i = 0; i < loc->symbol; i++)
	{
		uni_print_char(loc->io, FILLER);
	}
	return 0;
}

int loc_update_end(location *const loc)
{
	char path[MAX_PATH];
	if (loc_search_from(loc) || !loc_get_path(loc, path))
	{
		loc->io = NULL;
		return -1;
	}

	uni_printf(loc->io, "%s%c%zu\n", PREFIX, SEPARATOR, loc->line);

	for (size_t i = 0; i < loc->symbol; i++)
	{
		uni_print_char(loc->io, FILLER);
	}
	return 0;
}


location loc_search(universal_io *const io)
{
	location loc;

	loc.io = io;
	loc.path = SIZE_MAX;
	loc.code = 0;
	loc.line = 0;
	loc.symbol = 0;

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

//

	return 0;
}


int loc_line_break(location *const loc) {}
bool loc_is_correct(const location *const loc)
{
	return loc != NULL && in_is_correct(loc->io);
}


size_t loc_get_tag(location *const loc, char *const buffer) {}
size_t loc_get_code_line(location *const loc, char *const buffer) {}
size_t loc_get_path(location *const loc, char *const buffer) {}
size_t loc_get_line(const location *const loc) {}
size_t loc_get_symbol(const location *const loc) {}
