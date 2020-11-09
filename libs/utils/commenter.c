#include "commenter.h"
#include <stdint.h>
#include <stdio.h>
//#include <string.h>


const char *const PREFIX = "//// #";
const char *const SEPARATOR = " ";


comment cmt_create(const char *const path, const size_t line)
{
	comment cmt;

	cmt.path = path;
	cmt.line = line;
	cmt.symbol = SIZE_MAX;
	cmt.code = NULL;

	return cmt;
}

comment cmt_create_macro(const char *const path, const size_t line, const size_t symbol)
{
	comment cmt;

	cmt.path = path;
	cmt.line = line;
	cmt.symbol = symbol;
	cmt.code = NULL;

	return cmt;
}


size_t cmt_to_string(const comment *const cmt, char *const buffer)
{
	if (!cmt_is_correct(cmt) || buffer == NULL)
	{
		return 0;
	}
	
	if (cmt->symbol != SIZE_MAX)
	{
		return sprintf(buffer, "%s%s%s%s%li%s%li\n", PREFIX, SEPARATOR
			, cmt->path, SEPARATOR, cmt->line, SEPARATOR, cmt->symbol);
	}

	return sprintf(buffer, "%s%s%s%s%li\n", PREFIX, SEPARATOR, cmt->path, SEPARATOR, cmt->line);
}

comment cmt_search(const char *const code, const size_t position)
{
	comment cmt;
	return cmt;
}


int cmt_is_correct(const comment *const cmt)
{
	return cmt != NULL && cmt->path != NULL;
}


size_t cmt_get_tag(const comment *const cmt, char *const buffer)
{
	if (!cmt_is_correct(cmt) || buffer == NULL)
	{
		return 0;
	}

	return sprintf(buffer, "%s:%li:%li", cmt->path, cmt->line, cmt->symbol);
}

size_t cmt_get_code_line(const comment *const cmt, char *const buffer)
{
	if (cmt == NULL || cmt->code == NULL || buffer == NULL)
	{
		return 0;
	}

	size_t i = 0;
	while (cmt->code[i] != '\0' && cmt->code[i] != '\n')
	{
		buffer[i] = cmt->code[i];
		i++;
	}

	buffer[i] = '\0';
	return i;
}

size_t cmt_get_path(const comment *const cmt, char *const buffer)
{
	if (!cmt_is_correct(cmt) || buffer == NULL)
	{
		return 0;
	}

	return sprintf(buffer, "%s", cmt->path);
}

size_t cmt_get_line(const comment *const cmt)
{
	return cmt->line;
}

size_t cmt_get_symbol(const comment *const cmt)
{
	return cmt->symbol;
}
