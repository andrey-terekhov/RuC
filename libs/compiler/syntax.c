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

#include "errors.h"


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

	sx.maxdisplg = 3;
	sx.wasmain = 0;

	sx.anstdispl = 0;

	return sx;
}

int is_operator(const int value)
{
	return value == TBegin
		|| value == TEnd
		|| value == TIf
		|| value == TWhile
		|| value == TDo
		|| value == TFor
		|| value == TSwitch
		|| value == TCase
		|| value == TDefault
		|| value == TBreak
		|| value == TContinue
		|| value == TReturnvoid
		|| value == TReturnval
		|| value == TGoto
		|| value == TLabel
		|| value == TPrint
		|| value == TPrintid
		|| value == TPrintf
		|| value == TGetid;
}

int is_declaration(const int value)
{
	return value == TFuncdef
		|| value == TDeclid
		|| value == TStructbeg
		|| value == TStructend
		|| value == TDeclarr
		|| value == TBeginit
		|| value == TStructinit;
}

int is_expression(const int value)
{
	return value == TConstd
		|| value == TStringd
		|| value == TExprend
		|| value == TCondexpr
		|| value == TIdenttovald
		|| value == TAddrtovald
		|| value == TIdenttoaddr
		|| value == TSelect
		|| value == TFunidtoval
		|| value == TIdent
		|| value == TConst
		|| value == TString
		|| value == TSliceident
		|| value == TSlice
		|| value == TIdenttoval
		|| value == TAddrtoval
		|| value == TCall1
		|| value == TCall2;
}

size_t skipper(const syntax *const sx, size_t i)
{
	switch (sx->tree[i++])
	{
		case TCondexpr:
			return skipper(sx, i);
		case TIdenttoaddr:
			return skipper(sx, i + 1);
		case TSelect:
			return skipper(sx, i + 1);
		case TFunidtoval:
			return skipper(sx, i + 1);
		case TIdent:
			return skipper(sx, i + 1);
		case TSliceident:
			return skipper(sx, i + 2);
		case TSlice:
			return skipper(sx, i + 1);
		case TIdenttoval:
			return skipper(sx, i + 1);
		case TAddrtoval:
			return skipper(sx, i);
		case TCall1:
			return skipper(sx, i + 1);
		case TCall2:
			return skipper(sx, i + 1);

		case TConst:
			return skipper(sx, i + 1);
		case TConstd:		// d - double
			return skipper(sx, i + 2);
		case TString:
		{
			int n = sx->tree[i++];
			return skipper(sx, i + n);
		}
		case TStringd:		// d - double
		{
			int n = sx->tree[i++];
			return skipper(sx, i + n * 2);
		}

		case TIdenttovald:	// d - END
			return i + 1;
		case TAddrtovald:	// d - END
			return i;

		case TExprend:
			return i;
	}

	i--;

	if (sx->tree[i] >= 9001 && sx->tree[i] <= 9595)
	{
		return skipper(sx, i + 1);
	}

	if (is_operator(sx->tree[i]) || is_declaration(sx->tree[i]))
	{
		return i;
	}

	printf("skipper: tree[%li] = %i\n", i, sx->tree[i]);
	exit(139);
}

size_t checker(const syntax *const sx, size_t i)
{
	if ((int)i >= sx->tc)
	{
		system_error("i >= sx->tc");
		exit(139);
	}

	if ((int)i == sx->tc - 1)
	{
		if (sx->tree[i++] == TEnd)
		{
			return i;
		}

		system_error("No TEnd");
		exit(139);
	}
	
	switch (sx->tree[i++])
	{
		case TFuncdef:		// Funcdef: 2 потомка (ссылка на identab, тело функции) 
			i += 2;
			return checker(sx, i);
		case TDeclid:		// IdentDecl:  6 потомков (ссылка на identab, тип элемента, размерность, all, usual, выражение-инициализатор (может не быть))
			return i + 7;
		case TDeclarr:		// ArrayDecl: n+2 потомков (размерность массива, n выражений-размеров, инициализатор (может не быть));
		{
			size_t n = sx->tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skipper(sx, i);
			}

			return i;
		}
		case TBeginit:		// ArrayInit: n+1 потомков (размерность инициализатора, n выражений-инициализаторов);
		{
			size_t n = sx->tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				if (sx->tree[i] == TBeginit)
				{
					i = checker(sx, i);
				}
				else
				{
					i = skipper(sx, i);
				}
			}

			if (sx->tree[i] != TExprend)
			{
				system_error("TBeginit need TExprend");
				exit(139);
			}
			return i + 1;
		}
		case TStructbeg:	// StructDecl: n+2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть));
			return i + 1;
		case TStructinit:	// StructInit: n+1 потомков (размерность инициализатора, n выражений-инициализаторов);
			return i + 1;
		case TStructend:
			return i + 1;

		case TBegin:
			while (sx->tree[i] != TEnd)
			{
				i = checker(sx, i);
			}
			return i + 1;

		case TPrint:		// Print: 2 потомка (тип значения, выражение);
			return skipper(sx, i + 1);
		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab);
			return skipper(sx, i + 1);
		case TPrintf:		// Printf: n+2 потомков (форматирующая строка, число параметров, n параметров-выражений);
			return i + 1;
		case TGetid:		// GetID: 1 потомок (ссылка на identab);
							// Scanf: n+2 потомков (форматирующая строка, число параметров, n параметров-ссылок на identab);
			return i + 1;

		case TGoto:			// Goto: 1 потомок (ссылка на identab);
			return i + 1;
		case TContinue:		// Continue: нет потомков;
			return i;
		case TBreak:		// Break: нет потомков;
			return i;
		case TReturnvoid:	// ReturnVoid: нет потомков;
			return i;
		case TReturnval:	// ReturnValue: 2 потомка (тип значения, выражение);
			return skipper(sx, i + 1);
	}

	//i--;
	return skipper(sx, i - 1);

	//printf("checker: tree[%li] = %i\n", i, sx->tree[i]);
	//exit(139);
}


void tree_test(const syntax *const sx)
{
	// Тестирование функций
	size_t i = 0;
	while ((int)i < sx->tc)
	{
		i = checker(sx, i);
	}
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
