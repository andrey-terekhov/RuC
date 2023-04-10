/*
 *	Copyright 2016 Andrey Terekhov
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

#include "errors.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "commenter.h"
#include "item.h"
#include "logger.h"
#include "uniio.h"
#include "utf8.h"


#define TAG_RUC "ruc"

#define MAX_TAG_SIZE 128
#define MAX_MSG_SIZE MAX_TAG_SIZE * 4
#define MAX_LINE_SIZE MAX_TAG_SIZE * 4


static void get_error(const err_t num, char *const msg, va_list args)
{
	switch (num)
	{
		// Lexing errors
		case bad_character:
			sprintf(msg, "плохой символ");
			break;
		case digit_of_another_base:
			sprintf(msg, "цифра из другой системы счисления");
			break;
		case exponent_has_no_digits:
			sprintf(msg, "после экспоненты должна быть цифра");
			break;
		case empty_character_literal:
			sprintf(msg, "пустой символьный литерал");
			break;
		case unknown_escape_sequence:
			sprintf(msg, "неизвестная escape-последовательность");
			break;
		case missing_terminating_apost_char:
			sprintf(msg, "символьный литерал не заканчивается символом '");
			break;
		case missing_terminating_quote_char:
			sprintf(msg, "строковый литерал не заканчивается символом \"");
			break;
		case unterminated_block_comment:
			sprintf(msg, "блочный комментарий не окончен");
			break;

		// Syntax errors
		case extraneous_bracket_before_semi:
			sprintf(msg, "лишняя скобка перед ';'");
			break;
		case expected_r_paren:
			sprintf(msg, "ожидалась ')'");
			break;
		case expected_r_square:
			sprintf(msg, "ожидалась ']'");
			break;
		case expected_r_brace:
			sprintf(msg, "ожидалась '}'");
			break;
		case expected_expression:
			sprintf(msg, "ожидалось выражение");
			break;
		case expected_identifier_in_member_expr:
			sprintf(msg, "ожидался идентификатор в выражении выборки");
			break;
		case expected_colon_in_conditional_expr:
			sprintf(msg, "ожидалось ':' в условном операторе");
			break;
		case empty_initializer:
			sprintf(msg, "пустой инициализатор");
			break;
		case expected_l_paren_in_condition:
			sprintf(msg, "ожидалась '(' в условии");
			break;
		case case_not_in_switch:
			sprintf(msg, "метка 'случай' не в операторе 'выбор'");
			break;
		case default_not_in_switch:
			sprintf(msg, "метка 'умолчание' не в операторе 'выбор'");
			break;
		case expected_colon_after_case:
			sprintf(msg, "ожидалось ':' после выражения метки 'случай'");
			break;
		case expected_colon_after_default:
			sprintf(msg, "ожидалось ':' после метки 'умолчание'");
			break;
		case expected_semi_after_expr:
			sprintf(msg, "ожидалась ';' после выражения");
			break;
		case expected_semi_after_stmt:
			sprintf(msg, "ожидалась ';' после оператора");
			break;
		case expected_while:
			sprintf(msg, "ожидалось 'пока' в операторе 'цикл'");
			break;
		case expected_l_paren_after_for:
			sprintf(msg, "ожидалась '(' после 'для'");
			break;
		case expected_semi_in_for_specifier:
			sprintf(msg, "ожидалась ';' в спецификаторе оператора 'для'");
			break;
		case continue_not_in_loop:
			sprintf(msg, "оператор 'продолжить' не в цикле");
			break;
		case break_not_in_loop_or_switch:
			sprintf(msg, "оператор 'выход' не в цикле и не в операторе 'выбор'");
			break;

		case main_should_return_int_or_void:
			sprintf(msg, "главная функция должна возвращать цел или пусто");
			break;
		case main_should_be_defined:
			sprintf(msg, "главная функция должна быть описана");
			break;
		case wrong_main_parameters:
			sprintf(msg, "главная функция должна иметь не больше одного параметра");
			break;
		case wrong_main_parameter_type:
			sprintf(msg, "единственный параметр главной функции должен иметь тип char[][]");
			break;

		// Semantics errors
		case use_of_undeclared_identifier:
			sprintf(msg, "использование не объявленного идентификатора");
			break;
		case subscripted_expr_not_array:
			sprintf(msg, "вырезка элемента не из массива");
			break;
		case array_subscript_not_integer:
			sprintf(msg, "индекс элемента массива должен иметь целочисленный тип");
			break;
		case called_expr_not_function:
			sprintf(msg, "вызываемое выражение должно иметь функциональный тип");
			break;
		case wrong_argument_amount:
			sprintf(msg, "неверное число аргументов в вызове");
			break;
		case member_reference_not_struct:
			sprintf(msg, "оператор '.' применяется не к структуре");
			break;
		case member_reference_not_struct_pointer:
			sprintf(msg, "оператор '->' применяется не к указателю на структуру");
			break;
		case no_such_member:
			sprintf(msg, "нет такого поля в структуре");
			break;
		case unassignable_expression:
			sprintf(msg, "в это выражение нельзя присваивать");
			break;
		case increment_operand_not_arithmetic:
			sprintf(msg, "операнд инкремента или декремента должен иметь арифметический тип");
			break;
		case addrof_operand_not_lvalue:
			sprintf(msg, "операция получения адреса '&' применима только к lvalue");
			break;
		case indirection_operand_not_pointer:
			sprintf(msg, "операнд косвенного обращения '*' должен иметь тип указатель");
			break;
		case unary_operand_not_arithmetic:
			sprintf(msg, "операнд этого унарного оператора должен иметь арифметический тип");
			break;
		case unnot_operand_not_integer:
			sprintf(msg, "операнд оператора '~' должен иметь целочисленный тип");
			break;
		case lognot_operand_not_scalar:
			sprintf(msg, "операнд оператора '!' должен иметь скалярный тип");
			break;
		case upb_operand_not_array:
			sprintf(msg, "операнд 'upb' должен иметь тип массив");
			break;
		case typecheck_binary_expr:
			sprintf(msg, "неверные типы аргументов в бинарном выражении");
			break;
		case condition_must_be_scalar:
			sprintf(msg, "условие должно иметь скалярный тип");
			break;
		case expected_constant_expression:
			sprintf(msg, "ожидалось константное выражение");
			break;
		case incompatible_cond_operands:
			sprintf(msg, "несовместимые типы операндов условного оператора");
			break;

		case expected_member_name:
			sprintf(msg, "ожидалось имя поля структуры");
			break;
		case array_member_must_have_bounds:
			sprintf(msg, "поля-массивы должны иметь границы");
			break;
		case expected_identifier_in_declarator:
			sprintf(msg, "ожидалось имя объявляемой переменной");
			break;
		case declaration_does_not_declare_anything:
			sprintf(msg, "объявление ничего не объявляет");
			break;
		case case_expr_not_integer:
			sprintf(msg, "выражение оператора 'случай' должно иметь целочисленный тип");
			break;
		case switch_expr_not_integer:
			sprintf(msg, "выражение оператора 'выбор' должно иметь целочисленный тип");
			break;
		case void_func_valued_return:
			sprintf(msg, "функция не должна возвращать значение");
			break;
		case nonvoid_func_void_return:
			sprintf(msg, "функция должна возвращать значение");
			break;
		case assign_to_const:
			sprintf(msg, "нельзя присваивать значение константе");
			break;
		case invalid_const_pointer_cast:
			sprintf(msg, "нельзя преобразовать указатель на константу в указатель на переменную");
			break;
		case function_type_const:
			sprintf(msg, "тип функции не может быть константой");
			break;
		case const_without_init:
			sprintf(msg, "встречено объявление константы без инициализации");
			break;
		case reference_without_init:
			sprintf(msg, "встречено объявление переменной-ссылки без инициализации");
			break;
		case reference_to_not_lvalue:
			sprintf(msg, "Невозможно создать ссылку на выражение, не являющееся lvalue");
			break;

		// Builtin errors
		case too_many_printf_args:
			sprintf(msg, "максимально в 'printf' можно выводить %zu значений", va_arg(args, size_t));
			break;
		case expected_format_specifier:
			sprintf(msg, "ожидался спецификатор типа после '%%'");
			break;
		case unknown_format_specifier:
		{
			size_t index = (size_t)sprintf(msg, "неизвестный спецификатор %%");
			const char32_t bad_printf_specifier = va_arg(args, char32_t);
			index += utf8_to_string(&msg[index], bad_printf_specifier);
		}
		break;
		case printf_fst_not_string:
			sprintf(msg, "первым аргументом вызова 'printf' должен быть форматный строковый литерал");
			break;
		case wrong_printf_argument_amount:
			sprintf(msg, "количество аргументов вызова 'printf' не соответствует количеству спецификаторов");
			break;
		case wrong_printf_argument_type:
		{
			size_t index = (size_t)sprintf(msg, "тип аргумента вызова 'printf' не соответствует спецификатору: %%");
			const char32_t bad_printf_specifier = va_arg(args, char32_t);
			index += utf8_to_string(&msg[index], bad_printf_specifier);
			switch (bad_printf_specifier)
			{
				case 'i':
				case U'ц':
					sprintf(&msg[index], " ожидает целое число");
					break;
				case 'c':
				case U'л':
					sprintf(&msg[index], " ожидает литеру");
					break;
				case 'f':
				case U'в':
					sprintf(&msg[index], " ожидает вещественное число");
					break;
				case U'с':
				case 's':
					sprintf(&msg[index], " ожидает строку");
					break;
				default:
					sprintf(&msg[index], " -- неизвестный спецификатор");
					break;
			}
		}
		break;
		case pointer_in_print:
			sprintf(msg, "указатель не может быть операндом для печати");
			break;
		case expected_identifier_in_printid:
			sprintf(msg, "ожидался идентификатор в вызове 'printid'");
			break;
		case expected_identifier_in_getid:
			sprintf(msg, "ожидался идентификатор в вызове 'getid'");
			break;

		// Environment errors
		case no_main_in_program: // test_exist
			sprintf(msg, "в каждой программе должна быть ГЛАВНАЯ функция");
			break;
		case predef_but_notdef: // need_test
			sprintf(msg, "функция %s была предопределена, но не описана", va_arg(args, char *));
			break;

		// Other errors
		case after_type_must_be_ident: // test_exist
			sprintf(msg, "после символа типа должен быть идентификатор или * идентификатор");
			break;
		case wait_right_sq_br: // test_exist
			sprintf(msg, "ожидалась ]");
			break;
		case only_functions_may_have_type_VOID: // test_exist
			sprintf(msg, "только функции могут иметь тип ПУСТО");
			break;
		case decl_and_def_have_diff_type:	// test_exist
			sprintf(msg, "прототип функции и ее описание имеют разные типы");
			break;
		case wrong_param_list: // need_test
			sprintf(msg, "неправильный список параметров");
			break;
		case expected_semi_after_decl: // test_exist
			sprintf(msg, "список описаний должен заканчиваться ;");
			break;
		case typedef_requires_a_name:
			sprintf(msg, "нужен идентификатор после ключевого слова typedef");
			break;
		case func_decl_req_params: // need_test
			sprintf(msg, "вообще-то я думал, что это предописание функции (нет "
				"идентификаторов-параметров), а тут тело функции");
			break;
		case repeated_decl:	// test_exist
			sprintf(msg, "повторное описание идентификатора %s", va_arg(args, char *));
			break;
		case ident_is_not_declared: // test_exist
			sprintf(msg, "не описан идентификатор %s", va_arg(args, char *));
			break;
		case not_const_int_expr:
			sprintf(msg, "должно быть константное выражение типа int");
			break;
		case redefinition_of_main:	// test_exist
			sprintf(msg, "в программе может быть только 1 идентификатор ГЛАВНАЯ");
			break;
		case aster_before_func:	// need_test
			sprintf(msg, "* перед описанием функции");
			break;
		case aster_with_row:	// need_test
			sprintf(msg, "операцию * нельзя применять к массивам");
			break;
		case wrong_func_as_arg: // need_test
			sprintf(msg, "неправильная запись функции, передаваемой параметром в другую функцию");
			break;
		case no_right_br_in_arg_func: // need_test
			sprintf(msg, "нет ) в функции, передаваемой параметром в другую функцию");
			break;
		case par_type_void_with_nofun:	// need_test
			sprintf(msg, "в параметре функции тип пусто может быть только у параметра-функции");
			break;
		case ident_in_declarator:	// need_test
			sprintf(msg, "в деклараторах (предописаниях) могут быть только типы, но без идентификаторов-параметров");
			break;
		case array_before_func: // need_test
			sprintf(msg, "функция не может выдавать значение типа массив");
			break;
		case wait_definition: // need_test
			sprintf(msg, "вообще-то, я думал, что это определение функции, а тут нет идентификатора-параметра");
			break;
		case wait_declarator: // need_test
			sprintf(msg, "вообще-то, я думал, что это предописание функции, а тут идентификатор-параметр");
			break;
		case two_idents_for_1_declarer:	// need_test
			sprintf(msg, "в описании функции на каждый описатель должен быть один параметр");
			break;
		case function_has_no_body: // need_test
			sprintf(msg, "есть параметры определения функции, но нет блока, являющегося ее телом");
			break;
		case wrong_struct:	// test_exist
			sprintf(msg, "неправильное описание структуры");
			break;
		case wrong_init:	//test_exist
			sprintf(msg, "переменные такого типа нельзя инициализировать");
			break;
		case pnt_before_array:	// test_exist
			sprintf(msg, "в РуСи не бывает указателей на массивы");
			break;
		case array_size_must_be_int:	// test_exist
			sprintf(msg, "размер массива может иметь тип только ЦЕЛ или ЛИТЕРА");
			break;
		case no_semicolon_in_struct:	// need_test
			sprintf(msg, "описание поля структуры должно заканчиваться ;");
			break;
		case no_comma_in_enum:
			sprintf(msg, "описание поля перечисления должно заканчиваться ,");
			break;
		case wait_ident_after_semicolon_in_struct: // test_exist
			sprintf(msg, "в структуре после типа поля должен идти идентификатор поля");
			break;
		case wait_ident_after_comma_in_enum:
			sprintf(msg, "в перечислении должен быть идентификатор поля");
			break;
		case empty_init:	// test_exist
			sprintf(msg, "в РуСи можно определять границы массива по инициализации только по младшему измерению");
			break;
		case ident_not_type:	// test_exist
			sprintf(msg, "в качестве описателя можно использовать только идентификаторы, описанные как типы");
			break;
		case not_decl:	// test_exist
			sprintf(msg, "здесь должен быть тип (стандартный или описанный пользователем)");
			break;
		case multiple_const_in_type:
			sprintf(msg, "встречено несколько модификаторов const для одного типа");
			break;
		case empty_bound_without_init:	// test_exist
			sprintf(msg, "в описании массива границы не указаны, а инициализации нет");
			break;
		case empty_struct:
			sprintf(msg, "структура должна иметь поля");
			break;
		case empty_enum:
			sprintf(msg, "перечисление должно иметь поля");
			break;
		case amp_before_func:
			sprintf(msg, "& перед описанием функции");
			break;

		// Tree testing errors
		case node_unexpected:
			sprintf(msg, "недопустимый узел верхнего уровня %i", va_arg(args, int));
			break;

		// Codegen errors
		case tables_cannot_be_compressed:
			sprintf(msg, "невозможно сжать таблицы до заданного размера");
			break;

		case wrong_init_in_actparam:
			sprintf(msg, "в инициализаторе-фактическом параметре функции могут быть только константы");
		case array_borders_cannot_be_static_dynamic:
			sprintf(msg, "массив не может иметь статические и динамические границы");
			break;
		case such_array_is_not_supported:
			sprintf(msg, "такие массивы пока не поддерживаются в кодогенераторе");
			break;
		case too_many_arguments:
			sprintf(msg, "слишком много аргументов у функции, допустимое количество до 128");
			break;

		default:
			sprintf(msg, "неизвестный код ошибки (%i)", num);
			break;
	}
}

static void get_warning(const warning_t num, char *const msg, va_list args)
{
	(void)args;
	switch (num)
	{
		case result_of_assignment_as_condition:
			sprintf(msg, "результат присваивания используется в качестве условия");
			break;
		case too_long_int:
			sprintf(msg, "слишком большая целая константа, преобразована в ДЛИН (DOUBLE)");
			break;
		case variable_deviation:
			sprintf(msg, "cравнение вещественных без учета погрешности");
			break;
	}
}


static void output(const universal_io *const io, const char *const msg, const logger system_func
	, void (*func)(const char *const, const char *const, const char *const, const size_t))
{
	char tag[MAX_TAG_SIZE] = TAG_RUC;

	const char *code = in_get_buffer(io);
	if (code == NULL)
	{
		in_get_path(io, tag);
		system_func(tag, msg);
		return;
	}

	size_t position = in_get_position(io) - 1;
	while (position > 0
		&& (code[position] == ' ' || code[position] == '\t'
		|| code[position] == '\r' || code[position] == '\n'))
	{
		position--;
	}

	comment cmt = cmt_search(code, position);
	cmt_get_tag(&cmt, tag);

	char line[MAX_LINE_SIZE];
	cmt_get_code_line(&cmt, line);

	func(tag, msg, line, cmt_get_symbol(&cmt));
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void error(const universal_io *const io, err_t num, ...)
{
	va_list args;
	va_start(args, num);

	verror(io, num, args);

	va_end(args);
}

void warning(const universal_io *const io, warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	vwarning(io, num, args);

	va_end(args);
}


void verror(const universal_io *const io, const err_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_error(num, msg, args);
	output(io, msg, &log_system_error, &log_error);
}

void vwarning(const universal_io *const io, const warning_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_warning(num, msg, args);
	output(io, msg, &log_system_warning, &log_warning);
}


void system_error(err_t num, ...)
{
	va_list args;
	va_start(args, num);

	char msg[MAX_MSG_SIZE];
	get_error(num, msg, args);

	va_end(args);
	log_system_error(TAG_RUC, msg);
}

void system_warning(warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	char msg[MAX_MSG_SIZE];
	get_warning(num, msg, args);

	va_end(args);
	log_system_warning(TAG_RUC, msg);
}


void error_msg(const char *const msg)
{
	log_system_error(TAG_RUC, msg);
}

void warning_msg(const char *const msg)
{
	log_system_warning(TAG_RUC, msg);
}
