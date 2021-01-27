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
#include <stdlib.h>


node node_expression(tree *const tree, const size_t index);
node node_operator(tree *const tree, const size_t index);


tree sx_to_tree(syntax *const sx)
{
	tree tree;
	if (sx == NULL)
	{
		tree.array = NULL;
		return tree;
	}

	tree.array = sx->tree;
	tree.size = &sx->tc;

	return tree;
}


int tree_is_correct(const tree *const tree)
{
	return tree != NULL && tree->array != NULL && tree->size != NULL;
}

size_t tree_size(const tree *const tree)
{
	return tree_is_correct(tree) ? *tree->size : 0;
}

int tree_add(tree *const tree, const int value)
{
	if (!tree_is_correct(tree))
	{
		return -1;
	}

	tree->array[(*tree->size)++] = value;
	return 0;
}

int tree_set(tree *const tree, const size_t index, const int value)
{
	if (!tree_is_correct(tree) || index > tree_size(tree))
	{
		return -1;
	}

	tree->array[index] = value;
	return 0;
}

int tree_get(const tree *const tree, const size_t index)
{
	return tree_is_correct(tree) && index < tree_size(tree) ? tree->array[index] : INT_MAX;
}


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

		|| value == CREATEDIRECTC	// Lexemes
		|| value == EXITDIRECTC;
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
		&& value != CREATEDIRECTC
		&& value != EXITDIRECTC)
		|| value == ABSIC;
}


node node_broken()
{
	node nd;
	nd.tree.array = NULL;
	return nd;
}


size_t skip_expression(tree *const tree, size_t i)
{
	node nd = node_expression(tree, i);
	if (!node_is_correct(&nd))
	{
		return SIZE_MAX;
	}

	i = nd.children;
	for (size_t j = 0; j < node_get_amount(&nd); j++)
	{
		i = skip_expression(tree, i);
	}

	return i;
}

node node_expression(tree *const tree, const size_t index)
{
	if (index == SIZE_MAX)
	{
		return node_broken();
	}

	node nd;

	nd.tree = *tree;
	nd.type = index;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	if (is_operator(tree_get(tree, index)))
	{
		error(NULL, tree_expression_not_block, index, tree_get(tree, index));
		return node_broken();
	}

	switch (tree_get(tree, index))
	{
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
			nd.argc = 1;
			nd.amount = node_get_arg(&nd, 0);
			break;

		case TAddrtovald:
		case TAddrtoval:

		case TCondexpr:
			break;

		case TConstd:		// d - double
			nd.argc = 2;
			break;
		case TConst:
		case TSelect:

		case TPrint:		// Print: 2 потомка (тип значения, выражение)

		case TIdenttoaddr:
		case TIdenttovald:
		case TIdenttoval:

		case TCall1:
			nd.argc = 1;
			break;
		case TCall2:

		case TIdent:
			nd.argc = 1;
			break;

		case TStringd:		// d - double
			nd.argc = 1;
			nd.argc += node_get_arg(&nd, 0) * 2;
			break;
		case TString:
			nd.argc = 1;
			nd.argc += node_get_arg(&nd, 0);
			break;

		case TSliceident:
			nd.argc = 2;
			nd.amount = 1;
			break;
		case TSlice:
			nd.argc = 1;
			nd.amount = 1;
			break;

		case TExprend:
			nd.children = nd.argv + nd.argc;
			return nd;

		case NOP:
			if (tree_get(tree, index + 1) != TExprend)
			{
				error(NULL, tree_expression_no_texprend, index, tree_get(tree, index));
				return node_broken();
			}
			break;

		default:
		{
			if (!is_lexeme(tree_get(tree, index)))
			{
				error(NULL, tree_expression_unknown, index, tree_get(tree, index));
				return node_broken();
			}

			size_t j = index + 1;
			while (tree_get(tree, j) != NOP && !is_expression(tree_get(tree, j)) && !is_lexeme(tree_get(tree, j)))
			{
				if (is_operator(tree_get(tree, j)))
				{
					error(NULL, tree_expression_operator, j, tree_get(tree, j));
					return node_broken();
				}

				nd.argc++;
				j++;
			}
		}
	}

	nd.children = nd.argv + nd.argc;
	size_t j = nd.children;
	for (size_t k = 0; k < nd.amount; k++)
	{
		j = skip_expression(tree, j);
	}

	if (j == SIZE_MAX)
	{
		return node_broken();
	}

	if (tree_get(tree, j) == NOP || is_expression(tree_get(tree, j)) || is_lexeme(tree_get(tree, j)))
	{
		nd.amount++;
	}
	else
	{
		error(NULL, tree_expression_no_texprend, j, tree_get(tree, j));
		return node_broken();
	}

	return nd;
}


size_t skip_operator(tree *const tree, size_t i)
{
	if (!is_operator(tree_get(tree, i)) && tree_get(tree, i) != NOP)
	{
		return skip_expression(tree, i);
	}

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

	return i;
}

node node_operator(tree *const tree, const size_t index)
{
	if (index == SIZE_MAX)
	{
		return node_broken();
	}

	node nd;

	nd.tree = *tree;
	nd.type = index;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	switch (tree_get(tree, index))
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
			while (j != SIZE_MAX && tree_get(tree, j) != TStructend)
			{
				j = skip_operator(tree, j);
				nd.amount++;
			}

			if (j != SIZE_MAX)
			{
				skip_operator(tree, j);
				nd.amount++;
			}
			else
			{
				return node_broken();
			}
		}
		break;
		case TStructend:
			nd.argc = 1;
			break;

		case TBegin:
		{
			size_t j = nd.argv + nd.argc;
			while (j != SIZE_MAX && tree_get(tree, j) != TEnd)
			{
				j = skip_operator(tree, j);
				nd.amount++;
			}

			if (j != SIZE_MAX)
			{
				skip_operator(tree, j);
				nd.amount++;
			}
			else
			{
				return node_broken();
			}
			
		}
		break;
		case TEnd:
			break;

		case TPrintid:		// PrintID: 2 потомка (ссылка на reprtab, ссылка на identab)
			nd.argc = 1;
			nd.amount = is_expression(tree_get(tree, nd.argv + nd.argc)) || is_lexeme(tree_get(tree, nd.argv + nd.argc)) ? 1 : 0;
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
			while (j != SIZE_MAX && tree_get(tree, j) != EXITDIRECTC)
			{
				j = skip_operator(tree, j);
				nd.amount++;
			}

			if (j != SIZE_MAX)
			{
				skip_operator(tree, j);
				nd.amount++;
			}
			else
			{
				return node_broken();
			}
		}
		break;
		case EXITDIRECTC:
			break;

		default:
			if (!is_expression(tree_get(tree, index)) && !is_lexeme(tree_get(tree, index)))
			{
				warning(NULL, tree_operator_unknown, index, tree_get(tree, index));
			}

			return node_expression(tree, index);	// CompoundStatement: n + 1 потомков (число потомков, n узлов-операторов)
													// ExpressionStatement: 1 потомок (выражение)
	}

	nd.children = nd.argv + nd.argc;
	return nd;
}


size_t node_test_recursive(node *const nd, size_t i)
{
	if (i == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	if (tree_get(&nd->tree, i++) != node_get_type(nd))
	{
		error(NULL, tree_unexpected, node_get_type(nd), i - 1, tree_get(&nd->tree, i - 1));
		return SIZE_MAX;
	}

	for (size_t j = 0; node_get_arg(nd, j) != INT_MAX; j++)
	{
		if (tree_get(&nd->tree, i++) != node_get_arg(nd, j))
		{
			error(NULL, tree_unexpected, node_get_arg(nd, j), i - 1, tree_get(&nd->tree, i - 1));
			return SIZE_MAX;
		}
	}

	for (size_t j = 0; j < node_get_amount(nd); j++)
	{
		node child = node_get_child(nd, j);
		i = node_test_recursive(&child, i);
	}
	return i;
}

int node_test_copy(node *const dest, node *const nd)
{
	node child_dest = node_set_child(dest);
	if (!node_is_correct(&child_dest))
	{
		error(NULL, node_cannot_set_child, dest->type, node_get_type(dest));
		return -1;
	}

	if (node_set_type(&child_dest, node_get_type(nd)))
	{
		error(NULL, node_cannot_set_type, node_get_type(nd), child_dest.type);
		return -1;
	}

	for (size_t i = 0; node_get_arg(nd, i) != INT_MAX; i++)
	{
		if (node_add_arg(&child_dest, node_get_arg(nd, i)))
		{
			error(NULL, node_cannot_add_arg, node_get_arg(nd, i), child_dest.type, node_get_type(&child_dest));
			return -1;
		}
	}

	for (size_t i = 0; i < node_get_amount(nd); i++)
	{
		node child = node_get_child(nd, i);
		if (node_test_copy(dest, &child))
		{
			return -1;
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


node node_get_root(syntax *const sx)
{
	if (sx == NULL)
	{
		return node_broken();
	}

	node nd;

	nd.tree = sx_to_tree(sx);
	nd.type = SIZE_MAX;
	nd.argv = 0;
	nd.argc = 0;
	nd.children = 0;
	nd.amount = 0;

	size_t i = 0;
	while (i != SIZE_MAX && tree_get(&nd.tree, i) != INT_MAX)
	{
		i = skip_operator(&nd.tree, i);
		nd.amount++;
	}

	if (i == SIZE_MAX)
	{
		return node_broken();
	}

	return nd;
}

node node_get_child(node *const nd, const size_t index)
{
	if (!node_is_correct(nd) || index > nd->amount)
	{
		return node_broken();
	}

	size_t i = nd->children;
	for (size_t num = 0; num < index; num++)
	{
		if (nd->type == SIZE_MAX || is_operator(node_get_type(nd)))
		{
			i = skip_operator(&nd->tree, i);
		}
		else
		{
			i = skip_expression(&nd->tree, i);
		}
	}

	return nd->type == SIZE_MAX || is_operator(node_get_type(nd))
		? node_operator(&nd->tree, i)
		: node_expression(&nd->tree, i);
}


size_t node_get_amount(const node *const nd)
{
	return node_is_correct(nd) ? nd->amount : 0;
}

int node_get_type(const node *const nd)
{
	return node_is_correct(nd) && nd->type != SIZE_MAX ? tree_get(&nd->tree, nd->type) : INT_MAX;
}

int node_get_arg(const node *const nd, const size_t index)
{
	return node_is_correct(nd) && index < nd->argc ? tree_get(&nd->tree, nd->argv + index) : INT_MAX;
}


node node_get_next(node *const nd)
{
	if (!node_is_correct(nd) || tree_get(&nd->tree, nd->children) == INT_MAX)
	{
		return node_broken();
	}

	if (nd->type == SIZE_MAX)
	{
		return node_operator(&nd->tree, 0);
	}

	return node_operator(&nd->tree, nd->children);
}

int node_set_next(node *const nd)
{
	node next = node_get_next(nd);
	if (!node_is_correct(&next))
	{
		return -1;
	}

	*nd = next;
	return 0;
}


int node_set_type(node *const nd, const int type)
{
	if (!node_is_correct(nd))
	{
		return -1;
	}

	if (nd->type == SIZE_MAX)
	{
		return -2;
	}

	if (nd->argc != 0 || nd->amount != 0)
	{
		return -3;
	}

	return nd->type == tree_size(&nd->tree)
		? tree_add(&nd->tree, type)
		: tree_set(&nd->tree, nd->type, type);
}

int node_add_arg(node *const nd, const int arg)
{
	if (!node_is_correct(nd))
	{
		return -1;
	}

	if (node_get_type(nd) == INT_MAX)
	{
		return -2;
	}

	if (nd->amount != 0)
	{
		return -3;
	}

	const int ret = nd->argv + nd->argc == tree_size(&nd->tree)
					? tree_add(&nd->tree, arg)
					: -1;
	if (!ret)
	{
		nd->argc++;
	}

	return ret;
}

int node_set_arg(node *const nd, const size_t index, const int arg)
{
	if (!node_is_correct(nd) || index >= nd->argc)
	{
		return -1;
	}

	if (node_get_type(nd) == INT_MAX)
	{
		return -2;
	}

	return tree_set(&nd->tree, nd->argv + index, arg);
}

node node_set_child(node *const nd)
{
	if (!node_is_correct(nd))
	{
		return node_broken();
	}

	node child;

	child.tree = nd->tree;
	child.type = tree_size(&nd->tree);

	child.argv = child.type + 1;
	child.argc = 0;

	child.children = child.argv + child.argc;
	child.amount = 0;

	return child;
}


int node_is_correct(const node *const nd)
{
	return nd != NULL && tree_is_correct(&nd->tree);
}


int tree_test(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	tree tree = sx_to_tree(sx);

	// Тестирование функций
	size_t i = 0;
	while (i != SIZE_MAX && i < tree_size(&tree) - 1)
	{
		i = skip_operator(&tree, i);
	}

	if (i == SIZE_MAX)
	{
		return -1;
	}

	if (tree_get(&tree, i) == TEnd)
	{
		return 0;
	}

	error(NULL, tree_no_tend);
	return -1;
}

int tree_test_next(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	node nd = node_get_root(sx);
	tree temp = nd.tree;

	size_t i = 0;
	nd = node_get_next(&nd);
	while (node_is_correct(&nd))
	{
		if (tree_get(&temp, i++) != node_get_type(&nd))
		{
			error(NULL, tree_unexpected, node_get_type(&nd), i - 1, tree_get(&temp, i - 1));
			return -1;
		}

		for (size_t j = 0; node_get_arg(&nd, j) != INT_MAX; j++)
		{
			if (tree_get(&temp, i++) != node_get_arg(&nd, j))
			{
				error(NULL, tree_unexpected, node_get_arg(&nd, j), i - 1, tree_get(&temp, i - 1));
				return -1;
			}
		}
		nd = node_get_next(&nd);
	}

	return i == tree_size(&temp) ? 0 : -1;
}

int tree_test_recursive(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	node nd = node_get_root(sx);

	size_t index = 0;
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		index = node_test_recursive(&child, index);
	}

	return index != SIZE_MAX && index == tree_size(&nd.tree) ? 0 : -1;
}

int tree_test_copy(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	syntax sx_dest;
	sx_init(&sx_dest);

	node nd = node_get_root(sx);
	node nd_dest = node_get_root(&sx_dest);

	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		if (node_test_copy(&nd_dest, &child))
		{
			sx_clear(&sx_dest);
			return -1;
		}
	}

	size_t i = 0;
	while (i < tree_size(&nd.tree) && i < tree_size(&nd_dest.tree))
	{
		if (tree_get(&nd.tree, i) != tree_get(&nd_dest.tree, i))
		{
			error(NULL, tree_unexpected, tree_get(&nd_dest.tree, i), i, tree_get(&nd.tree, i));
			sx_clear(&sx_dest);
			return -1;
		}

		i++;
	}

	const int ret = tree_size(&nd.tree) != tree_size(&nd_dest.tree);
	sx_clear(&sx_dest);
	return ret;
}
