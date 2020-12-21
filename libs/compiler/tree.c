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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int is_operator(const int value)
{
	return value == TFuncdef		// Declarations
		|| value == TDeclid
		|| value == TStructbeg
		|| value == TStructend
		|| value == TDeclarr

		|| value == TBegin			// Operators
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
		|| value == TPrintid
		|| value == TPrintf
		|| value == TGetid
		
		|| value == NOP				// Lexemes
		|| value == CREATEDIRECTC;
}

int is_expression(const int value)
{
	return value == TBeginit		// Declarations
		|| value == TStructinit

		|| value == TPrint			// Operator

		|| value == TConstd			// Expressions
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

int is_lexeme(const int value)
{
	return (value >= 9001 && value <= 9595
		&& value != CREATEDIRECTC)
		|| value == ABSIC;
}


size_t skip_expression(const tree *const tree, size_t i, int is_block)
{
	if (tree[i] == NOP && !is_block)
	{
		return i + 1;
	}

	if (is_operator(tree[i]))
	{
		if (!is_block)
		{
			printf("operator: tree[%zi] = %i\n", i, tree[i]);
			exit(139);
		}
		return i;
	}

	if (is_block)
	{
		while (tree[i] != TExprend)
		{
			i = skip_expression(tree, i, 0);
		}

		return i + 1;
	}

	switch (tree[i++])
	{
		// Может иметь несколько потомков
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}
			return i;
		}
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}
			return i;
		}

		case TPrint:		// Print: 2 потомка (тип значения, выражение)
			return i + 1;

		case TCondexpr:
			return i;
		case TIdenttoaddr:
			return i + 1;
		case TSelect:
			return i + 1;
		/*case TFunidtoval:	// Not used
			return i + 1;*/
		case TIdent:
			return skip_expression(tree, i + 1, 1) - 1;	// Может быть общий TExprend

		case TSliceident:
			i = skip_expression(tree, i + 2, 1);			// 2 ветви потомков
			return skip_expression(tree, i, 1) - 1;		// Может быть общий TExprend
		case TSlice:
			i = skip_expression(tree, i + 1, 1);			// 2 ветви потомков
			return skip_expression(tree, i, 1) - 1;		// Может быть общий TExprend

		case TCall1:
			return i + 1;
		case TCall2:
			return skip_expression(tree, i + 1, 1) - 1;	// Может быть общий TExprend

		case TConst:
			return i + 1;
		case TConstd:		// d - double
			return i + 2;
		case TString:
		{
			int n = tree[i++];
			return i + n;
		}
		case TStringd:		// d - double
		{
			int n = tree[i++];
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
			if (is_block)
			{
				printf("TExprend: tree[%zi] = %i\n", i - 1, tree[i - 1]);
				exit(139);
			}
			return i - 1;
	}

	i--;

	if (is_lexeme(tree[i]))
	{
		i += 1;
		while (!is_expression(tree[i]))
		{
			if (is_operator(tree[i]))
			{
				return i;
			}

			i++;
		}

		return i;
	}

	printf("skipper: tree[%zi] = %i\n", i, tree[i]);
	exit(139);
}

size_t skip_operator(const tree *const tree, size_t i)
{
	switch (tree[i++])
	{
		case TFuncdef:		// Funcdef: 2 потомка (ссылка на identab, тело функции)
			i += 2;
			return skip_operator(tree, i);
		case TDeclid:		// IdentDecl: 6 потомков (ссылка на identab, тип элемента, размерность, all, usual, выражение-инициализатор (может не быть))
			return skip_expression(tree, i + 7, 1);
		case TDeclarr:		// ArrayDecl: n + 2 потомков (размерность массива, n выражений-размеров, инициализатор (может не быть))
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}

			return skip_operator(tree, i);
		}
		case TStructbeg:	// StructDecl: n + 2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть))
			i += 1;
			while (tree[i] != TStructend)
			{
				i = skip_operator(tree, i);
			}
			return i + 2;

		case TBegin:
			while (tree[i] != TEnd)
			{
				i = skip_operator(tree, i);
			}
			return i + 1;

		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab)
			return skip_expression(tree, i + 1, 1);
		case TPrintf:		// Printf: n + 2 потомков (форматирующая строка, число параметров, n параметров-выражений)
			return i + 1;
		case TGetid:		// GetID: 1 потомок (ссылка на identab)
							// Scanf: n + 2 потомков (форматирующая строка, число параметров, n параметров-ссылок на identab)
			return i + 1;

		case TIf:			// If: 3 потомка (условие, тело-then, тело-else) - ветка else присутствует не всегда, здесь предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			int is_else = tree[i++];
			i = skip_expression(tree, i, 1);
			i = skip_operator(tree, i);
			return is_else ? skip_operator(tree, is_else) : i;
		}
		case TSwitch:		// Switch: 2 потомка (условие, тело оператора)
			i = skip_expression(tree, i, 1);
			return skip_operator(tree, i);
		case TCase:			// Case: 2 потомка (условие, тело оператора)
			i = skip_expression(tree, i, 1);
			return skip_operator(tree, i);
		case TDefault:		// Default: 1 потомок (тело оператора)
			return skip_operator(tree, i);

		case TWhile:		// While: 2 потомка (условие, тело цикла)
			i = skip_expression(tree, i, 1);
			return skip_operator(tree, i);
		case TDo:			// Do: 2 потомка (тело цикла, условие)
			i = skip_operator(tree, i);
			return skip_expression(tree, i, 1);
		case TFor:			// For: 4 потомка (выражение или объявление, условие окончания, выражение-инкремент, тело цикла); - первые 3 ветки присутствуют не всегда,  здесь также предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			size_t var = tree[i++];
			if (var != 0)
			{
				skip_expression(tree, var, 1);
			}

			size_t cond = tree[i++];
			if (cond != 0)
			{
				skip_expression(tree, cond, 1);
			}

			size_t inc = tree[i++];
			if (inc != 0)
			{
				skip_expression(tree, inc, 1);
			}

			size_t body = tree[i++];
			return skip_operator(tree, body);
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
			return skip_expression(tree, i + 1, 1);

		case NOP:			// NoOperation: 0 потомков
			return i;

		case CREATEDIRECTC:
			while (tree[i] != EXITC)
			{
				i = skip_operator(tree, i);
			}
			return i + 1;
	}

	i--;

	if (!is_expression(tree[i]) && !is_lexeme(tree[i]))
	{
		printf("checker: tree[%zi] = %i\n", i, tree[i]);
	}

	return skip_expression(tree, i, 1);	// CompoundStatement: n + 1 потомков (число потомков, n узлов-операторов)
								// ExpressionStatement: 1 потомок (выражение)
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


node node_get_root(syntax *const sx)
{
	node nd;
	nd.tree = NULL;
	nd.type = SIZE_MAX;

	nd.argv = 0;
	nd.argc = 0;

	nd.children = 0;
	nd.num = 0;

	if (sx == NULL)
	{
		return nd;
	}

	nd.tree = sx->tree;

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
	return nd != NULL ? nd->tree[nd->type] : INT_MAX;
}

int node_get_arg(const node *const nd, const size_t index)
{
	return nd != NULL && index < nd->argc ? nd->tree[nd->argv + index] : INT_MAX;
}


void tree_test(const syntax *const sx)
{
	// Тестирование функций
	size_t i = 0;
	while ((int)i < sx->tc - 1)
	{
		i = skip_operator(sx->tree, i);
	}

	if (sx->tree[i] == TEnd)
	{
		return;
	}

	system_error("No TEnd");
	exit(139);
}
