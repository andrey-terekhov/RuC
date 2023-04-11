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

#include "uniscanner.h"
#include "utf8.h"
#include <stdarg.h>
#include <string.h>


int uni_scanf(universal_io *const io, const char *const format, ...)
{
    if (!in_is_correct(io))
    {
        return 0;
    }

    va_list args;
    va_start(args, format);

    io_func func = in_get_func(io);
    int ret = func(io, format, args);

    va_end(args);
    return ret;
}

char32_t uni_scan_char(universal_io *const io)
{
    char buffer[MAX_SYMBOL_SIZE];
    if (!uni_scanf(io, "%c", &buffer[0]))
    {
        return (char32_t)EOF;
    }

    const size_t size = utf8_symbol_size(buffer[0]);
    for (size_t i = 1; i < size; i++)
    {
        if (!uni_scanf(io, "%c", &buffer[i]))
        {
            return (char32_t)EOF;
        }
    }

    return utf8_convert(buffer);
}

size_t uni_scan_number(universal_io *const io, char *const buffer)
{
    const size_t begin = in_get_position(io);
    double number = 0;
    uni_scanf(io, "%lf", &number);

    const size_t end = in_get_position(io);
    in_set_position(io, begin);

    for (size_t index = 0; in_get_position(io) < end; index += utf8_to_string(&buffer[index], uni_scan_char(io)))
    {
    }

    return end - begin;
}

size_t uni_scan_identifier(universal_io *const io, char *const buffer)
{
    char32_t character = uni_scan_char(io);
    if (!utf8_is_letter(character))
    {
        uni_unscan_char(io, character);
        return 0;
    }

    size_t index = 0;
    while (utf8_is_letter(character) || utf8_is_digit(character))
    {
        index += utf8_to_string(&buffer[index], character);
        character = uni_scan_char(io);
    }

    uni_unscan_char(io, character);
    return index;
}

int uni_unscan_char(universal_io *const io, const char32_t wchar)
{
    return in_set_position(io, in_get_position(io) - utf8_size(wchar));
}

int uni_unscan(universal_io *const io, const char *const str)
{
    return str != NULL ? in_set_position(io, in_get_position(io) - strlen(str)) : -1;
}
