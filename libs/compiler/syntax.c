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

	sx.maxdisplg = 3;
	sx.wasmain = 0;

	sx.anstdispl = 0;

	return sx;
}


int equal_modes(syntax *const sx, int first_mode, int second_mode)
{
	int record_length;
	int i;
	int flag = 1;
	int mode;
	
	if (sx->modetab[first_mode] != sx->modetab[second_mode])
	{
		return 0;
	}
	
	mode = sx->modetab[first_mode];
	// Определяем, сколько полей надо сравнивать для различных типов записей
	if (mode == MSTRUCT || mode == MFUNCTION)
		record_length = 2 + sx->modetab[first_mode + 2];
	else
		record_length = 1;
	
	for (i = 1; i <= record_length && flag; i++)
	{
		if (sx->modetab[first_mode + i] != sx->modetab[second_mode + i])
		{
			return 0;
		}
	}
	
	return 1;
}


int check_mode_duplicates(syntax *const sx)
{
	// Проверяет, имеется ли в modetab только что внесенный тип
	// Если да, то возвращает ссылку на старую запись, иначе - на новую
	
	int old = sx->modetab[sx->startmode];
	
	while (old)
	{
		if (equal_modes(sx, sx->startmode + 1, old + 1))
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


int modetab_add(syntax *const sx, const int size, const int new_record[])
{
	sx->modetab[sx->md] = sx->startmode;
	sx->startmode = sx->md++;
	for (int i = 0; i < size; i++)
	{
		sx->modetab[sx->md++] = new_record[i];
	}
	return check_mode_duplicates(sx);
}

int modetab_get(syntax *const sx, const int index)
{
	return sx->modetab[index];
}


int newdecl(syntax *const sx, const int type, const int elemtype)
{
	sx->modetab[sx->md] = sx->startmode;
	sx->startmode = sx->md++;
	sx->modetab[sx->md++] = type;
	sx->modetab[sx->md++] = elemtype; // ссылка на элемент
	
	return check_mode_duplicates(sx);
}
