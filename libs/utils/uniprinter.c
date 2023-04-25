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

#include "uniprinter.h"
#include "utf8.h"
#include <stdarg.h>


int uni_printf(universal_io *const io, const char *const format, ...)
{
    if (!out_is_correct(io))
    {
        return -1;
    }

    va_list args;
    va_start(args, format);

    io_func func = out_get_func(io);
    int ret = func(io, format, args);

    va_end(args);
    return ret;
}

int uni_print_char(universal_io *const io, const char32_t wchar)
{
    char buffer[8];

    if (!utf8_to_string(buffer, wchar))
    {
        return 0;
    }

    return uni_printf(io, "%s", buffer);
}
