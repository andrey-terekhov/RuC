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
#include "errors.h"
#include "tree.h"


int get_static(syntax *const sx, const int type)
{
	const int old_displ = sx->displ;
	sx->displ += sx->lg * size_of(sx, type);

	if (sx->lg > 0)
	{
		sx->maxdispl = sx->displ > sx->maxdispl ? sx->displ : sx->maxdispl;
	}
	else
	{
		sx->maxdisplg = -sx->displ;
	}

	return old_displ;
}

/**	Check if modes are equal */
int mode_is_equal(const syntax *const sx, const size_t first, const size_t second)
{
	if (sx->modetab[first] != sx->modetab[second])
	{
		return 0;
	}

	size_t length = 1;
	const int mode = sx->modetab[first];
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


int sx_init(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	sx->memory = vector_create(MAXMEMSIZE);
	vector_increase(&sx->memory, 4);
	sx->processes = vector_create(INIPROSIZE);
	vector_increase(&sx->processes, 1);
	sx->stack = vector_create(256);

	sx->predef = vector_create(FUNCSIZE);
	sx->functions = vector_create(FUNCSIZE);
	vector_increase(&sx->functions, 2);
	
	sx->tree = vector_create(MAXTREESIZE);

	sx->id = 2;
	sx->md = 1;
	sx->startmode = 1;
	sx->rp = 1;

	sx->maxdisplg = 3;
	sx->ref_main = 0;

	sx->maxdispl = 3;
	sx->displ = -3;
	sx->curid = 2;
	sx->lg = -1;

	for (size_t i = 0; i < 256; i++)
	{
		sx->hashtab[i] = 0;
	}

	sx->max_threads = 0;

	return 0;
}

int sx_is_correct(syntax *const sx)
{
	int is_correct = 1;
	if (sx->ref_main == 0)
	{
		error(NULL, no_main_in_program);
		is_correct = 0;
	}

	for (size_t i = 0; i < vector_size(&sx->predef); i++)
	{
		if (vector_get(&sx->predef, i))
		{
			char buffer[MAXSTRINGL];
			repr_get_ident(sx, (size_t)vector_get(&sx->predef, i), buffer);
			error(NULL, predef_but_notdef, buffer);
			is_correct = 0;
		}
	}

	return is_correct;
}

int sx_clear(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	vector_clear(&sx->memory);
	vector_clear(&sx->processes);
	vector_clear(&sx->stack);

	vector_clear(&sx->predef);
	vector_clear(&sx->functions);

	vector_clear(&sx->tree);

	return 0;
}


int mem_increase(syntax *const sx, const size_t size)
{
	return sx != NULL ? vector_increase(&sx->memory, size) : -1;
}

int mem_add(syntax *const sx, const item_t value)
{
	return sx != NULL ? vector_add(&sx->memory, value) != SIZE_MAX ? 0 : -1 : -1;
}

int mem_set(syntax *const sx, const size_t index, const item_t value)
{
	return sx != NULL ? vector_set(&sx->memory, index, value) : -1;
}

item_t mem_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->memory, index) : ITEM_MAX;
}

size_t mem_size(const syntax *const sx)
{
	return sx != NULL ? vector_size(&sx->memory) : SIZE_MAX;
}


int proc_set(syntax *const sx, const size_t index, const item_t value)
{
	return sx != NULL ? vector_set(&sx->processes, index, value) : -1;
}

item_t proc_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->processes, index) : ITEM_MAX;
}


int stack_push(syntax *const sx, const item_t value)
{
	return sx != NULL ? vector_add(&sx->stack, value) != SIZE_MAX ? 0 : -1 : -1;
}

item_t stack_pop(syntax *const sx)
{
	return sx != NULL ? vector_remove(&sx->stack) : ITEM_MAX;
}


int func_add(syntax *const sx, const item_t ref)
{
	return sx != NULL ? vector_add(&sx->functions, ref) != SIZE_MAX ? 0 : -1 : -1;
}

int func_set(syntax *const sx, const size_t index, const item_t ref)
{
	return sx != NULL ? vector_set(&sx->functions, index, ref) : -1;
}

item_t func_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->functions, index) : ITEM_MAX;
}


size_t ident_add(syntax *const sx, const size_t repr, const int type, const int mode, const int func_def)
{
	const size_t lastid = sx->id;
	sx->identab[lastid] = repr_get_reference(sx, repr);
	sx->id += 4;

	if (repr_get_reference(sx, repr) == 0) // это может быть только MAIN
	{
		if (sx->ref_main)
		{
			return SIZE_MAX;
		}
		sx->ref_main = lastid;
	}

	// Ссылка на описание с таким же представлением в предыдущем блоке
	const size_t prev = sx->identab[lastid];
	if (prev)
	{
		// prev == 0 только для main, эту ссылку портить нельзя
		// иначе это ссылка на текущее описание с этим представлением
		repr_set_reference(sx, repr, lastid);
	}

	// Один и тот же идентификатор м.б. переменной и меткой
	if (type != 1 && prev >= sx->curid && (func_def != 1 || ident_get_repr(sx, prev) > 0))
	{
		// Только определение функции может иметь 2 описания, то есть иметь предописание
		return SIZE_MAX - 1;
	}

	ident_set_repr(sx, lastid, repr);
	ident_set_mode(sx, lastid, mode);

	if (type < 0)
	{
		// Так как < 0, это функция-параметр
		ident_set_displ(sx, lastid, -(sx->displ++));
		sx->maxdispl = sx->displ;
	}
	else if (type == 0)
	{
		ident_set_displ(sx, lastid, get_static(sx, mode));
	}
	else if (type == 1)
	{
		// Это метка
		// В поле mode: 0, если первым встретился goto; когда встретим метку, поставим 1
		// В поле displ: при генерации кода, когда встретим метку, поставим pc
		ident_set_mode(sx, lastid, 0);
		ident_set_displ(sx, lastid, 0);
	}
	else if (type >= 1000)
	{
		// Это описание типа, а (type-1000) – это номер инициирующей процедуры
		ident_set_displ(sx, lastid, type);
	}
	else if (type > 1 && type < 1000)
	{
		// Это функция, и в поле displ находится её номер
		ident_set_displ(sx, lastid, type);

		if (func_def == 2)
		{
			// Это предописание функции
			sx->identab[lastid + 1] = -ident_get_repr(sx, lastid);
			vector_add(&sx->predef, (item_t)repr);
		}
		else
		{
			// Это описание функции
			for (size_t i = 0; i < vector_size(&sx->predef); i++)
			{
				if ((size_t)vector_get(&sx->predef, i) == repr)
				{
					vector_set(&sx->predef, i, 0);
				}
			}
		}
	}
	return lastid;
}

int ident_get_repr(const syntax *const sx, const size_t index)
{
	if (sx == NULL || index >= sx->id)
	{
		return INT_MAX;
	}

	return sx->identab[index + 1];
}

int ident_get_mode(const syntax *const sx, const size_t index)
{
	if (sx == NULL || index >= sx->id)
	{
		return INT_MAX;
	}

	return sx->identab[index + 2];
}

int ident_get_displ(const syntax *const sx, const size_t index)
{
	if (sx == NULL || index >= sx->id)
	{
		return INT_MAX;
	}

	return sx->identab[index + 3];
}

int ident_set_repr(syntax *const sx, const size_t index, const size_t repr)
{
	if (sx == NULL || index >= sx->id)
	{
		return -1;
	}

	sx->identab[index + 1] = (int)repr;
	return 0;
}

int ident_set_mode(syntax *const sx, const size_t index, const int mode)
{
	if (sx == NULL || index >= sx->id)
	{
		return -1;
	}

	sx->identab[index + 2] = mode;
	return 0;
}

int ident_set_displ(syntax *const sx, const size_t index, const int displ)
{
	if (sx == NULL || index >= sx->id)
	{
		return -1;
	}

	sx->identab[index + 3] = displ;
	return 0;
}


int size_of(const syntax *const sx, const int mode)
{
	return mode == LFLOAT ? 2 : (mode > 0 && mode_get(sx, mode) == MSTRUCT) ? mode_get(sx, mode + 1) : 1;
}

size_t mode_add(syntax *const sx, const int *const record, const size_t size)
{
	if (sx == NULL || record == NULL)
	{
		return SIZE_MAX;
	}

	sx->modetab[sx->md] = (int) sx->startmode;
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
	if (sx == NULL || index >= sx->md)
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
		hash += (spelling[i] & 255);
		sx->reprtab[sx->rp++] = spelling[i];
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

size_t repr_get_ident(const syntax *const sx, const size_t index, char *const buffer)
{
	if (sx == NULL || index >= sx->rp)
	{
		return SIZE_MAX;
	}

	size_t i = 0;
	size_t pos = index + 2; // ссылка на reprtab
	while (sx->reprtab[pos] != '\0')
	{
		i += utf8_to_string(buffer, sx->reprtab[pos++]);
	}

	return i;
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


int scope_block_enter(syntax *const sx, int *const displ, int *const lg)
{
	if (sx == NULL || displ == NULL || lg == NULL)
	{
		return -1;
	}

	sx->curid = sx->id;
	*displ = sx->displ;
	*lg = sx->lg;
	return 0;
}

int scope_block_exit(syntax *const sx, const int displ, const int lg)
{
	if (sx == NULL)
	{
		return -1;
	}

	for (size_t i = sx->id - 4; i >= sx->curid; i -= 4)
	{
		repr_set_reference(sx, ident_get_repr(sx, i), sx->identab[i]);
	}

	sx->displ = displ;
	sx->lg = lg;
	return 0;
}

int scope_func_enter(syntax *const sx)
{
	if (sx == NULL)
	{
		return INT_MAX;
	}

	const int displ = sx->displ;
	sx->curid = sx->id;
	sx->displ = 3;
	sx->maxdispl = 3;
	sx->lg = 1;
	
	return displ;
}

int scope_func_exit(syntax *const sx, const size_t decl_ref, const int displ)
{
	if (sx == NULL || decl_ref >= vector_size(&sx->tree))
	{
		return -1;
	}

	for (size_t i = sx->id - 4; i >= sx->curid; i -= 4)
	{
		repr_set_reference(sx, ident_get_repr(sx, i), sx->identab[i]);
	}

	sx->curid = 2;	// Все функции описываются на одном уровне
	vector_set(&sx->tree, decl_ref, sx->maxdispl);
	sx->lg = -1;
	sx->displ = displ;
	
	return 0;
}
