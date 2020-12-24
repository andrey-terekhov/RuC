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
#include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


node node_expression(tree *const tree, const size_t index);
node node_operator(tree *const tree, const size_t index);


int is_operator(const int value)
{
	return value == TFuncdef		// Declarations
		|| value == TDeclid
		|| value == TDeclarr
		|| value == TStructbeg
		|| value == TStructend

		|| value == TBegin			// Operators
		|| value == TEnd
		|| value == TPrintid
		|| value == TPrintf
		|| value == TGetid
		|| value == TGoto
		|| value == TLabel
		|| value == TIf
		|| value == TFor
		|| value == TDo
		|| value == TWhile
		|| value == TSwitch
		|| value == TCase
		|| value == TDefault
		|| value == TReturnval
		|| value == TReturnvoid
		|| value == TBreak
		|| value == TContinue

		|| value == NOP				// Lexemes
		|| value == CREATEDIRECTC;
}

int is_expression(const int value)
{
	return value == TBeginit		// Declarations
		|| value == TStructinit

		|| value == TPrint			// Operator

		|| value == TCondexpr		// Expressions
		|| value == TSelect
		|| value == TAddrtoval
		|| value == TAddrtovald
		|| value == TIdenttoval
		|| value == TIdenttovald
		|| value == TIdenttoaddr
		|| value == TIdent
		|| value == TConst
		|| value == TConstd
		|| value == TString
		|| value == TStringd
		|| value == TSliceident
		|| value == TSlice
		|| value == TCall1
		|| value == TCall2
		|| value == TExprend;
}

int is_lexeme(const int value)
{
	return (value >= 9001 && value <= 9595
		&& value != CREATEDIRECTC)
		|| value == ABSIC;
}


size_t skip_expression(tree *const tree, size_t i, int is_block)
{
	node nd;
	if (i == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	nd.tree = tree;
	nd.type = i;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	if (tree[i] == NOP && !is_block)
	{
		if (tree[i + 1] != TExprend)
		{
			exit(139);
		}

		nd.children = nd.argv + nd.argc;
		return i + 1;
	}

	if (is_operator(tree[i]))
	{
		error(NULL, tree_expression_not_block, i, tree[i]);
		return SIZE_MAX;
	}

	if (is_block)
	{
		while (i != SIZE_MAX && tree[i] != TExprend)
		{
			i = skip_expression(tree, i, 0);
		}

		return i == SIZE_MAX ? SIZE_MAX : i + 1;
	}

	switch (tree[i++])
	{
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
			nd.argc = 1;
			nd.amount = node_get_arg(&nd, 0);

			i += nd.argc;
			for (size_t j = 0; j < nd.amount; j++)
			{
				i = skip_expression(tree, i, 1);
			}
			break;

		case TAddrtovald:
		case TAddrtoval:

		case TCondexpr:
			break;

		case TConstd:		// d - double
			nd.argc = 2;
			i += nd.argc;
			break;
		case TConst:
		case TSelect:

		case TPrint:		// Print: 2 потомка (тип значения, выражение)

		case TIdenttoaddr:
		case TIdenttovald:
		case TIdenttoval:

		case TCall1:
			nd.argc = 1;
			i += nd.argc;
			break;
		case TCall2:

		case TIdent:
			nd.argc = 1;
			nd.amount = 1;

			i += nd.argc;
			i = skip_expression(tree, i, 1);
			if (i == SIZE_MAX)
			{
				return SIZE_MAX;
			}
			i -= 1;			// Может быть общий TExprend
			break;

		case TStringd:		// d - double
		{
			nd.argc = 1;
			nd.argc += node_get_arg(&nd, 0) * 2;
			i += nd.argc;
		}
		break;
		case TString:
		{
			nd.argc = 1;
			nd.argc += node_get_arg(&nd, 0);
			i += nd.argc;
		}
		break;

		case TSliceident:
			nd.argc = 2;
			nd.amount = 2;

			i += nd.argc;
			i = skip_expression(tree, i, 1);			// 2 ветви потомков
			i = skip_expression(tree, i, 1);
			if (i == SIZE_MAX)
			{
				return SIZE_MAX;
			}
			i -= 1;			// Может быть общий TExprend
			break;
		case TSlice:
			nd.argc = 1;
			nd.amount = 2;

			i += nd.argc;
			i = skip_expression(tree, i, 1);			// 2 ветви потомков
			i = skip_expression(tree, i, 1);
			if (i == SIZE_MAX)
			{
				return SIZE_MAX;
			}
			i -= 1;			// Может быть общий TExprend
			break;

		case TExprend:
			if (is_block)
			{
				error(NULL, tree_expression_texprend, i - 1, tree[i - 1]);
				return SIZE_MAX;
			}
			i -= 1;
			break;

		default:
			if (!is_lexeme(tree[i - 1]))
			{
				error(NULL, tree_expression_unknown, i - 1, tree[i - 1]);
				return SIZE_MAX;
			}

			while (tree[i] != NOP && !is_expression(tree[i]))
			{
				if (is_operator(tree[i]))
				{
					error(NULL, tree_expression_operator, i, tree[i]);
					return SIZE_MAX;
				}

				nd.argc++;
				i++;
			}
	}

	nd.children = nd.argv + nd.argc;
	return i;
}

node node_expression(tree *const tree, const size_t index)
{
	node nd;
	nd.tree = tree;
	nd.type = index;

	nd.tree = NULL;
	return nd;
}


size_t skip_operator(tree *const tree, size_t i)
{
	node nd = node_operator(tree, i);
	if (!node_is_correct(&nd))
	{
		return SIZE_MAX;
	}

	i = nd.children;
	for (size_t j = 0; j < node_get_amount(&nd); j++)
	{
		i = skip_operator(tree, i);
	}

	if (i != SIZE_MAX)
	{
		switch (node_get_type(&nd))
		{
			case TStructbeg:
				i += 2;
				break;
			case TBegin:
			case CREATEDIRECTC:
				i += 1;
				break;
		}
	}

	return i;
}

node node_operator(tree *const tree, const size_t index)
{
	node nd;
	if (index == SIZE_MAX)
	{
		nd.tree = NULL;
		return nd;
	}

	nd.tree = tree;
	nd.type = index;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	switch (tree[index])
	{
		case TFuncdef:		// Funcdef: 2 потомка (ссылка на identab, тело функции)
			nd.argc = 2;
			nd.amount = 1;
			break;
		case TDeclid:		// IdentDecl: 6 потомков (ссылка на identab, тип элемента, размерность, all, usual, выражение-инициализатор (может не быть))
			nd.argc = 7;
			nd.amount = node_get_arg(&nd, 3) ? 1 : 0;	// по all можно определить наличие TExprend
			break;
		case TDeclarr:		// ArrayDecl: n + 2 потомков (размерность массива, n выражений-размеров, инициализатор (может не быть))
		{
			nd.argc = 1;
			nd.amount = node_get_arg(&nd, 0) + 1;
		}
		break;
		case TStructbeg:	// StructDecl: n + 2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть))
		{
			nd.argc = 1;
			size_t j = nd.argv + nd.argc;
			while (j != SIZE_MAX && tree[j] != TStructend)
			{
				j = skip_operator(nd.tree, j);
				nd.amount++;
			}

			if (j == SIZE_MAX)
			{
				nd.tree = NULL;
			}
		}
		break;

		case TBegin:
		{
			size_t j = nd.argv + nd.argc;
			while (j != SIZE_MAX && tree[j] != TEnd)
			{
				j = skip_operator(nd.tree, j);
				nd.amount++;
			}

			if (j == SIZE_MAX)
			{
				nd.tree = NULL;
			}
		}
		break;

		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab)
			nd.argc = 1;
			nd.amount = is_expression(tree[nd.argv + nd.argc]) || is_lexeme(tree[nd.argv + nd.argc]) ? 1 : 0;
			break;
		case TPrintf:		// Printf: n + 2 потомков (форматирующая строка, число параметров, n параметров-выражений)
		case TGetid:		// GetID: 1 потомок (ссылка на identab)
							// Scanf: n + 2 потомков (форматирующая строка, число параметров, n параметров-ссылок на identab)
		
		case TGoto:			// Goto: 1 потомок (ссылка на identab)
			nd.argc = 1;
			break;
		case TLabel:		// LabeledStatement: 2 потомка (ссылка на identab, тело оператора)
			nd.argc = 1;
			nd.amount = 1;
			break;

		case TIf:			// If: 3 потомка (условие, тело-then, тело-else) - ветка else присутствует не всегда, здесь предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
			nd.argc = 1;
			nd.amount = node_get_arg(&nd, 0) != 0 ? 3 : 2;
			break;

		case TFor:			// For: 4 потомка (выражение или объявление, условие окончания, выражение-инкремент, тело цикла); - первые 3 ветки присутствуют не всегда,  здесь также предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
			nd.argc = 4;
			nd.amount = 1;

			for (size_t j = 0; j < 3; j++)
			{
				if (node_get_arg(&nd, j) != 0)
				{
					nd.amount++;
				}
			}
			break;
		case TDo:			// Do: 2 потомка (тело цикла, условие)
			nd.amount = 2;
			break;
		case TWhile:		// While: 2 потомка (условие, тело цикла)

		case TSwitch:		// Switch: 2 потомка (условие, тело оператора)
		case TCase:			// Case: 2 потомка (условие, тело оператора)
			nd.amount = 2;
			break;
		case TDefault:		// Default: 1 потомок (тело оператора)
			nd.amount = 1;
			break;

		case TReturnval:	// ReturnValue: 2 потомка (тип значения, выражение)
			nd.argc = 1;
			nd.amount = 1;
			break;
		case TReturnvoid:	// ReturnVoid: нет потомков
		case TBreak:		// Break: нет потомков
		case TContinue:		// Continue: нет потомков

		case NOP:			// NoOperation: 0 потомков
			break;
		case CREATEDIRECTC:
		{
			size_t j = nd.argv + nd.argc;
			while (j != SIZE_MAX && tree[j] != EXITC)
			{
				j = skip_operator(nd.tree, j);
				nd.amount++;
			}

			if (j == SIZE_MAX)
			{
				nd.tree = NULL;
			}
		}
		break;

		default:
			if (!is_expression(tree[nd.type]) && !is_lexeme(tree[nd.type]))
			{
				warning(NULL, tree_operator_unknown, nd.type, tree[nd.type]);
			}

			nd.type = skip_expression(tree, nd.type, 1) - 1;	// CompoundStatement: n + 1 потомков (число потомков, n узлов-операторов)
																// ExpressionStatement: 1 потомок (выражение)
			nd.argv = nd.type + 1;
	}

	nd.children = nd.argv + nd.argc;
	return nd;
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
	if (sx == NULL)
	{
		nd.tree = NULL;
		return nd;
	}

	nd.tree = sx->tree;
	nd.type = SIZE_MAX;
	nd.argv = 0;
	nd.argc = 0;
	nd.children = 0;
	nd.amount = 0;

	size_t i = 0;
	while (i != SIZE_MAX && nd.tree[i] != TEnd)
	{
		i = skip_operator(nd.tree, i);
		nd.amount++;
	}

	if (i == SIZE_MAX)
	{
		nd.tree = NULL;
	}

	return nd;
}

node node_get_child(node *const nd, const size_t index)
{
	if (!node_is_correct(nd) || index > nd->amount)
	{
		node child;
		child.tree = NULL;
		return child;
	}

	size_t i = nd->children;
	for (size_t num = 0; num < index; num++)
	{
		if (nd->type == SIZE_MAX || node_get_type(nd))
		{
			i = skip_operator(nd->tree, i);
		}
		else
		{
			i = skip_expression(nd->tree, i, 1);
		}
	}

	return node_operator(nd->tree, i);
}


size_t node_get_amount(const node *const nd)
{
	return node_is_correct(nd) ? nd->amount : 0;
}

int node_get_type(const node *const nd)
{
	return node_is_correct(nd) ? nd->tree[nd->type] : INT_MAX;
}

int node_get_arg(const node *const nd, const size_t index)
{
	return node_is_correct(nd) && index < nd->argc ? nd->tree[nd->argv + index] : INT_MAX;
}


int node_is_correct(const node *const nd)
{
	return nd != NULL && nd->tree != NULL;
}


int tree_test(syntax *const sx)
{
	// Тестирование функций
	size_t i = 0;
	while (i != SIZE_MAX && (int)i < sx->tc - 1)
	{
		i = skip_operator(sx->tree, i);
	}

	if (i == SIZE_MAX)
	{
		return -1;
	}

	if (sx->tree[i] == TEnd)
	{
		return 0;
	}

	error(NULL, tree_no_tend);
	return -1;
}