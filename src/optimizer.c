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

// Поправка индексов
int index_correction = 0;

/**
 * Начало разбираемого в данный момент цикла for
 */
int for_start;

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

void mtotree(int op)
{
	mtree[mtc++] = op;
}

static int mcopy()
{
	// Пропускаем все узлы TSkip, которые встретим
	while (tree[tc] == TSkip) index_correction++, tc++;
	
	// Правим ссылки TFor
	if (tree[tc] == TFor)
	{
		tree[tc+2+check_nested_for] += index_correction;
		tree[tc+3+check_nested_for] += index_correction;
		tree[tc+4+check_nested_for] += index_correction;
		tree[tc+5+check_nested_for] += index_correction;
	}
	
	// Копируем узел
	return mtree[mtc++] = tree[tc++];
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

/** Проверить, существуют ли вложенные циклы for */
void mark_nested_for()
{
	int flag_nested_for_ref = tc;
	int temp_tc = tc + 1;
	while (tree[temp_tc] != TForEnd)
	{
		if (tree[temp_tc] == TFor)
		{
			return;
		}
		temp_tc++;
	}

	if (tree[temp_tc] == TForEnd)
		tree[flag_nested_for_ref] = 1;
}

int check_constant_identifier(int id)
{
	// Начинаем проверку с условия выхода из цикла,
	// так как в инициализации можно менять значение этой переменной
	int local_tc = tree[for_start + check_nested_for + 2];
	
	while (tree[local_tc] != TForEnd)
	{
		if (tree[local_tc] == TIdent && tree[local_tc+1] == id)
		{
			// Здесь используется прямая польская запись, поэтому
			// все инкременты и операции присваивания стоят прямо перед идентификатором
			switch (tree[local_tc-1])
			{
				case POSTINC:
				case POSTDEC:
				case INC:
				case DEC:

				case REMASS     + 1000:
				case SHLASS     + 1000:
				case SHRASS     + 1000:
				case ANDASS     + 1000:
				case EXORASS    + 1000:
				case ORASS      + 1000:

				case ASS        + 1000:
				case PLUSASS    + 1000:
				case MINUSASS   + 1000:
				case MULTASS    + 1000:
				case DIVASS     + 1000:
						
					return 0;
					
				default:
					break;
			}
		}
		else if (tree[local_tc] == TIdenttoaddr && tree[local_tc+1] == id)
			return 0;
		
		local_tc++;
	}
	return 1;
}

/**
 *	Проверить, является ли выражение константным в рамках цикла
 *	Это верно, если выражение НЕ содержит:
 *		- вызовов функций
 *		- вырезок
 *		- идентификаторов, значение которых может измениться в цикле
 */
int check_constant_expression(const int start_tc, const int end_tc)
{
	int local_tc = start_tc;
	while (local_tc != end_tc)
	{
		// FIXME: А еще эта функция написана не совсем честно
		// нужно пропускать "неузловые" числа, например, номер идентификатора, который идет после TIdent
		// Из-за этого могут быть неправильные ответы, если встретятся, например, константы -340 и т.д.
		// Но надеюсь, что этот баг не найдут до перехода на полноценное дерево, когда мне придется весь код писать заново
		switch (tree[local_tc])
		{
			// Честно говоря, тут не проверены функции из стандартной библиотеки RuC, но:
			//     - они не поддерживаются в этой ветке на данный момент
			//     - когда-нибудь потом их появление в дереве будет заменено на TCall
			case TCall1:
			case TSliceident:
				return 0;
			
			case TIdent:
				// Если значение идентификатора не может измениться, то продолжаем
				if (check_constant_identifier(tree[local_tc])) break;
				// Иначе возвращаем 0
				else return 0;
				
			default:
				break;
		}
		
		local_tc++;
	}
}

/**
 *	Проверить, можно ли посчитать условие окончания цикла заранее
 *	В таком случае привести условие к каноническому виду
 *	Функция гарантирует, что tc будет указывать на начало следующего выражения
 */
int check_controlling_expression(int start_tc, int end_tc)
{
	// Здесь нам интересны только relational expression
	// Тут прямая польская запись, так что на первом месте должен быть оператор сравнения
	switch (tree[start_tc])
	{
		// Если это выражение сравнения, то мы его хотим
		case EQEQ + 1000:
		case NOTEQ + 1000:
		case LLT + 1000:
		case LLE + 1000:
		case LGT + 1000:
		case LGE + 1000:
			if (tree[start_tc+1] == TIdent && tree[start_tc+2] == iterator && tree[start_tc+3] == TExprend)
			{
				// Это если слева от оператора сравнения стоит индуктивная переменная
				// Разворачивать условие не нужно, просто проверить правую часть
				// Копируем все без изменений
				while (tc != end_tc) mcopy();
				// А результат выдаст эта функция
				return check_constant_expression(start_tc+4, end_tc-1);
			}
			else if (tree[end_tc-3] == TIdent && tree[end_tc-2] == iterator && tree[end_tc-1] == TExprend)
			{
				// Это если справа от оператора сравнения стоит индуктивная переменная
				// Разворачиваем оператор
				switch (tree[start_tc])
				{
					case EQEQ + 1000:
					case NOTEQ + 1000:
						mtotree(tree[start_tc]);
						break;
					case LLT + 1000:
						mtotree(LGT + 1000);
						break;
					case LLE + 1000:
						mtotree(LGE + 1000);
						break;
					case LGT + 1000:
						mtotree(LLT + 1000);
						break;
					case LGE + 1000:
						mtotree(LLE + 1000);
						break;
				}
				
				// Ставим в левую часть индуктивную переменную
				mtotree(TIdent); mtotree(iterator); mtotree(TExprend);
				
				// Бывшую левую часть без изменений копируем в правую часть
				while (tc != end_tc-3) mcopy();
				// А результат выдаст эта функция
				return check_constant_expression(start_tc+1, end_tc-3);
			}
			
		// Если это не выражение сравнения, то мы его не оптимизируем
		default:
			while (tc != end_tc) mcopy();
			return 0;
	}
}

/**
 *	Проверить, подходит ли третья часть выражения для индуцированных переменных
 *	Подходят только i++, i--, ++i, --i, i += const, i -= const
 *	Функция гарантирует, что tc будет указывать на начало следующего выражения
 */
int check_iterator_expression(int start_tc, int end_tc)
{
	int result;
	switch (tree[start_tc])
	{
		case POSTINC:
		case POSTDEC:
		case INC:
		case DEC:
			if (tree[start_tc+1] == TIdent && tree[start_tc+3] == TExprend)
			{
				int op = (tree[start_tc] == POSTINC || tree[start_tc] == INC) ? INC : DEC;
				iterator = tree[start_tc+2];
				step = op == INC ? 1 : -1;
				result = 1;
			}
			break;

		case PLUSASS + 1000:
		case MINUSASS + 1000:
			if (tree[start_tc+1] == TIdent && tree[start_tc+3] == TConst && tree[start_tc+5] == TExprend)
			{
				iterator = tree[start_tc+2];
				step = tree[start_tc+4] * (tree[start_tc] == PLUSASS + 1000 ? 1 : -1);
				result = 1;
			}

		default:
			result = 0;
			break;
	}

	// Копируем без изменений
	while (tc != end_tc) mcopy();
	return result;
}

/**
 *	Проверяет, подходит ли вырезка для индуцированной переменной
 *	Возвращает длину вырезки, если подходит, иначе 0
 */
int check_slice_expression(const int slice_start)
{
	int i = slice_start;
	int was_iterator = 0;

	// Для начала нужно проверить, что это вырезка из массива
	if (tree[i] != TSliceident)
		return 0;

	// Тут много избыточного кода, но сейчас не до этого
	i += 3;
	// Если это вырезка константой a[const]
	if (tree[i] == TConst && tree[i+2] == TExprend)
		i += 3;
	// Если это вырезка идентификатором a[identifier]
	else if (tree[i] == TIdenttoval && tree[i+2] == TExprend)
	{
		// Проверка постоянства tree[i+1] или равенство итератору
		if (tree[i+1] == iterator)
		{
			if (was_iterator)
				return 0;
			else
				was_iterator = 1;
		}
		else if (!check_constant_identifier(tree[i+1]))
			return 0;

		i += 3;
	}
	// Если это вырезка выражением a[identifier ± const]
	else if ((tree[i] == LPLUS + 1000 || tree[i] == LMINUS + 1000)
			 && tree[i+1] == TIdenttoval && tree[i+3] == TConst && tree[i+5] == TExprend)
	{
		// Проверка постоянства tree[i+1] или равенство итератору
		if (tree[i+1] == iterator)
		{
			if (was_iterator)
				return 0;
			else
				was_iterator = 1;
		}
		else if (!check_constant_identifier(tree[i+1]))
			return 0;

		i += 6;
	}
	else
		return 0;

	// Здесь мы не проверяем правильность постоения дерева, доверяем парсеру
	while (tree[i] == TSlice)
	{
		// Пропускаем поля с доп. информацией
		i += 2;

		// Если это вырезка константой a[const]
		if (tree[i] == TConst && tree[i+2] == TExprend)
			i += 3;
		// Если это вырезка идентификатором a[identifier]
		else if (tree[i] == TIdenttoval && tree[i+2] == TExprend)
		{
			// Проверка постоянства tree[i+1] или равенство итератору
			if (tree[i+1] == iterator)
			{
				if (was_iterator)
					return 0;
				else
					was_iterator = 1;
			}
			else if (!check_constant_identifier(tree[i+1]))
				return 0;

			i += 3;
		}
		// Если это вырезка выражением a[identifier ± const]
		if ((tree[i] == LPLUS + 1000 || tree[i] == LMINUS + 1000)
				 && tree[i+1] == TIdenttoval && tree[i+3] == TConst && tree[i+5] == TExprend)
		{
			// Проверка постоянства tree[i+1] или равенство итератору
			if (tree[i+1] == iterator)
			{
				if (was_iterator)
					return 0;
				else
					was_iterator = 1;
			}
			else if (!check_constant_identifier(tree[i+1]))
				return 0;

			i += 6;
		}
		else
			return 0;
	}

	return i - slice_start;
}

void optimize_for_statement()
{
	int has_nested_for = 1;
	int nice_condition = 0;
	int nice_increment = 0;
	for_start = mcopy(); // TFor

	if (check_nested_for)
	{
		mark_nested_for();
		has_nested_for = 1 - tree[tc];
		mcopy();
	}

	int inition	  = mcopy();
	int condition = mcopy();
	int increment = mcopy();
	int statement = mcopy();

	if (inition)
		// Копируем первую часть условия for без изменений
		do mcopy(); while (tree[tc] != TExprend);

	if (condition)
		// Проверяем, вычислимо ли условие цикла перед циклом
		// Функция гарантирует, что tc будет указывать на начало следующего выражения
		nice_condition = check_controlling_expression(condition, increment);

	if (increment)
		// Проверяем, подходит ли выражение-инкремент для оптимизации индуцированных переменных
		// Функция гарантирует, что tc будет указывать на начало следующего выражения
		nice_increment = check_iterator_expression(increment, statement);

	// Начало блока for, т.е. место, куда будут вписываться узлы объявлений
	int for_scope_start = tree[tc] == TBegin ? tc+1: tc;
	
	// Ставим в дерево флажок, что можно оптимизировать условие
	if (cycle_condition_calculation && nice_condition) tree[for_start+1] = 2;
	// Оптимизация индуцированных переменных
	if (enable_ind_var && !has_nested_for && nice_increment)
	{
		ind_vars_counter = 3; ind_vars_start = 1;
		for (int local_tc = tc; tree[local_tc] != TForEnd; local_tc++)
		{
			if (tree[local_tc] != TSliceident)
			{
				continue;
			}

			int slice_length = check_slice_expression(local_tc);
			if (slice_length > 0)
			{
				// Добавляем в массив переменных вырезку
				int ind_var_number = ind_var_add(&tree[local_tc], slice_length);
				// Если такой еще не было
				if (ind_vars[ind_var_number] == 0)
				{
					// То добавляем узел TIndVar с этой вырезкой в mtree
					mtotree(TIndVar);
					mtotree(ind_var_number);
					mtotree(step); // TODO: подсчет шага
					// Копируем выражение для вырезки
					for (int i = 0; i < slice_length; i++)
					{
						mtotree(tree[local_tc + i]);
					}
					// Добавляем в конец узла TIndVar узел TExprend как признак конца
					//insert_tree(tc_stmtref + 3 + slice_length, TExprend);
					// Помечаем, что этот узел уже был
					ind_vars[ind_var_number] = 1;
					
					index_correction += 3 + slice_length;
				}
				
				// Заменяем выражение для вырезки индуцированной переменной
				// Делаем это в изначальном дереве, потом узлы TSkip пропустятся при копировании
				tree[local_tc] = TSliceInd;
				tree[local_tc + 1] = ind_var_number;
				for (int i = 2; i < slice_length; i++)
					tree[local_tc + i] = TSkip;
			}
		}
	}
	
	// Редукция индуктивной переменной
	// Делаем эту оптимизацию только если можно посчитать условие заранее
	if (tree[for_start+check_nested_for] == 2)
	{
		// М.б. стоит в другом порядке, но тут просто для наглядности
		tree[for_start+check_nested_for] = 3;
		// Идем по телу цилка и смотрим, используется ли она
		int local_tc = tree[for_start+check_nested_for+4];
		while (tree[local_tc] != TForEnd)
		{
			if ((tree[local_tc] == TIdent || tree[local_tc] == TIdenttoval || tree[local_tc] == TIdenttoaddr)
				&& tree[local_tc+1] == iterator)
			{
				// Нельзя редуцировать переменную
				tree[for_start+check_nested_for] = 2;
				break;
			}
		}
	}
	
	// Просто копируем тело цикла
	do mcopy(); while (tree[tc] != TForEnd);
}


/** Применить оптимизации к дереву */
void optimize()
{
	tree_size = tc;

	// Это начальное заполнение таблицы индуцированных переменных
	// На самом деле это костыль, но лень пока придумывать что-то другое
	ind_vars[1] = 0;
	ind_vars[2] = 0;
	ind_vars[3] = TExprend;

	// В рамках данной работы оптимизируем только циклы for
	for (tc = 0; tc < tree_size; tc++)
		if (tree[tc] == TFor)
			optimize_for_statement();
		else
			mcopy();
}
