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

#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define UNUSED(x) (void)(x)


#if 0
/** Find a symbol in a buffer */
static int find_symbol(const char *buffer, unsigned char symbol)
{
	const char *tmp = strchr(buffer, symbol);
	if (tmp == NULL)
		return -1;

	return tmp - buffer;
}
#endif

/** In-memory UTF8 scanner */
int io_mem_getnext(universal_scanner_options *opts)
{
	unsigned char firstchar;
	unsigned char secondchar;
	int ret;
	int pos;
	int result;

	if (opts->ptr[opts->pos] == '\0')
	{
		return EOF;
	}

	result = sscanf(&opts->ptr[opts->pos], "%c%n", &firstchar, &pos);
	if (result != 1 || result == EOF)
	{
		return EOF;
	}

	/* We must find the symbol because we already did in sscanf() call */
	opts->pos += pos;

	if ((firstchar & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0)
	{
		result = sscanf(&opts->ptr[opts->pos], "%c%n", &secondchar, &pos);
		if (result != 1 || result == EOF)
		{
			return EOF;
		}

		opts->pos += pos;

		ret = ((int)(firstchar & /*0b11111*/ 0x1F)) << 6 | (secondchar & /*0b111111*/ 0x3F);
	}
	else
	{
		ret = firstchar;
	}

	if (ret == '\r')
	{
		return io_mem_getnext(opts);
	}

	return ret;
}

int io_mem_scanf(universal_scanner_options *opts, const char *fmt, va_list args)
{
	UNUSED(opts);
	UNUSED(fmt);
	UNUSED(args);

	fprintf(stderr, "mem scanf not implemented\n");
	exit(4);
}

scanner_desc scanner_mem = { IO_SOURCE_MEM, io_mem_getnext, io_mem_scanf };
