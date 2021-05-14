/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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
//#include <stdint.h>
//#include <stdlib.h>


node node_broken()
{
	node nd;
	nd.tree = NULL;
	return nd;
}


void vector_swap(vector *const vec, size_t fst, size_t snd)
{
	const item_t temp = vector_get(vec, fst);
	vector_set(vec, fst, vector_get(vec, snd));
	vector_set(vec, snd, temp);
}

int node_displ(node *fst, const size_t fst_index, node *snd, const size_t snd_index)
{
	if (!node_is_correct(fst) || !node_is_correct(snd) || fst->tree != snd->tree
		|| fst_index >= node_get_amount(fst) || snd_index >= node_get_amount(snd))
	{
		return -1;
	}

	vector *const tree = fst->tree;
	size_t reference;

	if (fst_index == 0)
	{
		reference = fst->index + 2 + vector_get(tree, fst->index);
		*fst = node_get_child(fst, fst_index);
	}
	else
	{
		*fst = node_get_child(fst, fst_index - 1);
		reference = fst->index - 2;
		fst->index = vector_get(tree, reference);
	}

	if (snd_index == 0)
	{
		vector_swap(tree, reference, snd->index + 2 + vector_get(tree, snd->index));
		*snd = node_get_child(snd, snd_index);
	}
	else
	{
		*snd = node_get_child(snd, snd_index - 1);
		vector_swap(tree, reference, snd->index - 2);
		snd->index = vector_get(tree, reference);
	}

	vector_swap(tree, fst->index - 2, snd->index - 2);
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
	const size_t size = vector_size(tree);
	if (size == 0)
	{
		vector_add(tree, 0);
		vector_add(tree, 0);
		vector_add(tree, 5);
	}
	else if (size == SIZE_MAX || size < 3 || vector_get(tree, 0) < 0)
	{
		return node_broken();
	}

	node nd = { tree, 0 };
	return nd;
}

node node_get_child(node *const nd, const size_t index)
{
	if (!node_is_correct(nd) || index >= nd->amount)
	{
		return node_broken();
	}

	return nd;
}

node node_get_parent(node *const nd)
{
	if (!node_is_correct(nd))
	{
		return node_broken();
	}

	return nd;
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


node node_get_next(node *const nd);

int node_set_next(node *const nd);


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
		? nd->index
		: SIZE_MAX;
}

node node_load(vector *const tree, const size_t index)
{
	if (!vector_is_correct(tree) || vector_get(tree, index) >= (item_t)vector_size(tree) - index - 2)
	{
		return node_broken();
	}

	node nd = { tree, index };
	return nd;
}

int node_order(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index)
{
	node fst_child = *fst;	
	node snd_child = *snd;

	if (node_displ(&fst_child, fst_index, &snd_child, snd_index))
	{
		return -1;
	}

	vector *const tree = fst->tree;

	vector_swap(tree, fst_child.index + 1 + vector_get(tree, fst_child.index)
		, snd_child.index + 1 + vector_get(tree, snd_child.index));
	vector_swap(tree, fst_child.index + 2 + vector_get(tree, fst_child.index)
		, snd_child.index + 2 + vector_get(tree, snd_child.index));

	return 0;
}

int node_swap(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index)
{
	node fst_child = *fst;	
	node snd_child = *snd;

	if (node_displ(&fst_child, fst_index, &snd_child, snd_index))
	{
		return -1;
	}

	vector *const tree = fst->tree;

#ifdef BUFFERING
	node_update(&fst_child);
	node_update(&snd_child);
#endif

	return 0;
}

int node_remove(node *const nd, const size_t index)
{
	if (!node_is_correct(nd) || index >= node_get_amount(nd))
	{
		return -1;
	}

	node child = node_get_child(nd, index);

	if (index == 0)
	{

	}

	return 0;
}

int node_is_correct(const node *const nd)
{
	return nd != NULL && vector_is_correct(nd->tree);
}
