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

/**	Check if modes are equal */
int mode_is_equal(const syntax *const sx, const size_t first, const size_t second)
{
	if (sx->modetab[first] != sx->modetab[second])
	{
		return 0;
	}

	size_t length = 1;
	int mode = sx->modetab[first];
	// Определяем, сколько полей надо сравнивать для различных типов записей
	if (mode == MSTRUCT || mode == MFUNCTION)
	{
		length = 2 + sx->modetab[first + 2];
	}

	for (size_t i = 1; i <= length; i++)
	{
		if (sx->modetab[first + i] != sx->modetab[second + i])
		{
			return 0;
		}
	}

	return 1;
}

/**	Check if representations are equal */
int repr_is_equal(const syntax *const sx, const size_t first, const size_t second)
{
	size_t i = 2;
	while (sx->reprtab[first + i] == sx->reprtab[second + i])
	{
		i++;
		if (sx->reprtab[first + i] == 0 && sx->reprtab[second + i] == 0)
		{
			return 1;
		}
	}
	return 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


syntax sx_create()
{
	syntax sx;

	sx.pc = 4;
	sx.procd = 1;
	sx.funcnum = 2;
	sx.id = 2;
	sx.md = 1;
	sx.startmode = 1;
	sx.tc = 0;
	sx.rp = 1;

	sx.maxdisplg = 3;
	sx.wasmain = 0;

	sx.anstdispl = 0;
	
	for (int i = 0; i < 256; i++)
	{
		sx.hashtab[i] = 0;
	}

	return sx;
}


int mem_set(syntax *const sx, const size_t index, const int value)
{
	if (sx == NULL || (int)index >= sx->pc)
	{
		return -1;
	}

	sx->mem[index] = value;
	return 0;
}

int mem_get(const syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->pc)
	{
		return INT_MAX;
	}

	return sx->mem[index];
}


int proc_set(syntax *const sx, const size_t index, const int value)
{
	if (sx == NULL || (int)index >= sx->procd)
	{
		return -1;
	}

	sx->iniprocs[index] = value;
	return 0;
}

int proc_get(const syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->procd)
	{
		return INT_MAX;
	}

	return sx->iniprocs[index];
}


int func_add(syntax *const sx, const size_t ref)
{
	if (sx == NULL)
	{
		return -1;
	}

	sx->funcnum++;
	return func_set(sx, sx->funcnum - 1, ref);
}

int func_set(syntax *const sx, const size_t index, const size_t ref)
{
	if (sx == NULL || (int)index >= sx->funcnum)
	{
		return -1;
	}

	sx->functions[index] = (int)ref;
	return 0;
}

int func_get(const syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->funcnum)
	{
		return INT_MAX;
	}

	return sx->functions[index];
}


size_t mode_add(syntax *const sx, const int *const record, const size_t size)
{
	if (sx == NULL || record == NULL)
	{
		return SIZE_MAX;
	}

	sx->modetab[sx->md] = sx->startmode;
	sx->startmode = sx->md++;
	for (size_t i = 0; i < size; i++)
	{
		sx->modetab[sx->md++] = record[i];
	}

	// Checking mode duplicates
	size_t old = sx->modetab[sx->startmode];
	while (old)
	{
		if (mode_is_equal(sx, sx->startmode + 1, old + 1))
		{
			sx->md = sx->startmode;
			sx->startmode = sx->modetab[sx->startmode];
			return old + 1;
		}
		else
		{
			old = sx->modetab[old];
		}
	}

	return sx->startmode + 1;
}

int mode_get(const syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->md)
	{
		return INT_MAX;
	}

	return sx->modetab[index];
}


size_t repr_add(syntax *const sx, const char32_t *const spelling)
{
	const size_t old_repr = sx->rp;
	uint8_t hash = 0;
	size_t i = 0;
	sx->rp += 2;

	do
	{
		 hash += spelling[i];
		 sx->reprtab[sx->rp++] = (spelling[i] & 255);
	} while (spelling[i++] != 0);

	sx->hash = (int)hash;

	size_t cur_repr = sx->hashtab[hash];
	if (cur_repr != 0)
	{
		do
		{
			if (repr_is_equal(sx, cur_repr, old_repr))
			{
				sx->rp = old_repr;
				return cur_repr;
			}
			else
			{
				cur_repr = sx->reprtab[cur_repr];
			}
		} while (cur_repr != 0);
	}

	sx->reprtab[old_repr] = (int)sx->hashtab[hash];
	sx->hashtab[hash] = old_repr;
	// 0 - только MAIN, (< 0) - ключевые слова, 1 - обычные иденты
	sx->reprtab[old_repr + 1] = (sx->keywordsnum) ? -((++sx->keywordsnum - 2) / 4) : 1;
	return old_repr;
}

int repr_get_spelling(const syntax *const sx, const size_t index, char32_t *const spelling)
{
	if (sx == NULL || index >= sx->rp)
	{
		return -1;
	}

	size_t i = 2;
	do
	{
		spelling[i] = sx->reprtab[index + i];
		i++;
	} while (sx->reprtab[index + i] != 0);
	return 0;
}

int repr_get_reference(const syntax *const sx, const size_t index)
{
	if (sx == NULL || index >= sx->rp)
	{
		return INT_MAX;
	}

	return sx->reprtab[index + 1];
}

int repr_set_reference(syntax *const sx, const size_t index, const size_t ref)
{
	if (sx == NULL || index >= sx->rp)
	{
		return -1;
	}

	sx->reprtab[index + 1] = (int)ref;
	return 0;
}
