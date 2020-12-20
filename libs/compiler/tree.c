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

#include "tree.h"
#include "errors.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


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
		|| value == TGetid
		|| value == NOP
		|| value == CREATEDIRECTC;
}

int is_declaration(const int value)
{
	return value == TFuncdef
		|| value == TDeclid
		|| value == TStructbeg
		|| value == TStructend
		|| value == TDeclarr;
}

int is_expression(const int value)
{
	return value == TBeginit
		|| value == TStructinit
		|| value == TConstd
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


size_t skipper(const syntax *const sx, size_t i, int from_checker)
{
	if (sx->tree[i] == TPrint && !from_checker)
	{
		return i + 2;
	}

	if (sx->tree[i] == NOP && !from_checker)
	{
		return i + 1;
	}

	if ((is_operator(sx->tree[i]) || is_declaration(sx->tree[i]))
		&& sx->tree[i] != TPrint)
	{
		if (!from_checker)
		{
			printf("operator: tree[%zi] = %i\n", i, sx->tree[i]);
			exit(139);
		}
		return i;
	}

	if (from_checker)
	{
		while (sx->tree[i] != TExprend)
		{
			i = skipper(sx, i, 0);
		}

		return i + 1;
	}

	switch (sx->tree[i++])
	{
		// Может иметь несколько потомков
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = sx->tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skipper(sx, i, 1);
			}
			return i;
		}
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = sx->tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skipper(sx, i, 1);
			}
			return i;
		}

		case TCondexpr:
			return i;
		case TIdenttoaddr:
			return i + 1;
		case TSelect:
			return i + 1;
		/*case TFunidtoval:	// Not used
			return i + 1;*/
		case TIdent:
			return skipper(sx, i + 1, 1) - 1;	// Может быть общий TExprend

		case TSliceident:
			i = skipper(sx, i + 2, 1);			// 2 ветви потомков
			return skipper(sx, i, 1) - 1;		// Может быть общий TExprend
		case TSlice:
			i = skipper(sx, i + 1, 1);			// 2 ветви потомков
			return skipper(sx, i, 1) - 1;		// Может быть общий TExprend

		case TCall1:
			return i + 1;
		case TCall2:
			return skipper(sx, i + 1, 1) - 1;	// Может быть общий TExprend

		case TConst:
			return i + 1;
		case TConstd:		// d - double
			return i + 2;
		case TString:
		{
			int n = sx->tree[i++];
			return i + n;
		}
		case TStringd:		// d - double
		{
			int n = sx->tree[i++];
			return i + n * 2;
		}

		case TIdenttoval:
			return i + 1;
		case TIdenttovald:	// d - WTF?!
			return i + 1;

		case TAddrtoval:
			return i;
		case TAddrtovald:	// d - END
			return i;

		case TExprend:
			if (from_checker)
			{
				printf("TExprend: tree[%zi] = %i\n", i - 1, sx->tree[i - 1]);
				exit(139);
			}
			return i - 1;
	}

	i--;

	if ((sx->tree[i] >= 9001 && sx->tree[i] <= 9595) || sx->tree[i] == 9651)
	{
		i += 1;
		while (!is_expression(sx->tree[i]))
		{
			if (is_operator(sx->tree[i]) || is_declaration(sx->tree[i]))
			{
				return i;
			}

			i++;
		}

		return i;
	}

	printf("skipper: tree[%zi] = %i\n", i, sx->tree[i]);
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
		case TDeclid:		// IdentDecl: 6 потомков (ссылка на identab, тип элемента, размерность, all, usual, выражение-инициализатор (может не быть))
			return skipper(sx, i + 7, 1);
		case TDeclarr:		// ArrayDecl: n + 2 потомков (размерность массива, n выражений-размеров, инициализатор (может не быть))
		{
			size_t n = sx->tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skipper(sx, i, 1);
			}

			return checker(sx, i);
		}
		case TStructbeg:	// StructDecl: n + 2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть))
			i += 1;
			while (sx->tree[i] != TStructend)
			{
				i = checker(sx, i);
			}
			return i + 2;

		case TBegin:
			while (sx->tree[i] != TEnd)
			{
				i = checker(sx, i);
			}
			return i + 1;

		case TPrint:		// Print: 2 потомка (тип значения, выражение)
			system_warning("TPrint call");
			exit(139);

			/*i += 1;
			if (sx->tree[i] != TExprend)
			{
				system_error("TPrint need TExprend");
				exit(139);
			}
			return i + 1;*/
		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab)
			return skipper(sx, i + 1, 1);
		case TPrintf:		// Printf: n + 2 потомков (форматирующая строка, число параметров, n параметров-выражений)
			return i + 1;
		case TGetid:		// GetID: 1 потомок (ссылка на identab)
							// Scanf: n + 2 потомков (форматирующая строка, число параметров, n параметров-ссылок на identab)
			return i + 1;

		case TIf:			// If: 3 потомка (условие, тело-then, тело-else) - ветка else присутствует не всегда, здесь предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			int is_else = sx->tree[i++];
			i = skipper(sx, i, 1);
			i = checker(sx, i);
			return is_else ? checker(sx, is_else) : i;
		}
		case TSwitch:		// Switch: 2 потомка (условие, тело оператора)
			i = skipper(sx, i, 1);
			return checker(sx, i);
		case TCase:			// Case: 2 потомка (условие, тело оператора)
			i = skipper(sx, i, 1);
			return checker(sx, i);
		case TDefault:		// Default: 1 потомок (тело оператора)
			return checker(sx, i);

		case TWhile:		// While: 2 потомка (условие, тело цикла)
			i = skipper(sx, i, 1);
			return checker(sx, i);
		case TDo:			// Do: 2 потомка (тело цикла, условие)
			i = checker(sx, i);
			return skipper(sx, i, 1);
		case TFor:			// For: 4 потомка (выражение или объявление, условие окончания, выражение-инкремент, тело цикла); - первые 3 ветки присутствуют не всегда,  здесь также предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			size_t var = sx->tree[i++];
			if (var != 0)
			{
				skipper(sx, var, 1);
			}

			size_t cond = sx->tree[i++];
			if (cond != 0)
			{
				skipper(sx, cond, 1);
			}

			size_t inc = sx->tree[i++];
			if (inc != 0)
			{
				skipper(sx, inc, 1);
			}

			size_t body = sx->tree[i++];
			return checker(sx, body);
		}

		case TLabel:		// LabeledStatement: 2 потомка (ссылка на identab, тело оператора)
			return i + 1;
		case TGoto:			// Goto: 1 потомок (ссылка на identab)
			return i + 1;

		case TContinue:		// Continue: нет потомков
			return i;
		case TBreak:		// Break: нет потомков
			return i;
		case TReturnvoid:	// ReturnVoid: нет потомков
			return i;
		case TReturnval:	// ReturnValue: 2 потомка (тип значения, выражение)
			return skipper(sx, i + 1, 1);

		case NOP:			// NoOperation: 0 потомков
			return i;

		case CREATEDIRECTC:
			while (sx->tree[i] != EXITC)
			{
				i = checker(sx, i);
			}
			return i + 1;
	}

	i--;

	if (!is_expression(sx->tree[i])
		&& !((sx->tree[i] >= 9001 && sx->tree[i] <= 9595) || sx->tree[i] == 9651))
	{
		printf("checker: tree[%zi] = %i\n", i, sx->tree[i]);
	}

	return skipper(sx, i, 1);	// CompoundStatement: n + 1 потомков (число потомков, n узлов-операторов)
								// ExpressionStatement: 1 потомок (выражение)
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


node node_get_root(const syntax *const sx)
{
	node nd;
	nd.ref = (tree *)&sx->tree;

	return nd;
}

int node_get_child(const node *const nd, const size_t index, node *const child)
{
	if (nd == NULL || index > nd->num || child == NULL)
	{
		return -1;
	}

	return 0;
}


int node_get_type(const node *const nd)
{
	return nd != NULL ? *((int *)nd->type) : INT_MAX;
}

int node_get_arg(const node *const nd, const size_t index)
{
	return nd != NULL && index < nd->argc ? ((int *)nd->argv)[index] : INT_MAX;
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
