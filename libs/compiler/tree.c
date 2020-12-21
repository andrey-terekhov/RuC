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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


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
		|| value == TIf
		|| value == TSwitch
		|| value == TCase
		|| value == TDefault
		|| value == TWhile
		|| value == TDo
		|| value == TFor
		|| value == TGoto
		|| value == TLabel
		|| value == TContinue
		|| value == TBreak
		|| value == TReturnvoid
		|| value == TReturnval
		
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


size_t skip_expression(const tree *const tree, size_t i, int is_block)
{
	if (i == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	if (tree[i] == NOP && !is_block)
	{
		return i + 1;
	}

	if (is_operator(tree[i]))
	{
		if (!is_block)
		{
			error(NULL, tree_expression_not_block, i, tree[i]);
			return SIZE_MAX;
		}
		return i;
	}

	if (is_block)
	{
		while (tree[i] != TExprend)
		{
			i = skip_expression(tree, i, 0);
		}

		return i == SIZE_MAX ? SIZE_MAX : i + 1;
	}

	switch (tree[i++])
	{
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}
			return i == SIZE_MAX ? SIZE_MAX : i;
		}
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}
			return i == SIZE_MAX ? SIZE_MAX : i;
		}

		case TPrint:		// Print: 2 потомка (тип значения, выражение)
			return i + 1;

		case TCondexpr:
			return i;
		case TSelect:
			return i + 1;

		case TAddrtoval:
			return i;
		case TAddrtovald:
			return i;

		case TIdenttoval:
			return i + 1;
		case TIdenttovald:
			return i + 1;

		case TIdenttoaddr:
			return i + 1;
		case TIdent:
			i = skip_expression(tree, i + 1, 1);	// Может быть общий TExprend
			return i == SIZE_MAX ? SIZE_MAX : i - 1;

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

		case TSliceident:
			i = skip_expression(tree, i + 2, 1);			// 2 ветви потомков
			i = skip_expression(tree, i, 1);
			return i == SIZE_MAX ? SIZE_MAX : i - 1;		// Может быть общий TExprend
		case TSlice:
			i = skip_expression(tree, i + 1, 1);			// 2 ветви потомков
			i = skip_expression(tree, i, 1);
			return i == SIZE_MAX ? SIZE_MAX : i - 1;		// Может быть общий TExprend

		case TCall1:
			return i + 1;
		case TCall2:
			i = skip_expression(tree, i + 1, 1);
			return i == SIZE_MAX ? SIZE_MAX : i - 1;		// Может быть общий TExprend

		case TExprend:
			if (is_block)
			{
				error(NULL, tree_expression_texprend, i - 1, tree[i - 1]);
				return SIZE_MAX;
			}
			return i - 1;
	}

	if (is_lexeme(tree[i - 1]))
	{
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

	error(NULL, tree_expression_unknown, i - 1, tree[i - 1]);
	return SIZE_MAX;
}
size_t skip_operator(tree *const tree, size_t i);
size_t node_operator(tree *const tree, size_t i, node *const nd)
{
	if (i == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	switch (tree[i++])
	{
		case TFuncdef:		// Funcdef: 2 потомка (ссылка на identab, тело функции)
			i = skip_operator(tree, i + 2);
			break;
		case TDeclid:		// IdentDecl: 6 потомков (ссылка на identab, тип элемента, размерность, all, usual, выражение-инициализатор (может не быть))
			i = skip_expression(tree, i + 7, 1);
			break;
		case TDeclarr:		// ArrayDecl: n + 2 потомков (размерность массива, n выражений-размеров, инициализатор (может не быть))
		{
			size_t n = tree[i++];
			for (size_t j = 0; j < n; j++)
			{
				i = skip_expression(tree, i, 1);
			}

			i = skip_operator(tree, i);
		}
		break;
		case TStructbeg:	// StructDecl: n + 2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть))
			i += 1;
			while (tree[i] != TStructend)
			{
				i = skip_operator(tree, i);
			}
			i = i == SIZE_MAX ? SIZE_MAX : i + 2;
			break;

		case TBegin:
			while (tree[i] != TEnd)
			{
				i = skip_operator(tree, i);
			}
			i = i == SIZE_MAX ? SIZE_MAX : i + 1;
			break;

		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab)
			i = skip_expression(tree, i + 1, 1);
			break;
		case TPrintf:		// Printf: n + 2 потомков (форматирующая строка, число параметров, n параметров-выражений)
			i += 1;
			break;
		case TGetid:		// GetID: 1 потомок (ссылка на identab)
							// Scanf: n + 2 потомков (форматирующая строка, число параметров, n параметров-ссылок на identab)
			i += 1;
			break;

		case TIf:			// If: 3 потомка (условие, тело-then, тело-else) - ветка else присутствует не всегда, здесь предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			int is_else = tree[i++];
			i = skip_expression(tree, i, 1);
			i = skip_operator(tree, i);
			if (is_else && i != SIZE_MAX)
			{
				i = skip_operator(tree, is_else);
			}
		}
		break;
		case TSwitch:		// Switch: 2 потомка (условие, тело оператора)
			i = skip_expression(tree, i, 1);
			i = skip_operator(tree, i);
			break;
		case TCase:			// Case: 2 потомка (условие, тело оператора)
			i = skip_expression(tree, i, 1);
			i = skip_operator(tree, i);
			break;
		case TDefault:		// Default: 1 потомок (тело оператора)
			i = skip_operator(tree, i);
			break;

		case TWhile:		// While: 2 потомка (условие, тело цикла)
			i = skip_expression(tree, i, 1);
			i = skip_operator(tree, i);
			break;
		case TDo:			// Do: 2 потомка (тело цикла, условие)
			i = skip_operator(tree, i);
			i = skip_expression(tree, i, 1);
			break;
		case TFor:			// For: 4 потомка (выражение или объявление, условие окончания, выражение-инкремент, тело цикла); - первые 3 ветки присутствуют не всегда,  здесь также предлагается не добавлять лишних узлов-индикаторов, а просто проверять, указывает на 0 или нет
		{
			size_t var = tree[i++];
			if (var != 0)
			{
				if (skip_expression(tree, var, 1) == SIZE_MAX)
				{
					return SIZE_MAX;
				}
			}

			size_t cond = tree[i++];
			if (cond != 0)
			{
				if (skip_expression(tree, cond, 1) == SIZE_MAX)
				{
					return SIZE_MAX;
				}
			}

			size_t inc = tree[i++];
			if (inc != 0)
			{
				if (skip_expression(tree, inc, 1) == SIZE_MAX)
				{
					return SIZE_MAX;
				}
			}

			size_t body = tree[i++];
			i = skip_operator(tree, body);
		}
		break;

		case TGoto:			// Goto: 1 потомок (ссылка на identab)
			i += 1;
			break;
		case TLabel:		// LabeledStatement: 2 потомка (ссылка на identab, тело оператора)
			i += 1;
			break;

		case TContinue:		// Continue: нет потомков
		case TBreak:		// Break: нет потомков
		case TReturnvoid:	// ReturnVoid: нет потомков
			break;
		case TReturnval:	// ReturnValue: 2 потомка (тип значения, выражение)
			i = skip_expression(tree, i + 1, 1);
			break;

		case NOP:			// NoOperation: 0 потомков
			break;
		case CREATEDIRECTC:
			while (tree[i] != EXITC)
			{
				i = skip_operator(tree, i);
			}
			i = i == SIZE_MAX ? SIZE_MAX : i + 1;
			break;

		default:
			i--;
			if (!is_expression(tree[i]) && !is_lexeme(tree[i]))
			{
				warning(NULL, tree_operator_unknown, i, tree[i]);
			}

			i = skip_expression(tree, i, 1);	// CompoundStatement: n + 1 потомков (число потомков, n узлов-операторов)
												// ExpressionStatement: 1 потомок (выражение)
	}
	
	if (nd != NULL)
	{
		nd->tree = tree;
		//nd->type = i;
	}

	return i;
}

size_t skip_operator(tree *const tree, const size_t i)
{
	return node_operator(tree, i, NULL);
}


void tree_print(syntax *const sx)
{
	node nd = node_get_root(sx);
	printf("-=root=-\nnum\t%zi\nargc\t%zi\n", nd.amount, nd.argc);
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
	node child;
	if (!node_is_correct(nd) || index > nd->amount)
	{
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

	child.tree = nd->tree;
	/*hild->type = i;
	return node_init(child);*/
	return child;
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
	//tree_print(sx);
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
