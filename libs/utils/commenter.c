#include "commenter.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


const char *const PREFIX = "// #";
const char SEPARATOR = ' ';


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
		return sprintf(buffer, "%s%c%s%c%li%c%li\n", PREFIX, SEPARATOR
			, cmt->path, SEPARATOR, cmt->line, SEPARATOR, cmt->symbol);
	}

	return sprintf(buffer, "%s%c%s%c%li\n", PREFIX, SEPARATOR, cmt->path, SEPARATOR, cmt->line);
}

void reverse_search(comment *const cmt, const char *const code, const size_t position)
{
	const size_t size = strlen(PREFIX);
	
	size_t i = position;
	while (i != 0)
	{
		if (code[i] == '\n')
		{
			cmt->line++;
			i--;
			continue;
		}

		size_t j = 0;
		while (j < size && PREFIX[size - j - 1] == code[i - j])
		{
			j++;
		}

		if (j == size)
		{
			cmt->path = &code[i + 2];
			break;
		}

		i--;
	}
}

comment cmt_search(const char *const code, const size_t position)
{
	comment cmt;
	cmt.path = NULL;
	cmt.line = 0;
	cmt.symbol = SIZE_MAX;
	cmt.code = NULL;
	
	if (code == NULL)
	{
		return cmt;
	}
	
	size_t i = position;
	while (i != 0 && code[i - 1] != '\n')
	{
		i--;
	}

	cmt.code = &code[i];
	cmt.symbol = position - i;

	reverse_search(&cmt, code, i);
	return cmt;
}


int cmt_is_correct(const comment *const cmt)
{
	return cmt != NULL && cmt->path != NULL;
}


size_t cmt_get_tag(const comment *const cmt, char *const buffer)
{
	size_t i = cmt_get_path(cmt, buffer);

	return i == 0 ? 0 : i + sprintf(&buffer[i], ":%li:%li", cmt->line, cmt->symbol);
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

	size_t i = 0;
	while (cmt->path[i] != '\0' && cmt->path[i] != '\n' && cmt->path[i] != SEPARATOR)
	{
		buffer[i] = cmt->path[i];
		i++;
	}

	buffer[i] = '\0';
	return i;
}

size_t cmt_get_line(const comment *const cmt)
{
	return cmt->line;
}

size_t cmt_get_symbol(const comment *const cmt)
{
	return cmt->symbol;
}
