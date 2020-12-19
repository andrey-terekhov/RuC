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
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


int getstatic(syntax *const sx, int type)
{
	int olddispl = sx->displ;
	sx->displ += sx->lg * sz_of(sx, type); // lg - смещение от l (+1) или от g (-1)
	if (sx->lg > 0)
	{
		sx->maxdispl = (sx->displ > sx->maxdispl) ? sx->displ : sx->maxdispl;
	}
	else
	{
		sx->maxdisplg = -sx->displ;
	}
	return olddispl;
}


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
	sx.repr = 0;

	sx.wasmain = 0;

	sx.anstdispl = 0;
	sx.maxdisplg = 3;
	sx.maxdispl = 3;
	sx.displ = -3;
	sx.curid = 2;
	sx.lg = -1;
	
	sx.prdf = -1;

	return sx;
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

int func_get(syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->funcnum)
	{
		return INT_MAX;
	}

	return sx->functions[index];
}


int ident_add(syntax *const sx, int f, int type, int func_def)
{
	// f =  0, если не ф-ция, f=1, если метка, f=funcnum,
	// если описание ф-ции,
	// f = -1, если ф-ция-параметр, f>=1000, если это описание типа
	// f = -2, #define
	int pred;
	int lastid = sx->id;
	if (sx->reprtab[sx->repr + 1] == 0) // это может быть только MAIN
	{
		if (sx->wasmain)
		{
			return -1;
		}
		sx->wasmain = sx->id;
	}
	
	// ссылка на описание с таким же
	// представлением в предыдущем блоке
	pred = sx->identab[sx->id] = sx->reprtab[sx->repr + 1];
	if (pred)
	{
		// pred == 0 только для main, эту ссылку портить нельзя
		// ссылка на текущее описание с этим представлением
		// (это в reprtab)
		sx->reprtab[sx->repr + 1] = sx->id;
	}
	
	if (f != 1 && pred >= sx->curid) // один  и тот же идент м.б. переменной и меткой
	{
		if (func_def == 3 ? 1 : sx->identab[pred + 1] > 0 ? 1 : func_def == 1 ? 0 : 1)
		{
			return -2;	// 1
						// только определение функции может иметь 2
						// описания, т.е. иметь предописание
		}
	}
	
	sx->identab[sx->id + 1] = sx->repr; // ссылка на представление
	if (f == -2)									 // #define
	{
		sx->identab[sx->id + 2] = 1;
		sx->identab[sx->id + 3] = type; // это целое число, определенное по #define
	}
	else // дальше тип или ссылка на modetab (для функций и структур)
	{
		sx->identab[sx->id + 2] = type; // тип -1 int, -2 char, -3 float, -4 long, -5 double,
														  // если тип > 0, то это ссылка на modetab
		if (f == 1)
		{
			sx->identab[sx->id + 2] = 0; // 0, если первым встретился goto, когда встретим метку,
														   // поставим 1
			sx->identab[sx->id + 3] = 0; // при генерации кода когда встретим метку, поставим pc
		}
		else if (f >= 1000)
		{
			sx->identab[sx->id + 3] = f; // это описание типа, если f > 1000, то f-1000 - это номер
														   // иниц проц
		}
		else if (f)
		{
			if (f < 0)
			{
				sx->identab[sx->id + 3] = -(sx->displ++);
				sx->maxdispl = sx->displ;
			}
			else // identtab[context->lastid+3] - номер функции, если < 0, то
				 // это функция-параметр
			{
				sx->identab[sx->id + 3] = f;
				if (func_def == 2)
				{
					sx->identab[lastid + 1] *= -1; //это предописание
					sx->predef[++sx->prdf] = sx->repr;
				}
				else
				{
					int i;
					
					for (i = 0; i <= sx->prdf; i++)
					{
						if (sx->predef[i] == sx->repr)
						{
							sx->predef[i] = 0;
						}
					}
				}
			}
		}
		else
		{
			sx->identab[sx->id + 3] = getstatic(sx, type);
		}
	}
	sx->id += 4;
	return lastid;
}

int ident_get_mode(syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->id)
	{
		return INT_MAX;
	}
	
	return sx->identab[index + 2];
}

int ident_set_mode(syntax *const sx, const size_t index, const int mode)
{
	if (sx == NULL || (int)index >= sx->id)
	{
		return -1;
	}
	
	sx->identab[index + 2] = mode;
	return 0;
}

int ident_get_displ(syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->id)
	{
		return INT_MAX;
	}
	
	return sx->identab[index + 3];
}

int ident_set_displ(syntax *const sx, const size_t index, const int displ)
{
	if (sx == NULL || (int)index >= sx->id)
	{
		return -1;
	}
	
	sx->identab[index + 3] = displ;
	return 0;
}

int sz_of(syntax *const sx, int type)
{
	return type == LFLOAT ? 2 : (type > 0 && mode_get(sx, type) == MSTRUCT) ? mode_get(sx, type + 1) : 1;
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

int mode_get(syntax *const sx, const size_t index)
{
	if (sx == NULL || (int)index >= sx->md)
	{
		return INT_MAX;
	}

	return sx->modetab[index];
}
