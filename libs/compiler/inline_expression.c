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

#include "inline_expression.h"

node create_printf_node_by_vector(builder *bldr, const vector *str, node_vector *args, location l_loc, location r_loc);
node build_printf_expression(builder *const bldr, node *const callee, node_vector *const args, const location r_loc);

static node copy_argument_node(builder *bldr, node *argument, location l_loc, location r_loc);

static char *string_append(const char *s1, const char *s2)
{
	size_t s1_len = strlen(s1);
	size_t s2_len = strlen(s2);

	size_t size = s1_len + s2_len + 1;

	char *s = calloc(size, sizeof(char));
	if (!s)
	{
		return NULL;
	}

	for (size_t i = 0; i < s1_len; i++)
	{
		s[i] = s1[i];
	}

	for (size_t i = 0; i < s2_len; i++)
	{
		s[s1_len + i] = s2[i];
	}

	s[size - 1] = '\0';

	return s;
}

static char *string_copy(char *src)
{
	size_t size = strlen(src) + 1;
	char *s = calloc(size, sizeof(char));
	if (!s)
	{
		return NULL;
	}

	for (size_t i = 0; i < size - 1; i++)
	{
		s[i] = src[i];
	}

	s[size - 1] = '\0';
	return s;
}

static void concat_strings(char **dst, const char *src)
{
	char *tmp = string_append(*dst, src);
	free(*dst);
	if (!tmp)
	{
		*dst = NULL;
		return;
	}
	*dst = string_copy(tmp);
	free(tmp);
}

static char *create_new_temp_identifier_name(size_t ident_table_size)
{
	size_t ident_table_digits = 1;
	size_t tmp = ident_table_size;
	while (tmp /= 10)
	{
		ident_table_digits++;
	}

	char *new_identifier_number_str = calloc(ident_table_digits + 1, sizeof(char));
	if (!new_identifier_number_str)
	{
		return NULL;
	}
	sprintf(new_identifier_number_str, "%zu", (size_t)ident_table_size);

	char *new_identifier_name = calloc(1, sizeof(char));
	if (!new_identifier_name)
	{
		free(new_identifier_number_str);
		return NULL;
	}

	concat_strings(&new_identifier_name, "_temporal_identifier_");
	if (!new_identifier_name)
	{
		free(new_identifier_number_str);
		return NULL;
	}

	concat_strings(&new_identifier_name, new_identifier_number_str);
	free(new_identifier_number_str);
	if (!new_identifier_name)
	{
		return NULL;
	}

	concat_strings(&new_identifier_name, "_");
	if (!new_identifier_name)
	{
		return NULL;
	}

	return new_identifier_name;
}

static node create_printf_node(builder *bldr, node_vector *args, location loc, size_t str_index)
{
	node str_node = expression_string_literal(&bldr->context, type_string(bldr->sx), str_index, loc);

	// создаём новый вектор узлов, чтобы первым аргументом был форматный строковый литерал
	node_vector tmp_node_vector = node_vector_create();
	node_vector_add(&tmp_node_vector, &str_node);

	const size_t argc = node_vector_size(args);
	for (size_t i = 0; i < argc; i++)
	{
		node argument = node_vector_get(args, i);
		node_vector_add(&tmp_node_vector, &argument);
	}

	node printf_callee =
		expression_identifier(&bldr->context, type_function(bldr->sx, TYPE_INTEGER, "s."), BI_PRINTF, loc);

	// разворачиваемся в вызов printf
	return build_printf_expression(bldr, &printf_callee, &tmp_node_vector, loc);
}

static node create_printf_node_by_str(builder *bldr, const char *str, node_vector *args, location l_loc, location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	// создаём строку и вносим её в вектор
	size_t str_index = string_add_by_char(bldr->sx, str);

	return create_printf_node(bldr, args, loc, str_index);
}

static node create_consec_subscripts(builder *bldr, node *argument, size_t dimensions, vector idents, location l_loc,
									 location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	// создание идёт от той вырезки, базой для которой является исходный аргумент,
	// базой следующей вырезки является только что созданная вырезка и т.д.
	node curr_subscr_arg = *argument;

	for (size_t i = 0; i < dimensions; i++)
	{
		// вырезку берём по соответствующему идентификатору
		node sub_subscript_index_expr =
			expression_identifier(&bldr->context, TYPE_INTEGER, vector_get(&idents, i), loc);

		node sub_subscript_expr =
			expression_subscript(type_array_get_element_type(bldr->sx, expression_get_type(&curr_subscr_arg)),
								 &curr_subscr_arg, &sub_subscript_index_expr, loc);

		curr_subscr_arg = sub_subscript_expr;
	}
	return curr_subscr_arg;
} 

static node copy_subscript_node(builder *bldr, node *argument, location l_loc, location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	// берём текущий индекс вырезки
	node index_curr = expression_subscript_get_index(argument);
	// копируем его
	node index_copy = copy_argument_node(bldr, &index_curr, l_loc, r_loc);
	if (!node_is_correct(&index_copy))
	{
		return node_broken();
	}

	// берём родителя переданного аргумента
	node base_curr = expression_subscript_get_base(argument);
	// копируем его узел
	node base_copy = copy_argument_node(bldr, &base_curr, l_loc, r_loc);
	if (!node_is_correct(&base_copy))
	{
		return node_broken();
	}

	// return build_subscript_expression(bldr, &base_copy, &index_copy, l_loc, r_loc);
	return expression_subscript(expression_get_type(&base_copy), &base_copy, &index_copy, loc);
}

static node copy_identitfier_node(builder *bldr, node *argument, location loc)
{
	// берём индекс для переданного argument из identifiers table
	const size_t argument_id = expression_identifier_get_id(argument);

	return expression_identifier(&bldr->context, expression_get_type(argument), argument_id, loc);
}

static node copy_member_node(builder *bldr, node *argument, location l_loc, location r_loc)
{
	location loc = { l_loc.begin, r_loc.end };

	// берём родителя переданного аргумента
	node base_curr = expression_member_get_base(argument);
	// копируем его узел
	node base_copy = copy_argument_node(bldr, &base_curr, l_loc, r_loc);
	if (!node_is_correct(&base_copy))
	{
		return node_broken();
	}

	item_t type = expression_get_type(argument);

	size_t index = expression_member_get_member_index(argument);

	return expression_member(type, expression_is_lvalue(&base_copy) ? LVALUE : RVALUE, index, 0, &base_copy, loc);
}

static node copy_argument_node(builder *bldr, node *argument, location l_loc, location r_loc)
{
	location loc = { l_loc.begin, r_loc.end };

	switch (expression_get_class(argument))
	{
		case EXPR_IDENTIFIER:
			return copy_identitfier_node(bldr, argument, loc);
		case EXPR_MEMBER:
			return copy_member_node(bldr, argument, l_loc, r_loc);
		case EXPR_SUBSCRIPT:
			return copy_subscript_node(bldr, argument, l_loc, r_loc);
		default:
			return node_broken();
	}
}

static node create_struct_nodes(builder *bldr, node *argument, size_t tab_deep, location l_loc, location r_loc);

static node create_array_nodes(builder *bldr, node *argument, item_t type, location l_loc, location r_loc,
							   size_t dimensions, vector temp_idents_reprs)
{
	const location loc = { l_loc.begin, r_loc.end };

	// аргументы для тела цикла
	node_vector body_args = node_vector_create();

	item_t elements_type = type_array_get_element_type(bldr->sx, type);
	if (type_is_array(bldr->sx, elements_type))
	{
		// 1. инициализация -- объявляем новый уникальный идентификатор
		char *new_identifier_name = create_new_temp_identifier_name(vector_size(&bldr->sx->identifiers));
		if (!new_identifier_name)
		{
			return node_broken();
		}
		// добавляем имя нового идентификатора в таблицу representations
		const size_t repr = map_add(&bldr->sx->representations, new_identifier_name, ITEM_MAX);
		free(new_identifier_name);
		// добавляем идентификатор в identifiers
		const size_t id = ident_add(bldr->sx, repr, 0, TYPE_INTEGER, 3);

		node decl_stmt = statement_declaration(&bldr->context);
		node init_rhs_expr = expression_integer_literal(&bldr->context, TYPE_INTEGER, 0, loc);

		node_vector bounds = node_vector_create();
		node init_expr = declaration_variable(&decl_stmt, id, &bounds, &init_rhs_expr, loc);

		statement_declaration_add_declarator(&decl_stmt, &init_expr);

		// вносим его в вектор для дальнейшего использования
		if (vector_add(&temp_idents_reprs, id) == SIZE_MAX)
		{
			return node_broken();
		}

		// 2. условие
		node cond_lhs_expr = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);

		// узел, который отправится в build_unary_expression() оператора upb
		// для него создаём нужное количество "предварительных" вырезок
		node cond_subs_arg_expr = copy_argument_node(bldr, argument, l_loc, r_loc);
		if (!node_is_correct(&cond_subs_arg_expr))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		node cond_main_subs_expr =
			create_consec_subscripts(bldr, &cond_subs_arg_expr, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

		// оператор upb ("кол_во")
		node cond_rhs_expr = expression_unary(TYPE_INTEGER, RVALUE, &cond_main_subs_expr, UN_UPB, loc);

		node cond_expr = expression_binary(TYPE_BOOLEAN, &cond_lhs_expr, &cond_rhs_expr, BIN_LT, loc);

		// 3. инкремент
		node incr_lhs_expr = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);
		node incr_expr = expression_unary(TYPE_INTEGER, RVALUE, &incr_lhs_expr, UN_POSTINC, loc);

		// 4. тело цикла
		// печатаем "{"
		node_vector blank_tmp_node_vector = node_vector_create();
		node first_printf_node = create_printf_node_by_str(bldr, "{", &blank_tmp_node_vector, l_loc, r_loc);
		node_vector_add(&body_args, &first_printf_node);

		// печатаем массив
		node array_nodes =
			create_array_nodes(bldr, argument, elements_type, l_loc, r_loc, dimensions + 1, temp_idents_reprs);
		node_vector_add(&body_args, &array_nodes);

		// печатаем "}, " или "}", если цикл дальше не пойдёт

		// левая часть if
		node if_binary_lhs = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);

		// правая часть if: "upb(<массив по нужному измерению>) - 1"

		// главный узел, который отправится в build_unary_expression() оператора upb
		// для него создаём нужное количество "предварительных" вырезок
		node if_binary_rhs_main_subs_arg_expr = copy_argument_node(bldr, argument, l_loc, r_loc);
		if (!node_is_correct(&if_binary_rhs_main_subs_arg_expr))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		node if_binary_rhs_main_subs = create_consec_subscripts(bldr, &if_binary_rhs_main_subs_arg_expr, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

		// оператор upb ("кол_во")
		node if_binary_rhs_upb = expression_unary(TYPE_INTEGER, RVALUE, &if_binary_rhs_main_subs, UN_UPB, loc);

		node if_binary_rhs_num_one = expression_integer_literal(&bldr->context, TYPE_INTEGER, 1, loc);

		// оператор "минус"
		node if_binary_rhs = expression_binary(TYPE_INTEGER, &if_binary_rhs_upb, &if_binary_rhs_num_one, BIN_SUB, loc);

		// операция сравнения (равенства)
		node if_binary = expression_binary(TYPE_BOOLEAN, &if_binary_lhs, &if_binary_rhs, BIN_EQ, loc);

		// условие выполнено
		node if_true = create_printf_node_by_str(bldr, "}", &blank_tmp_node_vector, l_loc, r_loc);
		// условие не выполнено
		node if_false = create_printf_node_by_str(bldr, "}, ", &blank_tmp_node_vector, l_loc, r_loc);

		// само выражение
		node if_statement = statement_if(&if_binary, &if_true, &if_false, loc);
		node_vector_add(&body_args, &if_statement);

		node body = statement_compound(&bldr->context, &body_args, loc);

		// 5. полный узел цикла
		node for_stmt = statement_for(&decl_stmt, &cond_expr, &incr_expr, &body, loc);

		return for_stmt;
	}

	// проверяем, не пришёл ли массив-строка (на случай многомерных массивов char, иначе
	// будут отпечатаны слова через запятую)
	if (type_is_string(bldr->sx, type))
	{
		node subscript_node = create_consec_subscripts(bldr, argument, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

		node_vector arg = node_vector_create();
		node_vector_add(&arg, &subscript_node);

		node printf_node = create_printf_node_by_str(bldr, "%s", &arg, l_loc, r_loc);
		node_vector_add(&body_args, &printf_node);

		return build_compound_statement(bldr, &body_args, l_loc, r_loc);
	}

	// не пришёл => делаем цикл

	// 1. инициализация -- объявляем новый уникальный идентификатор
	char *new_identifier_name = create_new_temp_identifier_name(vector_size(&bldr->sx->identifiers));
	if (!new_identifier_name)
	{
		return node_broken();
	}
	// добавляем имя нового идентификатора в таблицу representations
	const size_t repr = map_add(&bldr->sx->representations, new_identifier_name, ITEM_MAX);
	free(new_identifier_name);
	// добавляем идентификатор в identifiers
	const size_t id = ident_add(bldr->sx, repr, 0, TYPE_INTEGER, 3);

	node decl_stmt = statement_declaration(&bldr->context);
	node init_rhs_expr = expression_integer_literal(&bldr->context, TYPE_INTEGER, 0, loc);

	node_vector bounds = node_vector_create();
	node init_expr = declaration_variable(&decl_stmt, id, &bounds, &init_rhs_expr, loc);

	statement_declaration_add_declarator(&decl_stmt, &init_expr);

	// вносим его в вектор для дальнейшего использования
	if (vector_add(&temp_idents_reprs, id) == SIZE_MAX)
	{
		return node_broken();
	}

	// 2. условие
	// node cond_lhs_expr = build_identifier_expression(bldr, repr, loc);
	node cond_lhs_expr = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);

	// главный узел, который отправится в build_unary_expression() оператора upb
	// для него создаём нужное количество "предварительных" вырезок
	node cond_main_subs_arg_expr = copy_argument_node(bldr, argument, l_loc, r_loc);
	if (!node_is_correct(&cond_main_subs_arg_expr))
	{ 
		// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
		return node_broken();
	}
	node cond_main_subs_expr =
		create_consec_subscripts(bldr, &cond_main_subs_arg_expr, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

	// оператор upb ("кол_во")
	node cond_rhs_expr = expression_unary(TYPE_INTEGER, RVALUE, &cond_main_subs_expr, UN_UPB, loc);

	node cond_expr = expression_binary(TYPE_BOOLEAN, &cond_lhs_expr, &cond_rhs_expr, BIN_LT, loc);

	// 3. инкремент
	node incr_lhs_expr = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);
	node incr_expr = expression_unary(TYPE_INTEGER, RVALUE, &incr_lhs_expr, UN_POSTINC, loc);

	// 4. тело цикла (зависит от класса типа элементов массива)
	item_t type_class = type_get_class(bldr->sx, elements_type);
	if (type_class != TYPE_STRUCTURE)
	{
		// разворачиваемся в узел if, в котором будет printf, для этого строим строки для каждого из случаев
		vector str = vector_create(1);

		const char *tmp = create_simple_type_str(type_class);
		if (!tmp)
		{
			vector_clear(&str);
			return node_broken();
		}
		if (!vector_add_str(&str, tmp))
		{
			return node_broken();
		}

		// создаём узлы вырезки, которые отправятся в качестве аргумента для printf
		// узлов должно быть два, т.к. будем разворачиваться в if statement

		// создаём новый узел аргумента
		node curr_subscr_arg1 = copy_argument_node(bldr, argument, l_loc, r_loc);
		if (!node_is_correct(&curr_subscr_arg1))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		// в качестве аргумента для create_printf_node() отправятся только последние по порядку создания узлы
		node main_subscript1 =
			create_consec_subscripts(bldr, &curr_subscr_arg1, dimensions, temp_idents_reprs, l_loc, r_loc);
		node_vector tmp_args1 = node_vector_create();
		node_vector_add(&tmp_args1, &main_subscript1);

		// создаём новый узел аргумента
		node curr_subscr_arg2 = copy_argument_node(bldr, argument, l_loc, r_loc);
		if (!node_is_correct(&curr_subscr_arg2))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		// в качестве аргумента для create_printf_node() отправятся только последние по порядку создания узлы
		node main_subscript2 =
			create_consec_subscripts(bldr, &curr_subscr_arg2, dimensions, temp_idents_reprs, l_loc, r_loc);
		node_vector tmp_args2 = node_vector_create();
		node_vector_add(&tmp_args2, &main_subscript2);

		// левая часть if
		node if_binary_lhs = expression_identifier(&bldr->context, TYPE_INTEGER, id, loc);

		// правая часть if: "upb(<массив по нужному измерению>) - 1"

		// главный узел, который отправится в build_unary_expression() оператора upb
		// для него создаём нужное количество "предварительных" вырезок
		node if_binary_rhs_main_subs =
			create_consec_subscripts(bldr, argument, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

		// оператор upb ("кол_во")
		node if_binary_rhs_upb = expression_unary(TYPE_INTEGER, RVALUE, &if_binary_rhs_main_subs, UN_UPB, loc);

		node if_binary_rhs_num_one = expression_integer_literal(&bldr->context, TYPE_INTEGER, 1, loc);

		// оператор "минус"
		node if_binary_rhs = expression_binary(TYPE_INTEGER, &if_binary_rhs_upb, &if_binary_rhs_num_one, BIN_SUB, loc);

		// операция сравнения (равенства)
		node if_binary = expression_binary(TYPE_BOOLEAN, &if_binary_lhs, &if_binary_rhs, BIN_EQ, loc);

		// разворачиваемся в узлы printf, чтобы отпечатать "%<нужный идентификатор>, " или
		// "%<нужный идентификатор>", если цикл дальше не пойдёт
		// условие выполнено
		node if_true = create_printf_node_by_vector(bldr, &str, &tmp_args1, l_loc, r_loc);
		// условие не выполнено
		if (!vector_add_str(&str, ", "))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		node if_false = create_printf_node_by_vector(bldr, &str, &tmp_args2, l_loc, r_loc);
		vector_clear(&str);

		// само выражение
		node if_statement = statement_if(&if_binary, &if_true, &if_false, loc);

		node_vector_add(&body_args, &if_statement);

		node body = statement_compound(&bldr->context, &body_args, loc);

		// 5. полный узел цикла
		node for_stmt = statement_for(&decl_stmt, &cond_expr, &incr_expr, &body, loc);

		return for_stmt;
	}
	else
	{
		// 5. полный узел цикла

		// создаём узел структуры как корректную вырезку из массива
		node tmp_arg = copy_argument_node(bldr, argument, l_loc, r_loc);
		if (!node_is_correct(&tmp_arg))
		{
			// для temp_idents_reprs будет сделан clear по выходу из create_array_nodes() в любом случае
			return node_broken();
		}
		node subs_struct_node = create_consec_subscripts(bldr, &tmp_arg, dimensions, temp_idents_reprs, l_loc, r_loc);

		// разворачиваемся в узел printf, чтобы отпечатать "\n{ struct"
		node_vector blank_tmp_node_vector = node_vector_create();
		node first_printf_node = create_printf_node_by_str(bldr, "\n{ struct", &blank_tmp_node_vector, l_loc, r_loc);
		node_vector_add(&body_args, &first_printf_node);

		// отправляем нужный узел в create_struct_nodes()
		node struct_nodes = create_struct_nodes(bldr, &subs_struct_node, 1, l_loc, r_loc);
		node_vector_add(&body_args, &struct_nodes);

		// if-statement: отпечатываем запятую после каждого элемента массива, кроме последнего

		// левая часть if
		node if_binary_lhs =
			expression_identifier(&bldr->context, TYPE_INTEGER, vector_get(&temp_idents_reprs, dimensions - 1), loc);

		// правая часть if: "upb(<массив по нужному измерению>) - 1"

		// главный узел, который отправится в build_unary_expression() оператора upb
		// для него создаём нужное количество "предварительных" вырезок по нулевому индексу
		node if_binary_rhs_main_subs =
			create_consec_subscripts(bldr, argument, dimensions - 1, temp_idents_reprs, l_loc, r_loc);

		// оператор upb ("кол_во")
		node if_binary_rhs_upb = expression_unary(TYPE_INTEGER, RVALUE, &if_binary_rhs_main_subs, UN_UPB, loc);

		node if_binary_rhs_num_one = expression_integer_literal(&bldr->context, TYPE_INTEGER, 1, loc);

		// оператор "минус"
		node if_binary_rhs = expression_binary(TYPE_INTEGER, &if_binary_rhs_upb, &if_binary_rhs_num_one, BIN_SUB, loc);

		// операция сравнения (равенства)
		node if_binary = expression_binary(TYPE_BOOLEAN, &if_binary_lhs, &if_binary_rhs, BIN_EQ, loc);

		// условие выполнено
		node if_true = create_printf_node_by_str(bldr, "\n}", &blank_tmp_node_vector, l_loc, r_loc);
		// условие не выполнено
		node if_false = create_printf_node_by_str(bldr, "}, ", &blank_tmp_node_vector, l_loc, r_loc);

		// само выражение
		node if_statement = statement_if(&if_binary, &if_true, &if_false, loc);
		node_vector_add(&body_args, &if_statement);

		node body = statement_compound(&bldr->context, &body_args, loc);

		// 5. полный узел цикла
		node for_stmt = statement_for(&decl_stmt, &cond_expr, &incr_expr, &body, loc);

		return for_stmt;
	}
}

static node create_ptr_nodes(builder *bldr, node *argument, location l_loc, location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	node arg_copy = copy_argument_node(bldr, argument, l_loc, r_loc); // не NULL_POINTER => аргумент -- идентификатор
	if (!node_is_correct(&arg_copy))
	{
		return node_broken();
	}

	node cond = expression_unary(TYPE_BOOLEAN, RVALUE, &arg_copy, UN_LOGNOT, loc);

	node_vector blank_tmp_nv = node_vector_create();
	node if_true = create_printf_node_by_str(bldr, "NULL", &blank_tmp_nv, l_loc, r_loc);

	node_vector tmp_nv = node_vector_create();

	node_vector_add(&tmp_nv, argument);
	node if_false = create_printf_node_by_str(bldr, "%p", &tmp_nv, l_loc, r_loc);

	return statement_if(&cond, &if_true, &if_false, loc);
} 

static bool create_correct_spaces(vector *str, size_t tab_deep)
{
	// глубина одного уровня -- 4 пробела
	for (size_t j = 0; j < tab_deep * 4; j++)
	{
		vector_add_str(str, " ");
		if (!vector_size(str))
		{
			return 0;
		}
	}
	return 1;
}

static node create_struct_nodes(builder *bldr, node *argument, size_t tab_deep, location l_loc, location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	// будет statement compound'ом
	node result;

	node_vector res_stmts = node_vector_create();

	node_vector args_to_print = node_vector_create();

	item_t type = expression_get_type(argument);
	const size_t member_amount = type_structure_get_member_amount(bldr->sx, type);

	// т.к. в качестве аргумента может придти вырезка из массива, идентификатор, или поле структуры -- объявляем новый
	// идентификатор, который будет проинициализирован аргументом
	char *new_arg_identifier_name = create_new_temp_identifier_name(vector_size(&bldr->sx->identifiers));
	if (!new_arg_identifier_name)
	{
		return node_broken();
	}
	// добавляем имя нового идентификатора в таблицу representations
	const size_t arg_repr = map_add(&bldr->sx->representations, new_arg_identifier_name, ITEM_MAX);
	free(new_arg_identifier_name);
	// добавляем идентификатор в identifiers
	const size_t arg_id = ident_add(bldr->sx, arg_repr, 0, type, 3);

	node decl_stmt = statement_declaration(&bldr->context);

	node_vector bounds = node_vector_create();
	node init_expr = declaration_variable(&decl_stmt, arg_id, &bounds, argument, loc);

	statement_declaration_add_declarator(&decl_stmt, &init_expr);

	node_vector_add(&res_stmts, &decl_stmt);

	vector str = vector_create(1);

	for (size_t i = 0; i < member_amount; i++)
	{
		item_t member_type = type_structure_get_member_type(bldr->sx, type, i);
		size_t member_name = type_structure_get_member_name(bldr->sx, type, i);
		const char *member_name_str = repr_get_name(bldr->sx, member_name);

		// строим узел нового аргумента
		node tmp_arg = expression_identifier(&bldr->context, type, arg_id, loc);
		// строим узел поля структуры
		node member_node = expression_member(member_type, LVALUE, i, 0, &tmp_arg, loc);

		// создаём корректное начало строки
		if (!vector_add_str(&str, "\n\n"))
		{
			return node_broken();
		}

		if (!create_correct_spaces(&str, tab_deep))
		{
			return node_broken();
		}

		if (!vector_add_str(&str, "{\n"))
		{
			return node_broken();
		}

		if (!create_correct_spaces(&str, tab_deep + 1))
		{
			return node_broken();
		}

		if (!vector_add_str(&str, "."))
		{
			return node_broken();
		}

		if (!vector_add_str(&str, member_name_str))
		{
			return node_broken();
		}

		if (!vector_add_str(&str, " = "))
		{
			return node_broken();
		}

		if ((type_is_aggregate(bldr->sx, member_type) && !type_is_string(bldr->sx, member_type)) ||
			(type_is_pointer(bldr->sx, member_type)) || (type_is_null_pointer(member_type)))
		{
			if (type_is_array(bldr->sx, member_type))
			{
				if (!vector_add_str(&str, "{"))
				{
					return node_broken();
				}
			}
			else if (type_is_structure(bldr->sx, member_type))
			{
				if (!vector_add_str(&str, "{ struct"))
				{
					return node_broken();
				}
			}

			// разворачиваемся в printf для имеющейся на данный момент строки
			node printf_node = create_printf_node_by_vector(bldr, &str, &args_to_print, r_loc, l_loc);
			vector_clear(&str);
			// запоминаем узел
			node_vector_add(&res_stmts, &printf_node);

			// дальнейшая строка и аргументы не будут иметь к только что построенному узлу никакого отношения
			str = vector_create(1);
			// избавляемся от предыдущих запомненных аргументов
			node_vector_clear(&args_to_print);
			args_to_print = node_vector_create();

			node complicated_type_node = create_complicated_type_str(bldr, &member_node, l_loc, r_loc, tab_deep + 1);
			if (!node_is_correct(&complicated_type_node))
			{
				vector_clear(&str);
				return complicated_type_node;
			}
			node_vector_add(&res_stmts, &complicated_type_node);

			if (type_is_array(bldr->sx, member_type))
			{
				if (!vector_add_str(&str, "} "))
				{
					return node_broken();
				}
			}
			else if (type_is_structure(bldr->sx, member_type))
			{
				if (!vector_add_str(&str, "}\n"))
				{
					return node_broken();
				}
			}
		}
		else
		{
			node_vector_add(&args_to_print, &member_node);

			const item_t member_type_class = type_get_class(bldr->sx, member_type);
			const char *tmp = create_simple_type_str(member_type_class);
			if (!tmp)
			{
				vector_clear(&str);
				return node_broken();
			}
			if (!vector_add_str(&str, tmp))
			{
				return node_broken();
			}
		}
		// правильно завершаем строку: если текущее поле, которое требуется распечатать, -- последнее, то запятую не
		// ставим, иначе -- ставим
		if (!vector_add_str(&str, "\n"))
		{
			return node_broken();
		}

		create_correct_spaces(&str, tab_deep);

		if (i == member_amount - 1)
		{
			if (!vector_add_str(&str, "}"))
			{
				return node_broken();
			}
		}
		else
		{
			if (!vector_add_str(&str, "},"))
			{
				return node_broken();
			}
		}
	}

	// разворачиваемся в printf для имеющейся на данный момент строки
	if (vector_size(&str))
	{
		node printf_node = create_printf_node_by_vector(bldr, &str, &args_to_print, l_loc, r_loc);
		vector_clear(&str);
		node_vector_add(&res_stmts, &printf_node);
	}

	result = statement_compound(&bldr->context, &res_stmts, loc);
	return result;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


node create_printf_node_by_vector(builder *bldr, const vector *str, node_vector *args, location l_loc, location r_loc)
{
	const location loc = { l_loc.begin, r_loc.end };

	// создаём строку и вносим её в вектор
	size_t str_index = strings_add_by_vector(&bldr->sx->string_literals, str);

	return create_printf_node(bldr, args, loc, str_index);
}


bool vector_add_str(vector *dst, const char *src)
{
	size_t i = 0;
	while (src[i] != '\0')
	{
		char32_t ch = utf8_convert(&src[i]);
		if (vector_add(dst, ch) == SIZE_MAX)
		{
			return 0;
		}
		i += utf8_symbol_size(src[i]);
	}
	return 1;
}


const char *create_simple_type_str(item_t type)
{
	switch (type)
	{
		case TYPE_BOOLEAN:
		case TYPE_INTEGER:
			return "%i";
		case TYPE_CHARACTER:
			return "%c";
		case TYPE_FLOATING:
			return "%f";
		case TYPE_ARRAY: // попали сюда => в качестве массива может выступать только строка => "%s"
			return "%s";
		case TYPE_NULL_POINTER:
			return "NULL";
	}
	return 0;
}


node create_complicated_type_str(builder *bldr, node *argument, location l_loc, location r_loc, size_t tab_deep)
{
	node complicated_type_node;
	item_t argument_type = expression_get_type(argument);
	if (type_is_array(bldr->sx, argument_type))
	{
		size_t dimensions = 0;
		item_t elements_type = argument_type;
		while (type_is_array(bldr->sx, elements_type))
		{
			elements_type = type_array_get_element_type(bldr->sx, elements_type);
			dimensions++;
		}

		vector idents = vector_create(dimensions);
		complicated_type_node = create_array_nodes(bldr, argument, argument_type, l_loc, r_loc, 1, idents);
		vector_clear(&idents);
	}
	else if (type_is_structure(bldr->sx, argument_type))
	{
		complicated_type_node = create_struct_nodes(bldr, argument, tab_deep, l_loc, r_loc);
	}
	else
	{
		complicated_type_node = create_ptr_nodes(bldr, argument, l_loc, r_loc);
	}

	return complicated_type_node;
}

