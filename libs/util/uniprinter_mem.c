/*
 *	Copyright 2019 Andrey Terekhov
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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Memory-based printer */
static int printer_mem_fprintf(universal_printer_options *opts, const char *fmt, va_list args)
{
	char buf[200] = { '\0' };
	char *buf_to_use = buf;
	char *allocated = NULL;
	int ret;
	va_list args2;
	va_list args3;

	va_copy(args2, args);
	va_copy(args3, args);

	ret = vsnprintf(buf, sizeof(buf), fmt, args);
	if (ret < 0)
	{
		va_end(args2);
		va_end(args3);
		return ret;
	}

	if ((size_t)ret >= sizeof(buf))
	{
		allocated = malloc(ret);
		ret = vsnprintf(allocated, ret, fmt, args2);
		if (ret < 0)
		{
			free(allocated);
			va_end(args2);
			va_end(args3);
			return ret;
		}
		buf_to_use = allocated;
	}

	if (opts->size < (opts->pos + ret + 1))
	{
		size_t new_size = (opts->size * 2) > (opts->pos + ret + 1) ? (opts->size * 2) : (opts->pos + ret + 1);
		char *reallocated = realloc(opts->ptr, new_size);

		if (reallocated == NULL)
		{
			va_end(args2);
			va_end(args3);
			free(allocated);
			return -1;
		}

		memset(&reallocated[opts->size], 0, new_size - opts->size);
		opts->ptr = reallocated;
		opts->size = new_size;
	}

	ret = vsnprintf(buf_to_use, ret + 1, fmt, args3);
	if (ret >= 0)
	{
		memcpy(&opts->ptr[opts->pos], buf_to_use, ret);
		opts->pos += ret;
	}

	free(allocated);
	va_end(args2);
	va_end(args3);
	return ret;
}

printer_desc printer_mem = { IO_SOURCE_MEM, printer_mem_fprintf };
