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

#include "old_tree.h"
#include <stdint.h>
#include <stdlib.h>
#include "defs.h"
#include "errors.h"


#ifdef OLD_TREE

#ifndef min
	#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif


const item_t REF_MASK = (item_t)0b11111111 << (8 * min(sizeof(item_t), sizeof(size_t)) - 8);
const item_t REF_LABEL = (item_t)0b10010010 << (8 * min(sizeof(item_t), sizeof(size_t)) - 8);


node node_expression(vector *const tree, const size_t index);
node node_operator(vector *const tree, const size_t index);


static inline int is_ref(const item_t value)
{
	return (REF_MASK & value) == REF_LABEL;
}

static inline item_t to_ref(const item_t value)
{
	return (~REF_MASK & value) | REF_LABEL;
}

static inline item_t from_ref(const item_t value)
{
	return is_ref(value)
		? ~REF_MASK & value
		: value;
}


int vector_swap(vector *const vec, size_t fst_index, size_t fst_size, size_t snd_index, size_t snd_size)
{
	if (fst_index > snd_index)
	{
		size_t temp = snd_index;
		snd_index = fst_index;
		fst_index = temp;

		temp = snd_size;
		snd_size = fst_size;
		fst_size = temp;
	}

	if (fst_index + fst_size > snd_index)
	{
		return -1;
	}

	vector temp = vector_create(snd_index + snd_size - fst_index);

	for (size_t i = snd_index; i < snd_index + snd_size; i++)
	{
		vector_add(&temp, vector_get(vec, i));
	}

	for (size_t i = fst_index + fst_size; i < snd_index; i++)
	{
		vector_add(&temp, vector_get(vec, i));
	}

	for (size_t i = fst_index; i < fst_index + fst_size; i++)
	{
		vector_add(&temp, vector_get(vec, i));
	}

	for (size_t i = 0; i < vector_size(&temp); i++)
	{
		vector_set(vec, fst_index + i, vector_get(&temp, i));
	}

	vector_clear(&temp);
	return 0;
}


int is_operator(const item_t value)
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

int is_expression(const item_t value)
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

int is_lexeme(const item_t value)
{
	return (value >= 9001 && value <= 9595
		&& value != CREATEDIRECTC
		&& value != EXITDIRECTC)
		|| value == ABSIC
		|| is_ref(value);
}


node node_broken()
{
	node nd;
	nd.tree = NULL;
	return nd;
}


size_t skip_expression(vector *const tree, size_t i)
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

node node_expression(vector *const tree, const size_t index)
{
	if (index == SIZE_MAX)
	{
		return node_broken();
	}

	node nd;

	nd.tree = tree;
	nd.type = index;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	if (is_operator(vector_get(tree, index)))
	{
		system_error(tree_expression_not_block, index, vector_get(tree, index));
		return node_broken();
	}

	switch (vector_get(tree, index))
	{
		case TBeginit:		// ArrayInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
		case TStructinit:	// StructInit: n + 1 потомков (размерность инициализатора, n выражений-инициализаторов)
			nd.argc = 1;
			nd.amount = (size_t)node_get_arg(&nd, 0);
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
			nd.argc += (size_t)node_get_arg(&nd, 0) * 2;
			break;
		case TString:
			nd.argc = 1;
			nd.argc += (size_t)node_get_arg(&nd, 0);
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
			nd.amount = is_ref(vector_get(tree, index + 1)) ? 1 : 0;
			return nd;

		case NOP:
			if (vector_get(tree, index + 1) != TExprend)
			{
				system_error(tree_expression_no_texprend, index, vector_get(tree, index));
				return node_broken();
			}
			break;

		default:
		{
			if (!is_lexeme(vector_get(tree, index)))
			{
				system_error(tree_expression_unknown, index, vector_get(tree, index));
				return node_broken();
			}

			size_t j = index + 1;
			while (j < vector_size(tree) && vector_get(tree, j) != NOP && !is_expression(vector_get(tree, j)) && !is_lexeme(vector_get(tree, j)))
			{
				if (is_operator(vector_get(tree, j)))
				{
					system_error(tree_expression_operator, j, vector_get(tree, j));
					return node_broken();
				}

				nd.argc++;
				j++;
			}
		}
		break;
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

	if (vector_get(tree, j) == NOP || is_expression(vector_get(tree, j)) || is_lexeme(vector_get(tree, j)))
	{
		nd.amount++;
	}
	else if (j < vector_size(tree))
	{
		system_error(tree_expression_no_texprend, j, vector_get(tree, j));
		return node_broken();
	}

	return nd;
}


size_t skip_operator(vector *const tree, size_t i)
{
	if (!is_operator(vector_get(tree, i)) && vector_get(tree, i) != NOP)
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

node node_operator(vector *const tree, const size_t index)
{
	if (index == SIZE_MAX)
	{
		return node_broken();
	}

	node nd;

	nd.tree = tree;
	nd.type = index;
	nd.argv = nd.type + 1;

	nd.argc = 0;
	nd.amount = 0;

	switch (vector_get(tree, index))
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
			nd.amount = (size_t)node_get_arg(&nd, 0) + 1;
		}
		break;

		case TStructbeg:	// StructDecl: n + 2 потомков (размерность структуры, n объявлений полей, инициализатор (может не быть))
		{
			nd.argc = 1;
			size_t j = nd.argv + nd.argc;
			while (j != SIZE_MAX && vector_get(tree, j) != TStructend)
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
			while (j != SIZE_MAX && vector_get(tree, j) != TEnd)
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
			nd.amount = is_expression(vector_get(tree, nd.argv + nd.argc)) || is_lexeme(vector_get(tree, nd.argv + nd.argc)) ? 1 : 0;
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
			while (j != SIZE_MAX && vector_get(tree, j) != EXITDIRECTC)
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
			if (!is_expression(vector_get(tree, index)) && !is_lexeme(vector_get(tree, index)))
			{
				warning(NULL, tree_operator_unknown, index, vector_get(tree, index));
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

	if (vector_get(nd->tree, i++) != node_get_type(nd))
	{
		system_error(tree_unexpected, node_get_type(nd), i - 1, vector_get(nd->tree, i - 1));
		return SIZE_MAX;
	}

	for (size_t j = 0; node_get_arg(nd, j) != ITEM_MAX; j++)
	{
		if (vector_get(nd->tree, i++) != node_get_arg(nd, j))
		{
			system_error(tree_unexpected, node_get_arg(nd, j), i - 1, vector_get(nd->tree, i - 1));
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
	node child_dest = node_add_child(dest, node_get_type(nd));
	if (!node_is_correct(&child_dest))
	{
		system_error(node_cannot_add_child, dest->type, node_get_type(dest));
		return -1;
	}

	if (node_set_type(&child_dest, node_get_type(nd)))
	{
		system_error(node_cannot_set_type, node_get_type(nd), child_dest.type);
		return -1;
	}

	for (size_t i = 0; node_get_arg(nd, i) != ITEM_MAX; i++)
	{
		if (node_add_arg(&child_dest, node_get_arg(nd, i)))
		{
			system_error(node_cannot_add_arg, node_get_arg(nd, i), child_dest.type, node_get_type(&child_dest));
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


node node_get_root(vector *const tree)
{
	if (!vector_is_correct(tree))
	{
		return node_broken();
	}

	node nd;

	nd.tree = tree;
	nd.type = SIZE_MAX;
	nd.argv = 0;
	nd.argc = 0;
	nd.children = 0;
	nd.amount = 0;

	size_t i = 0;
	while (i != SIZE_MAX && vector_get(nd.tree, i) != ITEM_MAX)
	{
		i = skip_operator(nd.tree, i);
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
	if (!node_is_correct(nd) || index >= nd->amount)
	{
		return node_broken();
	}

	size_t i = nd->children;
	for (size_t num = 0; num < index; num++)
	{
		if (nd->type == SIZE_MAX || is_operator(node_get_type(nd)))
		{
			i = skip_operator(nd->tree, i);
		}
		else
		{
			i = skip_expression(nd->tree, i);
		}
	}

	return nd->type == SIZE_MAX || is_operator(node_get_type(nd))
		? node_operator(nd->tree, i)
		: node_expression(nd->tree, i);
}


size_t node_get_amount(const node *const nd)
{
	return node_is_correct(nd) ? nd->amount : 0;
}

item_t node_get_type(const node *const nd)
{
	return node_is_correct(nd) ? vector_get(nd->tree, nd->type) : ITEM_MAX;
}

item_t node_get_arg(const node *const nd, const size_t index)
{
	return node_is_correct(nd) && index < nd->argc ? vector_get(nd->tree, nd->argv + index) : ITEM_MAX;
}


node node_get_next(node *const nd)
{
	if (!node_is_correct(nd) || vector_get(nd->tree, nd->children) == ITEM_MAX)
	{
		return node_broken();
	}

	if (nd->type == SIZE_MAX)
	{
		return node_operator(nd->tree, 0);
	}

	return node_operator(nd->tree, nd->children);
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


node node_add_child(node *const nd, const item_t type)
{
	if (!node_is_correct(nd))
	{
		return node_broken();
	}

	node child;

	child.tree = nd->tree;
	child.type = vector_add(nd->tree, type);

	child.argv = child.type + 1;
	child.argc = 0;

	child.children = child.argv + child.argc;
	child.amount = 0;

	nd->amount++;
	return child;
}

int node_set_type(node *const nd, const item_t type)
{
	if (!node_is_correct(nd))
	{
		return -1;
	}

	if (nd->type == SIZE_MAX)
	{
		return -2;
	}

	return vector_set(nd->tree, nd->type, type);
}

int node_add_arg(node *const nd, const item_t arg)
{
	if (!node_is_correct(nd))
	{
		return -1;
	}

	if (node_get_type(nd) == ITEM_MAX)
	{
		return -2;
	}

	if (nd->amount != 0)
	{
		return -3;
	}

	const int ret = nd->argv + nd->argc == vector_size(nd->tree)
					? vector_add(nd->tree, from_ref(arg)) != SIZE_MAX ? 0 : -1
					: -1;
	if (!ret)
	{
		nd->argc++;
	}

	return ret;
}

int node_set_arg(node *const nd, const size_t index, const item_t arg)
{
	if (!node_is_correct(nd) || index >= nd->argc)
	{
		return -1;
	}

	if (node_get_type(nd) == ITEM_MAX)
	{
		return -2;
	}

	return vector_set(nd->tree, nd->argv + index, from_ref(arg));
}


int node_copy(node *const dest, const node *const src)
{
	if (!node_is_correct(src) || dest == NULL)
	{
		return -1;
	}

	*dest = *src;
	return 0;
}

size_t node_save(const node *const nd)
{
	return node_is_correct(nd)
		? (size_t)to_ref(nd->type)
		: SIZE_MAX;
}

node node_load(vector *const tree, const size_t index)
{
	const size_t i = (size_t)from_ref(index);
	if (!vector_is_correct(tree) || i >= vector_size(tree))
	{
		return node_broken();
	}

	return is_operator(vector_get(tree, i))
		? node_operator(tree, i)
		: node_expression(tree, i);
}

int node_order(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index)
{
	if (!node_is_correct(fst) || !node_is_correct(snd) || fst->tree != snd->tree
		|| fst_index >= fst->amount || snd_index >= snd->amount)
	{
		return -1;
	}

	vector *const tree = fst->tree;

	node temp = node_get_child(fst, fst_index);
	const size_t fst_child_index = temp.type;
	const size_t fst_size = temp.argc + 1;

	temp = node_get_child(snd, snd_index);
	const size_t snd_child_index = temp.type;
	const size_t snd_size = temp.argc + 1;

	if (fst->type >= snd_child_index)
	{
		fst->type = fst->type == snd_child_index
			? fst_child_index + fst_size - snd_size
			: fst->type + fst_size - snd_size;
		fst->argv = fst->type + 1;
		fst->children = fst->argv + fst->argc;
	}

	if (snd->type >= fst_child_index)
	{
		snd->type = snd->type == fst_child_index
			? snd_child_index + snd_size - fst_size
			: snd->type + snd_size - fst_size;
		snd->argv = snd->type + 1;
		snd->children = snd->argv + snd->argc;
	}

	return vector_swap(tree, fst_child_index, fst_size, snd_child_index, snd_size);
}

int node_swap(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index)
{
	if (!node_is_correct(fst) || !node_is_correct(snd) || fst->tree != snd->tree
		|| fst_index >= fst->amount || snd_index >= snd->amount)
	{
		return -1;
	}

	vector *const tree = fst->tree;

	node temp = node_get_child(fst, fst_index);
	const size_t fst_child_index = temp.type;

	while (temp.amount != 0)
	{
		temp = node_get_child(&temp, temp.amount - 1);
	}
	const size_t fst_size = temp.type - fst_child_index + temp.argc + 1;

	temp = node_get_child(snd, snd_index);
	const size_t snd_child_index = temp.type;

	while (temp.amount != 0)
	{
		temp = node_get_child(&temp, temp.amount - 1);
	}
	const size_t snd_size = temp.type - snd_child_index + temp.argc + 1;

	return vector_swap(tree, fst_child_index, fst_size, snd_child_index, snd_size);
}

int node_remove(node *const nd, const size_t index)
{
	if (!node_is_correct(nd) || index >= nd->amount)
	{
		return -1;
	}

	node child = node_get_child(nd, index);
	const size_t from = child.type;

	while (child.amount != 0)
	{
		child = node_get_child(&child, child.amount - 1);
	}
	const size_t to = child.type + child.argc + 1;

	if (to == vector_size(nd->tree))
	{
		nd->amount--;
		return vector_resize(nd->tree, from);
	}

	for (size_t i = 0; i < vector_size(nd->tree) - to; i++)
	{
		vector_set(nd->tree, from + i, vector_get(nd->tree, to + i));
	}

	nd->amount--;
	return vector_resize(nd->tree, vector_size(nd->tree) - to + from);
}

int node_is_correct(const node *const nd)
{
	return nd != NULL && vector_is_correct(nd->tree);
}


int tree_test(vector *const tree)
{
	if (!vector_is_correct(tree))
	{
		return -1;
	}

	// Тестирование функций
	size_t i = 0;
	while (i != SIZE_MAX && i < vector_size(tree) - 1)
	{
		i = skip_operator(tree, i);
	}

	if (i == SIZE_MAX)
	{
		return -1;
	}

	if (vector_get(tree, i) == TEnd)
	{
		return 0;
	}

	system_error(tree_no_tend);
	return -1;
}

int tree_test_next(vector *const tree)
{
	if (!vector_is_correct(tree))
	{
		return -1;
	}

	node nd = node_get_root(tree);

	size_t i = 0;
	nd = node_get_next(&nd);
	while (node_is_correct(&nd))
	{
		if (vector_get(tree, i++) != node_get_type(&nd))
		{
			system_error(tree_unexpected, node_get_type(&nd), i - 1, vector_get(tree, i - 1));
			return -1;
		}

		for (size_t j = 0; node_get_arg(&nd, j) != ITEM_MAX; j++)
		{
			if (vector_get(tree, i++) != node_get_arg(&nd, j))
			{
				system_error(tree_unexpected, node_get_arg(&nd, j), i - 1, vector_get(tree, i - 1));
				return -1;
			}
		}
		nd = node_get_next(&nd);
	}

	return i == vector_size(tree) ? 0 : -1;
}

int tree_test_recursive(vector *const tree)
{
	if (!vector_is_correct(tree))
	{
		return -1;
	}

	node nd = node_get_root(tree);

	size_t index = 0;
	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		index = node_test_recursive(&child, index);
	}

	return index != SIZE_MAX && index == vector_size(nd.tree) ? 0 : -1;
}

int tree_test_copy(vector *const tree)
{
	if (!vector_is_correct(tree))
	{
		return -1;
	}

	vector tree_dest = vector_create(0);

	node nd = node_get_root(tree);
	node nd_dest = node_get_root(&tree_dest);

	for (size_t i = 0; i < node_get_amount(&nd); i++)
	{
		node child = node_get_child(&nd, i);
		if (node_test_copy(&nd_dest, &child))
		{
			vector_clear(&tree_dest);
			return -1;
		}
	}

	size_t i = 0;
	while (i < vector_size(nd.tree) && i < vector_size(nd_dest.tree))
	{
		if (vector_get(nd.tree, i) != vector_get(nd_dest.tree, i))
		{
			system_error(tree_unexpected, vector_get(nd_dest.tree, i), i, vector_get(nd.tree, i));
			vector_clear(&tree_dest);
			return -1;
		}

		i++;
	}

	const int ret = vector_size(nd.tree) != vector_size(nd_dest.tree);
	vector_clear(&tree_dest);
	return ret;
}

#endif
