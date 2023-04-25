/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include "commenter.h"
#include "utf8.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static const char *const PREFIX = "// #";
static const char SEPARATOR = ' ';


static inline void cmt_parse(comment *const cmt)
{
    size_t i = 0;
    while (cmt->path[i] != '\0' && cmt->path[i] != '\n' && cmt->path[i] != SEPARATOR)
    {
        i++;
    }

    if (cmt->path[i] != SEPARATOR)
    {
        return;
    }

    size_t line = 0;
    if (sscanf(&(cmt->path[++i]), "%zu", &line) == 0)
    {
        return;
    }

    while (cmt->path[i] >= '0' && cmt->path[i] <= '9')
    {
        i++;
    }

    if (cmt->path[i] != SEPARATOR)
    {
        cmt->line += line - 2;
        return;
    }

    size_t symbol = 0;
    if (sscanf(&(cmt->path[++i]), "%zu", &symbol) != 0)
    {
        cmt->line = line;
        cmt->symbol = symbol;
    }
}

static inline void cmt_reverse(comment *const cmt, const char *const code, const size_t position)
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

    if (cmt->path != NULL)
    {
        cmt_parse(cmt);
    }
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


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
        return (size_t)sprintf(buffer, "%s%c%s%c%zu%c%zu\n", PREFIX, SEPARATOR, cmt->path, SEPARATOR, cmt->line,
                               SEPARATOR, cmt->symbol);
    }

    return (size_t)sprintf(buffer, "%s%c%s%c%zu\n", PREFIX, SEPARATOR, cmt->path, SEPARATOR, cmt->line);
}


comment cmt_search(const char *const code, const size_t position)
{
    comment cmt;
    cmt.path = NULL;
    cmt.line = 1;
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

    cmt_reverse(&cmt, code, i);
    return cmt;
}


bool cmt_is_correct(const comment *const cmt)
{
    return cmt != NULL && cmt->path != NULL;
}


size_t cmt_get_tag(const comment *const cmt, char *const buffer)
{
    size_t index = cmt_get_path(cmt, buffer);

    if (index == 0)
    {
        return 0;
    }

    index += (size_t)sprintf(&buffer[index], ":%zu", cmt->line);

    const size_t first = utf8_to_first_byte(cmt->code, cmt->symbol);
    size_t symbol = first;

    size_t i = 0;
    while (i < first)
    {
        const size_t size = utf8_symbol_size(cmt->code[i]);
        symbol -= size - 1;
        i += size;
    }

    return index + (size_t)sprintf(&buffer[index], ":%zu", symbol + 1);
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
    return cmt != NULL ? cmt->line : 0;
}

size_t cmt_get_symbol(const comment *const cmt)
{
    return cmt != NULL ? cmt->symbol : 0;
}
