/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "global_vars.h"

/**
 *	Размер дерева, используется для ускорения вставки и удаления узлов
 */
int tree_size;


/**
 *	Индекс в таблице идентификаторов текущего итератора for
 *	@note мы оптимизируем только циклы с одним итератором
 */
int iterator;

/**
 *	Шаг, с которым увеличивается текущий итератор
 *	@note мы оптимизируем только инкременты с шагом-константой
 */
int step;

/**
 *	Это массив для выражений вырезок индуцированных переменных
 *	Формат записи: индекс начала предыдущей записи, флаг, была ли объявлена переменная, выражение для вырезки
 */
int ind_vars[MAXIDENTAB];

/**
 *	Текущий размер таблицы индуцированных переменных
 */
int ind_vars_counter, ind_vars_start;

/** Проверяет, одинаковые ли выражения для вырезок индуцированных переменных */
int ind_vars_equal(int first_ind_var, int second_ind_var)
{
	int i = 1;

	do
	{
		if (ind_vars[first_ind_var + i] != ind_vars[second_ind_var + i])
			return 0;
		i++;
	} while (ind_vars[first_ind_var + i] != TExprend);
	return 1;
}

/** Добавить индуцированную переменную, если такой не было, или вернуть индекс на такую же */
int ind_var_add(int *record, int length)
{
	ind_vars[ind_vars_counter] = ind_vars_start;
	ind_vars_start = ind_vars_counter++;

	ind_vars[ind_vars_counter++] = 0; // Еще не была объявлена
	for (int i = 0; i < length; i++)
	{
		ind_vars[ind_vars_counter++] = record[i];
	}

	// Checking duplicates
	int old = ind_vars[ind_vars_start];
	while (old)
	{
		if (ind_vars_equal(ind_vars_start + 1, old + 1))
		{
			ind_vars_counter = ind_vars_start;
			ind_vars_start = ind_vars[ind_vars_start];
			return old + 1;
		}
		else
			old = ind_vars[old];
	}

	return ind_vars_start + 1;
}


/** Добавить узел в дерево по индексу */
void insert_tree(int index, int op)
{
	for (int i = tree_size; i >= index; i--)
	{
		tree[i+1] = tree[i];
		// Если есть еще затронутые циклы for, то им нужно сдвинуть ссылки на дерево
		// Костыль, но в будущих итерациях эта функция не понадобится, поэтому пока останется так
		if (tree[i+1] == TFor)
		{
			tree[i+2+check_nested_for]++;
			tree[i+3+check_nested_for]++;
			tree[i+4+check_nested_for]++;
			tree[i+5+check_nested_for]++;
		}
	}

	tree[index] = op;
	tree_size++;
}

/** Удалить узел из дерева по индексу */
void remove_tree(int index)
{
	tree_size--;
	for (int i = index; i < tree_size; i++)
	{
		// Если есть еще затронутые циклы for, то им нужно сдвинуть ссылки на дерево
		// Костыль, но в будущих итерациях эта функция не понадобится, поэтому пока останется так
		if (tree[i+1] == TFor)
		{
			tree[i+2+check_nested_for]--;
			tree[i+3+check_nested_for]--;
			tree[i+4+check_nested_for]--;
			tree[i+5+check_nested_for]--;
		}
		tree[i] = tree[i+1];
	}
}


/** Применить оптимизации к дереву */
void optimize()
{

}
