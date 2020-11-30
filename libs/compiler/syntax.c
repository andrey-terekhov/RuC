/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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

#include "syntax.h"
#include <stdlib.h>
#include <string.h>


#ifdef __GNUC__
	#define likely(x)	__builtin_expect((x), 1)
	#define unlikely(x) __builtin_expect((x), 0)
#else
	#define likely(x)	(x)
	#define unlikely(x) (x)
#endif

#define DEFAULT_OUTBUF_SIZE (1024)


int syntax_init(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	compiler_table_init(&sx->reprtab);
	sx->reprtab.len = 1;

	sx->pc = 4;
	sx->procd = 1;
	sx->funcnum = 2;
	sx->id = 2;
	sx->md = 1;

	sx->maxdisplg = 3;

	return 0;
}

int syntax_deinit(syntax *const sx)
{
	if (sx == NULL || sx->reprtab.table == NULL)
	{
		return -1;
	}

	free(sx->reprtab.table);
	sx->reprtab.table = NULL;
	return 0;
}


void compiler_table_init(compiler_table *table)
{
	table->table = malloc(COMPILER_TABLE_SIZE_DEFAULT * sizeof(int));
	if (table->table == NULL)
	{
		exit(1); // need a better way to stop!
	}
	table->pos = 0;
	table->len = 0;
	table->size = COMPILER_TABLE_SIZE_DEFAULT;
	memset(table->table, 0, COMPILER_TABLE_SIZE_DEFAULT * sizeof(int));
}

int compiler_table_ensure_allocated(compiler_table *table, int pos)
{
	if (unlikely(pos >= table->size))
	{
		int size = (table->size * 2 > (pos + COMPILER_TABLE_INCREMENT_MIN)) ? (table->size * 2)
																			: (pos + COMPILER_TABLE_INCREMENT_MIN);
		int *buf = realloc(table->table, sizeof(int) * size);

		if (buf == NULL)
		{
			exit(1);
		}

		memset(&buf[table->size], 0, size - table->size);
		table->table = buf;
		table->size = size;
	}

	return table->size;
}

int compiler_table_expand(compiler_table *table, int len)
{
	return compiler_table_ensure_allocated(table, table->len + len);
}
