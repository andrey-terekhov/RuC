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


static inline bool is_negative(const item_t value)
{
    return value >> (8 * sizeof(item_t) - 1);
}

static inline item_t to_negative(const size_t value)
{
    return ~(item_t)value + 1;
}

static inline size_t from_negative(const item_t value)
{
    return (size_t)(~value + 1);
}


static inline void vector_swap(vector *const vec, size_t fst, size_t snd)
{
    const item_t temp = vector_get(vec, fst);
    vector_set(vec, fst, vector_get(vec, snd));
    vector_set(vec, snd, temp);
}


static inline size_t ref_get_next(const node *const nd)
{
    return nd->index - 2;
}

static inline size_t ref_get_amount(const node *const nd)
{
    return nd->index;
}

static inline size_t ref_get_children(const node *const nd)
{
    return nd->index + 1;
}

static inline size_t ref_get_argc(const node *const nd)
{
    return nd->index + 2;
}


static inline int ref_set_next(const node *const nd, const item_t value)
{
    return vector_set(nd->tree, ref_get_next(nd), value);
}

static inline int ref_set_amount(const node *const nd, const item_t value)
{
    return vector_set(nd->tree, ref_get_amount(nd), value);
}

static inline int ref_set_children(const node *const nd, const item_t value)
{
    return vector_set(nd->tree, ref_get_children(nd), value);
}

static inline int ref_set_argc(const node *const nd, const item_t value)
{
    return vector_set(nd->tree, ref_get_argc(nd), value);
}


static inline node node_broken()
{
    node nd = { NULL, SIZE_MAX };
    return nd;
}


static node node_search_parent(const node *const nd, size_t *const number)
{
    if (!node_is_correct(nd) || nd->index == 0)
    {
        return node_broken();
    }

    size_t child_number = 1;
    item_t index = vector_get(nd->tree, ref_get_next(nd));
    while (!is_negative(index) && index != 0)
    {
        index = vector_get(nd->tree, (size_t)index - 2);
        child_number++;
    }

    node parent = { nd->tree, from_negative(index) };
    if (number != NULL)
    {
        *number = node_get_amount(&parent) - child_number;
    }

    return parent;
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
        vector_increase(tree, 3);
    }
    else if (size == SIZE_MAX || size < 3 || vector_get(tree, 2) < 0)
    {
        return node_broken();
    }

    node nd = { tree, 0 };
    return nd;
}

node node_get_child(const node *const nd, const size_t index)
{
    if (!node_is_correct(nd) || index >= node_get_amount(nd))
    {
        return node_broken();
    }

    size_t child_index = (size_t)vector_get(nd->tree, ref_get_children(nd));
    for (size_t i = 0; i < index; i++)
    {
        child_index = (size_t)vector_get(nd->tree, child_index - 2);
    }

    node child = { nd->tree, child_index };
    return child;
}

node node_get_parent(const node *const nd)
{
    return node_search_parent(nd, NULL);
}


item_t node_get_type(const node *const nd)
{
    return node_is_correct(nd) && nd->index != 0 ? vector_get(nd->tree, nd->index - 1) : ITEM_MAX;
}

size_t node_get_argc(const node *const nd)
{
    return node_is_correct(nd) ? (size_t)vector_get(nd->tree, ref_get_argc(nd)) : 0;
}

item_t node_get_arg(const node *const nd, const size_t index)
{
    return index < node_get_argc(nd) ? vector_get(nd->tree, ref_get_argc(nd) + 1 + index) : ITEM_MAX;
}

double node_get_arg_double(const node *const nd, const size_t index)
{
    return index + DOUBLE_SIZE <= node_get_argc(nd) ? vector_get_double(nd->tree, ref_get_argc(nd) + 1 + index)
                                                    : DBL_MAX;
}

int64_t node_get_arg_int64(const node *const nd, const size_t index)
{
    return index + INT64_SIZE <= node_get_argc(nd) ? vector_get_int64(nd->tree, ref_get_argc(nd) + 1 + index)
                                                   : LLONG_MAX;
}

size_t node_get_amount(const node *const nd)
{
    return node_is_correct(nd) ? (size_t)vector_get(nd->tree, ref_get_amount(nd)) : 0;
}


node node_get_next(const node *const nd)
{
    if (!node_is_correct(nd))
    {
        return node_broken();
    }

    node next = { nd->tree, (size_t)vector_get(nd->tree, ref_get_children(nd)) };

    if (node_get_amount(nd) == 0)
    {
        item_t index = vector_get(nd->tree, ref_get_next(nd));
        while (is_negative(index))
        {
            // Get next reference from parent
            index = vector_get(nd->tree, from_negative(index) - 2);
        }

        next.index = (size_t)index;
    }

    return next.index != 0 ? next : node_broken();
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


node node_add_child(const node *const nd, const item_t type)
{
    if (!node_is_correct(nd))
    {
        return node_broken();
    }

    vector_add(nd->tree, to_negative(nd->index));
    vector_add(nd->tree, type);
    node child = { nd->tree, vector_add(nd->tree, 0) };
    vector_increase(nd->tree, 2);

    const size_t amount = node_get_amount(nd);
    ref_set_amount(nd, (item_t)(amount + 1));

    if (amount == 0)
    {
        ref_set_children(nd, (item_t)child.index);
    }
    else
    {
        node prev = node_get_child(nd, amount - 1);
        ref_set_next(&prev, (item_t)child.index);
    }

    return child;
}

int node_set_type(const node *const nd, const item_t type)
{
    if (!node_is_correct(nd))
    {
        return -1;
    }

    if (nd->index == 0)
    {
        return -2;
    }

    return vector_set(nd->tree, nd->index - 1, type);
}

int node_add_arg(const node *const nd, const item_t arg)
{
    if (!node_is_correct(nd))
    {
        return -1;
    }

    if (node_get_amount(nd) != 0)
    {
        return -2;
    }

    vector_add(nd->tree, arg);
    ref_set_argc(nd, (item_t)node_get_argc(nd) + 1);

    return 0;
}

int node_add_arg_double(const node *const nd, const double arg)
{
    if (!node_is_correct(nd))
    {
        return -1;
    }

    if (node_get_amount(nd) != 0)
    {
        return -2;
    }

    vector_add_double(nd->tree, arg);
    ref_set_argc(nd, (item_t)(node_get_argc(nd) + DOUBLE_SIZE));

    return 0;
}

int node_add_arg_int64(const node *const nd, const int64_t arg)
{
    if (!node_is_correct(nd))
    {
        return -1;
    }

    if (node_get_amount(nd) != 0)
    {
        return -2;
    }

    vector_add_int64(nd->tree, arg);
    ref_set_argc(nd, (item_t)(node_get_argc(nd) + INT64_SIZE));

    return 0;
}

int node_set_arg(const node *const nd, const size_t index, const item_t arg)
{
    if (index >= node_get_argc(nd))
    {
        return -1;
    }

    return vector_set(nd->tree, ref_get_argc(nd) + 1 + index, arg);
}

size_t node_set_arg_double(const node *const nd, const size_t index, const double arg)
{
    if (index + DOUBLE_SIZE > node_get_argc(nd))
    {
        return SIZE_MAX;
    }

    return vector_set_double(nd->tree, ref_get_argc(nd) + 1 + index, arg);
}

size_t node_set_arg_int64(const node *const nd, const size_t index, const int64_t arg)
{
    if (index + INT64_SIZE > node_get_argc(nd))
    {
        return SIZE_MAX;
    }

    return vector_set_int64(nd->tree, ref_get_argc(nd) + 1 + index, arg);
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
    return node_is_correct(nd) ? nd->index : SIZE_MAX;
}

node node_load(vector *const tree, const size_t index)
{
    if (!vector_is_correct(tree) || vector_get(tree, index + 2) >= (item_t)(vector_size(tree) - index - 2))
    {
        return node_broken();
    }

    node nd = { tree, index };
    return nd;
}

node node_insert(const node *const nd, const item_t type, const size_t argc)
{
    size_t index;
    node parent = node_search_parent(nd, &index);
    if (!node_is_correct(&parent))
    {
        return node_broken();
    }

    size_t reference;
    if (index == 0)
    {
        reference = ref_get_children(&parent);
    }
    else
    {
        const node prev = node_get_child(&parent, index - 1);
        reference = ref_get_next(&prev);
    }

    vector_add(nd->tree, vector_get(nd->tree, ref_get_next(nd)));
    vector_add(nd->tree, type);
    node child = { nd->tree, vector_add(nd->tree, 1) };
    vector_add(nd->tree, (item_t)nd->index);
    vector_add(nd->tree, (item_t)argc);
    vector_increase(nd->tree, argc);

    vector_set(nd->tree, reference, (item_t)child.index);
    ref_set_next(nd, to_negative(child.index));
    return child;
}

int node_order(const node *const fst, const node *const snd)
{
    if (node_swap(fst, snd))
    {
        return -1;
    }

    vector_swap(fst->tree, ref_get_amount(fst), ref_get_amount(snd));
    vector_swap(fst->tree, ref_get_children(fst), ref_get_children(snd));

    const size_t fst_amount = node_get_amount(fst);
    if (fst_amount != 0)
    {
        const node child = node_get_child(fst, fst_amount - 1);
        ref_set_next(&child, to_negative(fst->index));
    }

    const size_t snd_amount = node_get_amount(snd);
    if (snd_amount != 0)
    {
        const node child = node_get_child(snd, snd_amount - 1);
        ref_set_next(&child, to_negative(snd->index));
    }

    return 0;
}

int node_swap(const node *const fst, const node *const snd)
{
    size_t fst_index = 0;
    const node fst_parent = node_search_parent(fst, &fst_index);

    size_t snd_index = 0;
    const node snd_parent = node_search_parent(snd, &snd_index);

    if (!node_is_correct(&fst_parent) || !node_is_correct(&snd_parent) || fst->tree != snd->tree)
    {
        return -1;
    }

    vector *const tree = fst->tree;
    if (fst_index == 0 && snd_index == 0)
    {
        vector_swap(tree, ref_get_children(&fst_parent), ref_get_children(&snd_parent));
    }
    else if (fst_index == 0) // && snd_index != 0
    {
        const node snd_prev = node_get_child(&snd_parent, snd_index - 1);
        vector_swap(tree, ref_get_children(&fst_parent), ref_get_next(&snd_prev));
    }
    else if (snd_index == 0) // && fst_index != 0
    {
        const node fst_prev = node_get_child(&fst_parent, fst_index - 1);
        vector_swap(tree, ref_get_next(&fst_prev), ref_get_children(&snd_parent));
    }
    else // fst_index != 0 && snd_index != 0
    {
        const node fst_prev = node_get_child(&fst_parent, fst_index - 1);
        const node snd_prev = node_get_child(&snd_parent, snd_index - 1);
        vector_swap(tree, ref_get_next(&fst_prev), ref_get_next(&snd_prev));
    }

    vector_swap(tree, ref_get_next(fst), ref_get_next(snd));
    return 0;
}

int node_remove(node *const nd)
{
    size_t index;
    node parent = node_search_parent(nd, &index);
    if (!node_is_correct(&parent))
    {
        return -1;
    }

    if (index == 0)
    {
        ref_set_amount(&parent, (item_t)node_get_amount(&parent) - 1);

        if (node_get_amount(&parent) != 0)
        {
            ref_set_children(&parent, vector_get(nd->tree, ref_get_next(nd)));
        }
    }
    else
    {
        *nd = node_get_child(&parent, index - 1);
        const size_t reference = (size_t)vector_get(nd->tree, ref_get_next(nd));
        ref_set_next(nd, vector_get(nd->tree, reference - 2));

        nd->index = reference;
        ref_set_amount(&parent, (item_t)node_get_amount(&parent) - 1);
    }

    if (node_get_amount(nd) == 0 && (ref_get_argc(nd) + node_get_argc(nd)) == vector_size(nd->tree) - 1)
    {
        vector_resize(nd->tree, ref_get_next(nd));
    }

    *nd = node_broken();
    return 0;
}

bool node_is_correct(const node *const nd)
{
    return nd != NULL && vector_is_correct(nd->tree) && nd->index != SIZE_MAX;
}
