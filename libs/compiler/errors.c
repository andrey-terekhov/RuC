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
#include "defs.h"
#include "item.h"
#include "logger.h"
#include "uniio.h"
#include "utf8.h"


#define TAG_RUC "ruc"

#define MAX_TAG_SIZE MAXSTRINGL
#define MAX_MSG_SIZE MAXSTRINGL * 4
#define MAX_LINE_SIZE MAXSTRINGL * 4

#define MAX_INT_LENGTH 12


void get_error(const error_t num, char *const msg, va_list args)
{
	switch (num)
	{
		case bad_character:
		{
			const char32_t bad_char = va_arg(args, char32_t);
			sprintf(msg, "плохой символ = %i", bad_char);
		}
		break;

		case empty_character:
			sprintf(msg, "пустая символьная константа");
			break;

		case unknown_escape_sequence:	//test_exist
			sprintf(msg, "неизвестный служебный символ");
			break;

		case expected_apost_after_char_const: // need_test
			sprintf(msg, "символьная константа не заканчивается символом '");
			break;

		case missing_terminating_quote_char:
			sprintf(msg, "строка не заканчивается символом \"");
			break;

		case string_too_long:	// test_exist
			sprintf(msg, "слишком длинная строка (больше, чем MAXSTRINGL)");
			break;

		case unterminated_block_comment:
			sprintf(msg, "блочный комментарий не окончен");
			break;

		case no_main_in_program: // test_exist
			sprintf(msg, "в каждой программе должна быть ГЛАВНАЯ функция");
			break;

		case predef_but_notdef: // need_test
		{
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "функция %s была предопределена, но не описана", buffer);
		}
		break;

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
		case func_decl_req_params: // need_test
			sprintf(msg, "вообще-то я думал, что это предописание функции (нет "
				"идентификаторов-параметров), а тут тело функции");
			break;
		case expected_while: // test_exist
			sprintf(msg, "ждем ПОКА в операторе ЦИКЛ");
			break;
		case expected_semi_after_stmt: // test_exist
			sprintf(msg, "нет ; после оператора");
			break;
		case expected_end: // test_exist
			sprintf(msg, "нет } в конце блока");
			break;
		case cond_must_be_in_brkts: // test_exist
			sprintf(msg, "условие должно быть в ()");
			break;
		case repeated_decl:	// test_exist
		{
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "повторное описание идентификатора %s", buffer);
		}
		break;
		case arr_init_must_start_from_BEGIN: // test_exist
			sprintf(msg, "инициализация массива должна начинаться со {");
			break;
		case struct_init_must_start_from_BEGIN: // need_test
			sprintf(msg, "инициализация структуры должна начинаться со {");
			break;
		case no_comma_in_init_list: // need_test
			sprintf(msg, "между элементами инициализации массива или структуры должна быть ,");
			break;
		case ident_is_not_declared: // test_exist
		{
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "не описан идентификатор %s", buffer);
		}
		break;
		case no_rightsqbr_in_slice: // test_exist
			sprintf(msg, "не хватает ] в вырезке элемента массива");
			break;
		case index_must_be_int:	// test_exist
			sprintf(msg, "индекс элемента массива должен иметь тип ЦЕЛ");
			break;
		case slice_not_from_array:	// test_exist
			sprintf(msg, "попытка вырезки элемента не из массива");
			break;
		case call_not_from_function:	//test_exist
			sprintf(msg, "попытка вызова не функции");
			break;
		case no_comma_in_act_params: // test_exist
			sprintf(msg, "после фактического параметра должна быть ,");
			break;
		case float_instead_int:	// test_exist
			sprintf(msg, "формальный параметр имеет тип ЦЕЛ, а фактический - ВЕЩ");
			break;
		case wrong_number_of_params: // test_exist
			sprintf(msg, "неправильное количество фактических параметров");
			break;
		case wait_rightbr_in_primary: // test_exist
			sprintf(msg, "не хватает ) в первичном выражении");
			break;
		case unassignable_inc:	// test_exist
			sprintf(msg, "++ и -- применимы только к переменным и элементам массива");
			break;
		case wrong_addr:	// test_exist
			sprintf(msg, "операция получения адреса & применима только к переменным");
			break;
		case no_colon_in_cond_expr: // test_exist
			sprintf(msg, "нет : в условном выражении");
			break;
		case expected_colon_after_case: // test_exist
			sprintf(msg, "после выражения в выборе нет :");
			break;
		case no_ident_after_goto: // need_test
			sprintf(msg, "после goto должна быть метка, т.е. идентификатор");
			break;
		case no_leftbr_in_for: // test_exist
			sprintf(msg, "в операторе цикла ДЛЯ нет (");
			break;
		case no_semicolon_in_for: // test_exist
			sprintf(msg, "в операторе цикла ДЛЯ нет ;");
			break;
		case no_rightbr_in_for: // test_exist
			sprintf(msg, "в операторе цикла ДЛЯ нет )");
			break;
		case int_op_for_float:	// test_exist
			sprintf(msg, "операция, применимая только к целым, применена к вещественному аргументу");
			break;
		case assmnt_float_to_int:	// test_exist
			sprintf(msg, "нельзя присваивать целому вещественное значение");
			break;
		case redefinition_of_main:	// test_exist
			sprintf(msg, "в программе может быть только 1 идентификатор ГЛАВНАЯ");
			break;
		case no_leftbr_in_printid: // test_exist
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет (");
			break;
		case no_rightbr_in_printid: // test_exist
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )");
			break;
		case no_ident_in_printid: // need_test
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора");
			break;
		case no_leftbr_in_getid: // test_exist
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет (");
			break;
		case no_rightbr_in_getid: // test_exist
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )");
			break;
		case no_ident_in_getid: // need_test
			sprintf(msg, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора");
			break;
		case float_in_switch: // need_test
			sprintf(msg, "в условии переключателя можно использовать только типы ЛИТЕРА и ЦЕЛ");
			break;
		case init_int_by_float:	// test_exist
			sprintf(msg, "целая или литерная переменная инициализируется значением типа ВЕЩ");
			break;
		case must_be_digit_after_exp:	// test_exist
			sprintf(msg, "должна быть цифра после e");
			break;
		case no_comma_in_setmotor: // need_test
			sprintf(msg, "в команде управления роботом после первого параметра нет ,");
			break;
		case param_setmotor_not_int:	// need_test
			sprintf(msg, "в командах МОТОР, УСТНАПРЯЖЕНИЕ, ЦИФРДАТЧИК и АНАЛОГДАТЧИК параметры должны быть целыми");
			break;
		case no_leftbr_in_stand_func: // need_test
			sprintf(msg, "в вызове стандартной функции нет (");
			break;
		case no_rightbr_in_stand_func: // test_exist
			sprintf(msg, "в вызове стандартной функции нет )");
			break;
		case bad_param_in_stand_func:	// test_exist
			sprintf(msg, "параметры стандартных функций могут быть только целыми и вещественными");
			break;
		case no_ret_in_func: // test_exist
			sprintf(msg, "в функции, возвращающей непустое значение, нет оператора ВОЗВРАТ со значением");
			break;
		case bad_type_in_ret: // test_exist
			sprintf(msg, "в функции, возвращающей целое или литерное значение, оператор ВОЗВРАТ со значением ВЕЩ");
			break;
		case notvoidret_in_void_func: // test_exist
			sprintf(msg, "в функции, возвращающей пустое значение, оператор ВОЗВРАТ со значением");
			break;
		case aster_before_func:	// need_test
			sprintf(msg, "* перед описанием функции");
			break;
		case aster_not_for_pointer:	// test_exist
			sprintf(msg, "операция * применяется не к указателю");
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
		case diff_formal_param_type_and_actual:	// need_test
			sprintf(msg, "типы формального и фактического параметров различаются");
			break;
		case float_in_condition:	// need_test
			sprintf(msg, "условие должно иметь тип ЦЕЛ или ЛИТЕРА");
			break;
		case case_not_in_switch: // need_test
			sprintf(msg, "метка СЛУЧАЙ не в операторе ВЫБОР");
			break;
		case break_not_in_loop_or_switch: // need_test
			sprintf(msg, "оператор ВЫХОД не в цикле и не в операторе ВЫБОР");
			break;
		case continue_not_in_loop:	// need_test
			sprintf(msg, "оператор ПРОДОЛЖИТЬ не в цикле");
			break;
		case expected_expression:	// need_test
		{
			const int cur = va_arg(args, int);
			sprintf(msg, "первичное не может начинаться с лексемы %i", cur);
		}
		break;
		case wrong_operand:	// need_test
			sprintf(msg, "операнд операции может иметь только тип ЦЕЛ, ЛИТ или ВЕЩ");
			break;
		case label_not_declared:	// need_test
		{
			const size_t hash = va_arg(args, size_t);
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "в строке %zi переход на неописанную метку %s", hash, buffer);
		}
		break;
		case repeated_label: // test_exist
		{
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "повторное описание метки %s", buffer);
		}
		break;
		case operand_is_pointer:	// need_test
			sprintf(msg, "операнд бинарной формулы не может быть указателем");
			break;
		case pointer_in_print: // test_exist
			sprintf(msg, "указатели нельзя печатать");
			break;
		case wrong_struct:	// test_exist
			sprintf(msg, "неправильное описание структуры");
			break;
		case after_dot_must_be_ident: // test_exist
			sprintf(msg, "после . или -> должен быть идентификатор-имя поля структуры");
			break;
		case get_field_not_from_struct_pointer:	// need_test
			sprintf(msg, "применять операцию -> можно только к указателю на структуру");
			break;

		case error_in_initialization:	// test_exist
			sprintf(msg, "несоответствие типов при инициализации переменной");
			break;
		case type_missmatch:	// need_test
			sprintf(msg, "несоответствие типов");
			break;
		case array_assigment:	//test_exist
			sprintf(msg, "присваивание в массив запрещено");
			break;
		case wrong_struct_ass:	// need_test
			sprintf(msg, "для структур и указателей допустима только операция присваивания =");
			break;
		case wrong_init:	//test_exist
			sprintf(msg, "переменные такого типа нельзя инициализировать");
			break;
		case no_field:	// test_exist
		{
			const char *const buffer = va_arg(args, char *);
			sprintf(msg, "нет такого поля %s в структуре", buffer);
		}
		break;
		case slice_from_func:	// need_test
			sprintf(msg, "вырезка элемента из массива, выданного функцией, а функции не могут выдавать массивы");
			break;
		case wait_end: // need_test
			sprintf(msg, "в инициализации структуры здесь ожидалась правая фигурная скобка }");
			break;
		case act_param_not_ident:	// test_exist
			sprintf(msg, "фактическим параметром-функцией может быть только идентификатор");
			break;
		case unassignable:	// need_test
			sprintf(msg, "в левой части присваивания стоит что-то, чему нельзя присваивать");
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
		case wait_ident_after_semicolon_in_struct: // test_exist
			sprintf(msg, "в структуре после типа поля должен идти идентификатор поля");
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
		case print_without_br: // test_exist
			sprintf(msg, "операнд оператора печати должен быть в круглых скобках ()");
			break;
		case select_not_from_struct:	// test_exist
			sprintf(msg, "выборка поля . не из структуры");
			break;
		case init_not_struct:	// test_exist
			sprintf(msg, "в РуСи только структуре можно присвоить или передать параметром запись {,,,}");
			break;
		case param_threads_not_int:	// test_exist
			sprintf(msg, "процедуры, управляющие параллельными нитями, могут иметь только целые параметры");
			break;
		case wrong_arg_in_send:	// test_exist
			sprintf(msg, "неправильный тип аргумента в процедуре t_msg_send, должен иметь тип msg_info");
			break;
		case wrong_arg_in_create:	// test_exist
			sprintf(msg, "неправильный тип аргумента в процедуре t_create, должен иметь тип void*(void*)");
			break;

		case no_leftbr_in_printf: // test_exist
			sprintf(msg, "не хватает открывающей скобки в printf/печатьф");
			break;
		case no_rightbr_in_printf:	// test_exist
			sprintf(msg, "не хватает закрывающей скобки в printf/печатьф");
			break;
		case wrong_first_printf_param: // test_exist
			sprintf(msg, "первым параметром в printf/печатьф должна быть константная форматная строка");
			break;
		case wrong_printf_param_type: // test_exist
		{
			size_t index = sprintf(msg, "тип параметра printf/печатьф не соответствует спецификатору: %%");
			const char32_t bad_printf_placeholder = va_arg(args, char32_t);
			index += utf8_to_string(&msg[index], bad_printf_placeholder);
			switch (bad_printf_placeholder)
			{
				case 'i':
				case U'ц': // 1094
					index += sprintf(&msg[index], " ожидает целое число");
					break;

				case 'c':
					index += sprintf(&msg[index], " (англ.) ожидает литеру");
					break;
				case U'л': // 1083
					index += sprintf(&msg[index], " ожидает литеру");
					break;

				case 'f':
				case U'в': // 1074
					index += sprintf(&msg[index], " ожидает вещественное число");
					break;

				case U'с': // 1089
					index += sprintf(&msg[index], " (рус.) ожидает строку");
					break;
				case 's':
					index += sprintf(&msg[index], " ожидает строку");
					break;
				default:
					index += sprintf(&msg[index], " -- неизвестный спецификатор");
					break;
			}
		}
		break;
		case wrong_printf_param_number: // test_exist
			sprintf(msg, "количество параметров printf/печатьф не соответствует количеству спецификаторов");
			break;
		case printf_no_format_placeholder: // test_exist
			sprintf(msg, "в printf/печатьф нет спецификатора типа после '%%'");
			break;
		case printf_unknown_format_placeholder: // test_exist
		{
			size_t index = sprintf(msg, "в printf/печатьф неизвестный спецификатор типа %%");
			const char32_t bad_printf_placeholder = va_arg(args, char32_t);
			index += utf8_to_string(&msg[index], bad_printf_placeholder);
		}
		break;
		case too_many_printf_params: // test_exist
			sprintf(msg, "максимально в printf/печатьф можно выводить %i значений",
					MAXPRINTFPARAMS);
			break;

		case no_mult_in_cast: // need_test
			sprintf(msg, "нет * в cast (приведении)");
			break;
		case no_rightbr_in_cast: // need_test
			sprintf(msg, "нет ) в cast (приведении)");
			break;
		case not_pointer_in_cast:	// need_test
			sprintf(msg, "cast (приведение) может быть применено только к указателю");
			break;
		case empty_bound_without_init:	// test_exist
			sprintf(msg, "в описании массива границы не указаны, а инициализации нет");
			break;
		case begin_with_notarray:	// need_test
			sprintf(msg, "инициализация, начинающаяся с {, должна соответствовать массиву или структуре");
			break;
		case string_and_notstring:	// need_test
			sprintf(msg, "если в инициализаторе встретилась строка, то и дальше должны быть только строки");
			break;
		case wrong_init_in_actparam:	//test_exist
			sprintf(msg, "в инициализаторе-фактическом параметре функции могут быть только константы");
			break;
		case no_comma_or_end:	// need_test
			sprintf(msg, "в инициализаторе ожидали , или }");
			break;
		case no_comma_in_act_params_stanfunc: // need_test
			sprintf(msg, "в операции над строками после параметра нет , ");
			break;
		case not_string_in_stanfunc:	// test_exist
			sprintf(msg, "в операции над строками параметр не строка");
			break;
		case not_int_in_stanfunc:	// test_exist
			sprintf(msg, "в этой операции этот параметр должен иметь тип ЦЕЛ");
			break;
		case not_float_in_stanfunc:	// need_test
			sprintf(msg, "в этой операции этот параметр должен иметь тип ВЕЩ");
			break;
		case not_point_string_in_stanfunc:	// need_test
			sprintf(msg, "в этой операции над строками первый параметр должен быть указателем на строку");
			break;
		case not_rowofint_in_stanfunc:	// test_exist
			sprintf(msg, "в этой операции этот параметр должен иметь тип массив целых");
			break;
		case not_rowoffloat_in_stanfunc:	// need_test
			sprintf(msg, "в этой операции этот параметр должен иметь тип массив вещ");
			break;
		case not_array_in_stanfunc:	// need_test
			sprintf(msg, "в этой операции этот параметр должен иметь тип массив");
			break;
		case default_not_in_switch:
			sprintf(msg, "метка УМОЛЧАНИЕ не в операторе ВЫБОР");
			break;
		case expected_colon_after_default:
			sprintf(msg, "после метки УМОЛЧАНИЕ нет :");
			break;
		case empty_struct:
			sprintf(msg, "структура должна иметь поля");
			break;

		case tree_expression_not_block:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "в выражении встретился оператор вне блока, tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case tree_expression_unknown:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "неизвестное выражение, tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case tree_expression_operator:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "оператор в выражении, tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case tree_expression_no_texprend:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "отсутствует TExprend, tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case tree_no_tend:
			sprintf(msg, "отсутствует внешний TEnd дерева");
			break;
		case tree_unexpected:
		{
			const item_t unexp = va_arg(args, item_t);
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "получен %" PRIitem ", ожидался tree[%zi] = %" PRIitem, unexp, i, elem);
		}
		break;

		case node_cannot_add_child:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "невозможно добавить потомка к tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case node_cannot_set_type:
		{
			const item_t type = va_arg(args, item_t);
			const size_t i = va_arg(args, size_t);
			sprintf(msg, "невозможно установить тип %" PRIitem " в tree[%zi]", type, i);
		}
		break;
		case node_cannot_add_arg:
		{
			const item_t arg = va_arg(args, item_t);
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "невозможно добавить аргумент %" PRIitem " для tree[%zi] = %" PRIitem, arg, i, elem);
		}
		break;
		case node_unexpected:
		{
			const int type = va_arg(args, int);
			sprintf(msg, "недопустимый узел верхнего уровня %i", type);

		}
		break;

		case tables_cannot_be_compressed:
			sprintf(msg, "невозможно сжать таблицы до заданного размера");
			break;

		default:
			sprintf(msg, "неизвестный код ошибки (%i)", num);
			break;
	}
}

void get_warning(const warning_t num, char *const msg, va_list args)
{
	switch (num)
	{
		case too_long_int:
			sprintf(msg, "слишком большая целая константа, преобразована в ДЛИН (DOUBLE)");
			break;

		case tree_operator_unknown:
		{
			const size_t i = va_arg(args, size_t);
			const item_t elem = va_arg(args, item_t);
			sprintf(msg, "неизвестный оператор, tree[%zi] = %" PRIitem, i, elem);
		}
		break;
		case node_argc:
		{
			const size_t i = va_arg(args, size_t);
			const char *elem = va_arg(args, char *);
			sprintf(msg, "несоответствие количества аргументов, tree[%zi] = %s", i, elem);
		}
		break;
	}
}


void output(const universal_io *const io, const char *const msg, const logger system_func
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


void error(const universal_io *const io, error_t num, ...)
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


void verror(const universal_io *const io, const error_t num, va_list args)
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


void system_error(error_t num, ...)
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
