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
#include "codes.h"
#include "global.h"
#include "logger.h"
#include "scanner.h"
#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>


size_t printident(compiler_context *context, int r, char *const msg, size_t index)
{
	r += 2; // ссылка на context->reprtab
	do
	{
		int wchar = REPRTAB[r++];

		if (wchar < 128)
		{
			index += sprintf(&msg[index], "%c", wchar);
		}
		else
		{
			unsigned char first = (unsigned char)((wchar >> 6) | /*0b11000000*/ 0xC0);
			unsigned char second = (unsigned char)((wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80);

			index += sprintf(&msg[index], "%c%c", first, second);
		}
	} while (REPRTAB[r] != 0);
	
	return index;
}

/*void get_tag(compiler_context *context, char *const tag)
{
	data_file *file;

	if (context->c_flag)
	{
		file = &((context->cfs).files[(context->cfs).cur]);
	}
	else
	{
		file = &((context->hfs).files[(context->hfs).cur]);
	}

	sprintf(tag, "%s", file->name);
}*/

void warning(compiler_context *context, int ernum)
{
	char tag[MAXSTRINGL] = "ruc";
	char msg[4 * MAXSTRINGL];

	//get_tag(context, tag);

	switch (ernum)
	{
		case too_long_int:
			sprintf(msg, "слишком большая целая константа, преобразована в ДЛИН "
												  "(DOUBLE)");
			break;

		default:
			break;
	}

	log_system_warning(tag, msg);
}

/*void show_macro(compiler_context *context, int k, int nwe_line, int *s, int num)
{
	int flag = 1;
	int i = k;
	int j = 0;

	printer_printf(&context->err_options, "line %i) ", nwe_line + num);
	while (s[i] != '\n' && s[i] != EOF)
	{
		printer_printchar(&context->err_options, s[i]);
		if (flag && context->last_line[j] == s[i])
		{
			if (j == context->charnum)
			{
				break;
			}
			j++;
			i++;
		}
		else
		{
			flag = 0;
			i++;
		}
	}

	if (flag == 0)
	{
		printer_printf(&context->err_options, "\n\n В строке есть макрозамена, строка после макрогенерации:\nline %i)",
					   context->line);

		for (j = 0; j < context->charnum; j++)
		{
			printer_printchar(&context->err_options, context->last_line[j]);
		}
		printer_printf(&context->err_options, "\n");
	}
}*/

void error(compiler_context *context, int ernum)
{
	/*int i = 0;
	int k = 0;

	data_file *f;
	if (context->c_flag)
	{
		f = &((context->cfs).files[(context->cfs).cur]);
	}
	else
	{
		f = &((context->hfs).files[(context->hfs).cur]);
	}

	const char *name = f->name;
	//printer_printf(&context->err_options, "\n Oшибка в файле: \"%s\" № %i\n \n", name, ernum);
	context->line--;
	if (context->charnum == 0)
	{
		context->charnum = context->charnum_before;
	}
	else
	{
		context->charnum--;
	}

	int new_line = context->line;

	while (control_before[i] < context->line + 1)
	{
		new_line += control_after[i] - 1;
		i++;
	}
	nwe_line += 1;
	//!!! const char *name - имя файла
	//!!! int *s - большой текст
	//!!! int nwe_line - номер реальной строки в большом тексте (s)
	//int num_line = nwe_line + f->include_line;//!!! - номер строки (цифра которую вывести в сообщении)


	/*i = 0;
	k = 0;

	i = 0;

	for (int j = 1; j < new_line; j++)
	{
		printer_printf(&context->err_options, "line %i) ", j + f->include_line);

		while (s[i] != '\n' && s[i] != EOF)
		{
			printer_printchar(&context->err_options, s[i]);
			i++;
		}
		printer_printf(&context->err_options, "\n");
		i++;
	}

	show_macro(context, i, new_line, s);
	*/
	char tag[MAXSTRINGL] = "ruc";
	char msg[4 * MAXSTRINGL];
	size_t index = 0;

	//get_tag(context, tag);
	
	context->error_flag = 1;
	context->tc = context->temp_tc;
	if (!context->new_line_flag && context->curchar != EOF)
	{
		while (context->curchar != '\n' && context->curchar != EOF)
		{
			nextch(context);
		}

		if (context->curchar != EOF)
		{
			scaner(context);
		}
	}

	if (context->curchar != EOF)
	{
		scaner(context);
	}

	switch (ernum)
	{
		case after_type_must_be_ident: // test_exist
			index += sprintf(&msg[index], "после символа типа должен быть идентификатор или * "
												  "идентификатор");
			break;
		case wait_right_sq_br: // test_exist
			index += sprintf(&msg[index], "ожидалась ]");
			break;
		case only_functions_may_have_type_VOID: // need_test
			index += sprintf(&msg[index], "только функции могут иметь тип ПУСТО");
			break;
		case decl_and_def_have_diff_type:	// test_exist
			index += sprintf(&msg[index], "прототип функции и ее описание имеют разные типы");
			break;
		case wrong_param_list: // need_test
			index += sprintf(&msg[index], "неправильный список параметров");
			break;
		case def_must_end_with_semicomma: // test_exist
			index += sprintf(&msg[index], "список описаний должен заканчиваться ;");
			break;
		case func_decl_req_params: // need_test
			index += sprintf(&msg[index], "вообще-то я думал, что это предописание функции (нет "
												  "идентификаторов-параметров), а тут тело функции");
			break;
		case wait_while_in_do_stmt: // test_exist
			index += sprintf(&msg[index], "ждем ПОКА в операторе ЦИКЛ");
			break;
		case no_semicolon_after_stmt: // test_exist
			index += sprintf(&msg[index], "нет ; после оператора");
			break;
		case cond_must_be_in_brkts: // test_exist
			index += sprintf(&msg[index], "условие должно быть в ()");
			break;
		case repeated_decl:	// test_exist
			index += sprintf(&msg[index], "повторное описание идентификатора ");
			index = printident(context, REPRTAB_POS, msg, index);
			break;
		case arr_init_must_start_from_BEGIN: // test_exist
			index += sprintf(&msg[index], "инициализация массива должна начинаться со {");
			break;
		case struct_init_must_start_from_BEGIN: // need_test
			index += sprintf(&msg[index], "инициализация структуры должна начинаться со {");
			break;
		case no_comma_in_init_list: // need_test
			index += sprintf(&msg[index], "между элементами инициализации массива или структуры "
												  "должна быть ,");
			break;
		case ident_is_not_declared: // test_exist
			index += sprintf(&msg[index], "не описан идентификатор ");
			index = printident(context, REPRTAB_POS, msg, index);
			break;
		case no_rightsqbr_in_slice: // test_exist
			index += sprintf(&msg[index], "не хватает ] в вырезке элемента массива");
			break;
		case index_must_be_int:	// test_exist
			index += sprintf(&msg[index], "индекс элемента массива должен иметь тип ЦЕЛ");
			break;
		case slice_not_from_array:	// test_exist
			index += sprintf(&msg[index], "попытка вырезки элемента не из массива");
			break;
		case call_not_from_function:	//test_exist
			index += sprintf(&msg[index], "попытка вызова не функции");
			break;
		case no_comma_in_act_params: // test_exist
			index += sprintf(&msg[index], "после фактического параметра должна быть ,");
			break;
		case float_instead_int:	// test_exist
			index += sprintf(&msg[index], "формальный параметр имеет тип ЦЕЛ, а фактический - ВЕЩ");
			break;
		case wrong_number_of_params: // test_exist
			index += sprintf(&msg[index], "неправильное количество фактических параметров");
			break;
		case wait_rightbr_in_primary: // test_exist
			index += sprintf(&msg[index], "не хватает ) в первичном выражении");
			break;
		case unassignable_inc:	// test_exist
			index += sprintf(&msg[index], "++ и -- применимы только к переменным и элементам массива");
			break;
		case wrong_addr:	// test_exist
			index += sprintf(&msg[index], "операция получения адреса & применима только к переменным");
			break;
		case no_colon_in_cond_expr: // test_exist
			index += sprintf(&msg[index], "нет : в условном выражении");
			break;
		case no_colon_in_case: // test_exist
			index += sprintf(&msg[index], "после выражения в выборе нет :");
			break;
		case case_after_default: // need_test
			index += sprintf(&msg[index], "встретился выбор после умолчания");
			break;
		case no_ident_after_goto: // need_test
			index += sprintf(&msg[index], "после goto должна быть метка, т.е. идентификатор");
			break;
		case no_leftbr_in_for: // test_exist
			index += sprintf(&msg[index], "в операторе цикла ДЛЯ нет (");
			break;
		case no_semicolon_in_for: // test_exist
			index += sprintf(&msg[index], "в операторе цикла ДЛЯ нет ;");
			break;
		case no_rightbr_in_for: // test_exist
			index += sprintf(&msg[index], "в операторе цикла ДЛЯ нет )");
			break;
		case int_op_for_float:	// test_exist
			index += sprintf(&msg[index], "операция, применимая только к целым, применена к "
												  "вещественному аргументу");
			break;
		case assmnt_float_to_int:	// test_exist
			index += sprintf(&msg[index], "нельзя присваивать целому вещественное значение");
			break;
		case more_than_1_main:	// test_exist
			index += sprintf(&msg[index], "в программе может быть только 1 идентификатор ГЛАВНАЯ");
			break;
		case no_main_in_program: // test_exist
			index += sprintf(&msg[index], "в каждой программе должна быть ГЛАВНАЯ функция");
			break;
		case no_leftbr_in_printid: // test_exist
			index += sprintf(&msg[index], "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет (");
			break;
		case no_rightbr_in_printid: // test_exist
			index += sprintf(&msg[index], "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )");
			break;
		case no_ident_in_printid: // need_test
			index += sprintf(&msg[index], "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора");
			break;
		case float_in_switch: // need_test
			index += sprintf(&msg[index], "в условии переключателя можно использовать только типы "
												  "ЛИТЕРА и ЦЕЛ");
			break;
		case init_int_by_float:	// test_exist
			index += sprintf(&msg[index], "целая или литерная переменная инициализируется значением "
												  "типа ВЕЩ");
			break;
		case must_be_digit_after_exp:	// test_exist
			index += sprintf(&msg[index], "должна быть цифра после e");
			break;
		case no_comma_in_setmotor: // need_test
			index += sprintf(&msg[index], "в команде управления роботом после первого параметра нет ,");
			break;
		case param_setmotor_not_int:	// need_test
			index += sprintf(&msg[index], "в командах МОТОР, УСТНАПРЯЖЕНИЕ, ЦИФРДАТЧИК и АНАЛОГДАТЧИК "
												  "параметры должны быть целыми");
			break;
		case no_leftbr_in_stand_func: // need_test
			index += sprintf(&msg[index], "в вызове стандартной функции нет (");
			break;
		case no_rightbr_in_stand_func: // test_exist
			index += sprintf(&msg[index], "в вызове стандартной функции нет )");
			break;
		case bad_param_in_stand_func:	// test_exist
			index += sprintf(&msg[index], "параметры стандартных функций могут быть только целыми и "
												  "вещественными");
			break;
		case no_ret_in_func: // test_exist
			index += sprintf(&msg[index], "в функции, возвращающей непустое значение, нет оператора "
												  "ВОЗВРАТ со значением");
			break;
		case bad_type_in_ret: // test_exist
			index += sprintf(&msg[index], "в функции, возвращающей целое или литерное значение, "
												  "оператор ВОЗВРАТ со значением ВЕЩ");
			break;
		case notvoidret_in_void_func: // test_exist
			index += sprintf(&msg[index], "в функции, возвращающей пустое значение, оператор ВОЗВРАТ "
												  "со значением");
			break;
		case bad_escape_sym:	//test_exist
			index += sprintf(&msg[index], "неизвестный служебный символ");
			break;
		case no_right_apost: // need_test
			index += sprintf(&msg[index], "символьная константа не заканчивается символом '");
			break;
		case decl_after_strmt: // test_exist
			index += sprintf(&msg[index], "встретилось описание после оператора");
			break;
		case too_long_string:	// test_exist
			index += sprintf(&msg[index], "слишком длинная строка ( больше, чем MAXSTRINGL)");
			break;
		case aster_before_func:	// need_test
			index += sprintf(&msg[index], "* перед описанием функции");
			break;
		case aster_not_for_pointer:	// test_exist
			index += sprintf(&msg[index], "операция * применяется не к указателю");
			break;
		case aster_with_row:	// need_test
			index += sprintf(&msg[index], "операцию * нельзя применять к массивам");
			break;
		case wrong_fun_as_param: // need_test
			index += sprintf(&msg[index], "неправильная запись функции, передаваемой параметром в "
												  "другую функцию");
			break;
		case no_right_br_in_paramfun: // need_test
			index += sprintf(&msg[index], "нет ) в функции, передаваемой параметром в другую функцию");
			break;
		case par_type_void_with_nofun:	// need_test
			index += sprintf(&msg[index], "в параметре функции тип пусто может быть только у "
												  "параметра-функции");
			break;
		case ident_in_declarator:	// need_test
			index += sprintf(&msg[index], "в деклараторах (предописаниях) могут быть только типы, но "
												  "без идентификаторов-параметров");
			break;
		case array_before_func: // need_test
			index += sprintf(&msg[index], "функция не может выдавать значение типа массив");
			break;
		case wait_definition: // need_test
			index += sprintf(&msg[index], "вообще-то, я думал, что это определение функции, а тут нет "
												  "идентификатора-параметра");
			break;
		case wait_declarator: // need_test
			index += sprintf(&msg[index], "вообще-то, я думал, что это предописание функции, а тут "
												  "идентификатор-параметр");
			break;
		case two_idents_for_1_declarer:	// need_test
			index += sprintf(&msg[index], "в описании функции на каждый описатель должен быть один "
												  "параметр");
			break;
		case function_has_no_body: // need_test
			index += sprintf(&msg[index], "есть параметры определения функции, но нет блока, "
												  "являющегося ее телом");
			break;
		case diff_formal_param_type_and_actual:	// need_test
			index += sprintf(&msg[index], "типы формального и фактического параметров различаются");
			break;
		case float_in_condition:	// need_test
			index += sprintf(&msg[index], "условие должно иметь тип ЦЕЛ или ЛИТЕРА");
			break;
		case case_or_default_not_in_switch: // need_test
			index += sprintf(&msg[index], "метка СЛУЧАЙ или УМОЛЧАНИЕ не в операторе ВЫБОР");
			break;
		case break_not_in_loop_or_switch: // need_test
			index += sprintf(&msg[index], "оператор ВЫХОД не в цикле и не в операторе ВЫБОР");
			break;
		case continue_not_in_loop:	// need_test
			index += sprintf(&msg[index], "оператор ПРОДОЛЖИТЬ не в цикле");
			break;
		case not_primary:	// need_test
			index += sprintf(&msg[index], "первичное не может начинаться с лексемы %i", context->cur);
			break;
		case wrong_operand:	// need_test
			index += sprintf(&msg[index], "операнд операции может иметь только тип ЦЕЛ, ЛИТ или ВЕЩ");
			break;
		case label_not_declared:	// need_test
			index += sprintf(&msg[index], "в строке %i переход на неописанную метку ", context->hash);
			index = printident(context, REPRTAB_POS, msg, index);
			break;
		case repeated_label: // test_exist
			index += sprintf(&msg[index], "повторное описание метки ");
			index = printident(context, REPRTAB_POS, msg, index);
			break;
		case operand_is_pointer:	// need_test
			index += sprintf(&msg[index], "операнд бинарной формулы не может быть указателем");
			break;
		case pointer_in_print: // test_exist
			index += sprintf(&msg[index], "указатели нельзя печатать");
			break;
		case wrong_struct:	// test_exist
			index += sprintf(&msg[index], "неправильное описание структуры");
			break;
		case after_dot_must_be_ident: // test_exist
			index += sprintf(&msg[index], "после . или -> должен быть идентификатор-имя поля "
												  "структуры");
			break;
		case get_field_not_from_struct_pointer:	// need_test
			index += sprintf(&msg[index], "применять операцию -> можно только к указателю на "
												  "структуру");
			break;

		case error_in_initialization:	// test_exist
			index += sprintf(&msg[index], "несоответствие типов при инициализации переменной");
			break;
		case type_missmatch:	// need_test
			index += sprintf(&msg[index], "несоответствие типов");
			break;
		case array_assigment:	//test_exist
			index += sprintf(&msg[index], "присваивание в массив запрещено");
			break;
		case wrong_struct_ass:	// need_test
			index += sprintf(&msg[index], "для структур и указателей допустима только операция "
												  "присваивания =");
			break;
		case wrong_init:	//test_exist
			index += sprintf(&msg[index], "переменные такого типа нельзя инициализировать");
			break;
		case no_field:	// test_exist
			index += sprintf(&msg[index], "нет такого поля ");
			index = printident(context, REPRTAB_POS, msg, index);
			index += sprintf(&msg[index], " в структуре");
			break;
		case slice_from_func:	// need_test
			index += sprintf(&msg[index], "вырезка элемента из массива, выданного функцией, а функции "
												  "не могут выдавать массивы");
			break;
		case bad_toval:	// need_test
			index += sprintf(&msg[index], "странный toval ansttype=%i", context->ansttype);
			break;
		case wait_end: // need_test
			index += sprintf(&msg[index], "в инициализации структуры здесь ожидалась правая фигурная "
												  "скобка }");
			break;
		case act_param_not_ident:	// test_exist
			index += sprintf(&msg[index], "фактическим параметром-функцией может быть только "
												  "идентификатор");
			break;
		case unassignable:	// need_test
			index += sprintf(&msg[index], "в левой части присваивания стоит что-то, чему нельзя "
												  "присваивать");
			break;
		case pnt_before_array:	// test_exist
			index += sprintf(&msg[index], "в РуСи не бывает указателей на массивы");
			break;
		case array_size_must_be_int:	// test_exist
			index += sprintf(&msg[index], "размер массива может иметь тип только ЦЕЛ или ЛИТЕРА");
			break;
		case no_semicomma_in_struct:	// need_test
			index += sprintf(&msg[index], "описание поля структуры должно заканчиваться ;");
			break;
		case wait_ident_after_semicomma_in_struct: // test_exist
			index += sprintf(&msg[index], "в структуре после типа поля должен идти идентификатор поля");
			break;
		case empty_init:	// test_exist
			index += sprintf(&msg[index], "в РуСи можно определять границы массива по инициализации "
												  "только по младшему измерению");
			break;
		case ident_not_type:	// test_exist
			index += sprintf(&msg[index], "в качестве описателя можно использовать только "
												  "идентификаторы, описанные как типы");
			break;
		case not_decl:	// test_exist
			index += sprintf(&msg[index], "здесь должен быть тип (стандартный или описанный "
												  "пользователем)");
			break;
		case predef_but_notdef: // need_test
			index += sprintf(&msg[index], "функция ");
			index = printident(context, REPRTAB_POS, msg, index);
			index += sprintf(&msg[index], " была предопределена, но не описана");
			break;
		case print_without_br: // test_exist
			index += sprintf(&msg[index], "операнд оператора печати должен быть в круглых скобках ()");
			break;
		case select_not_from_struct:	// test_exist
			index += sprintf(&msg[index], "выборка поля . не из структуры");
			break;
		case init_not_struct:	// test_exist
			index += sprintf(&msg[index], "в РуСи только структуре можно присвоить или передать "
												  "параметром запись {,,,}");
			break;
		case param_threads_not_int:	// test_exist
			index += sprintf(&msg[index], "процедуры, управляющие параллельными нитями, могут иметь "
												  "только целые параметры");
			break;
		case wrong_arg_in_send:	// test_exist
			index += sprintf(&msg[index], "неправильный тип аргумента в процедуре t_msg_send, должен "
												  "иметь тип msg_info");
			break;
		case wrong_arg_in_create:	// test_exist
			index += sprintf(&msg[index], "неправильный тип аргумента в процедуре t_create, должен "
												  "иметь тип void*(void*)");
			break;

		case no_leftbr_in_printf: // test_exist
			index += sprintf(&msg[index], "не хватает открывающей скобки в printf/печатьф");
			break;
		case no_rightbr_in_printf:	// test_exist
			index += sprintf(&msg[index], "не хватает закрывающей скобки в printf/печатьф");
			break;
		case wrong_first_printf_param: // test_exist
			index += sprintf(&msg[index], "первым параметром в printf/печатьф должна быть константная "
												  "форматная строка");
			break;
		case wrong_printf_param_type: // test_exist
			index += sprintf(&msg[index], "тип параметра printf/печатьф не соответствует "
												  "спецификатору: %%");
			index += utf8_to_string(&msg[index], context->bad_printf_placeholder);
			switch (context->bad_printf_placeholder)
			{
				case 'i':
				case 1094: // 'ц'
					index += sprintf(&msg[index], " ожидает целое число");
					break;

				case 'c':
					index += sprintf(&msg[index], " (англ.)");
				case 1083: // л
					index += sprintf(&msg[index], " ожидает литеру");
					break;

				case 'f':
				case 1074: // в
					index += sprintf(&msg[index], " ожидает вещественное число");
					break;

				case 1089: // с
					index += sprintf(&msg[index], " (рус.)");
				case 's':
					index += sprintf(&msg[index], " ожидает строку");
					break;
				default:
					index += sprintf(&msg[index], " -- неизвестный спецификатор");
			}
			break;
		case wrong_printf_param_number: // test_exist
			index += sprintf(&msg[index], "количество параметров printf/печатьф не соответствует "
												  "количеству спецификаторов");
			break;
		case printf_no_format_placeholder: // test_exist
			index += sprintf(&msg[index], "в printf/печатьф нет спецификатора типа после '%%'");
			break;
		case printf_unknown_format_placeholder: // test_exist
			index += sprintf(&msg[index], "в printf/печатьф неизвестный спецификатор типа %%");
			index += utf8_to_string(&msg[index], context->bad_printf_placeholder);
			break;
		case too_many_printf_params: // test_exist
			index += sprintf(&msg[index], "максимально в printf/печатьф можно выводить %i значений",
						   MAXPRINTFPARAMS);
			break;

		case no_mult_in_cast: // need_test
			index += sprintf(&msg[index], "нет * в cast (приведении)");
			break;
		case no_rightbr_in_cast: // need_test
			index += sprintf(&msg[index], "нет ) в cast (приведении)");
			break;
		case not_pointer_in_cast:	// need_test
			index += sprintf(&msg[index], "cast (приведение) может быть применено только к указателю");
			break;
		case empty_bound_without_init:	// test_exist
			index += sprintf(&msg[index], "в описании массива границы не указаны, а инициализации нет");
			break;
		case begin_with_notarray:	// need_test
			index += sprintf(&msg[index], "инициализация, начинающаяся с {, должна соответствовать "
												  "массиву или структуре");
			break;
		case string_and_notstring:	// need_test
			index += sprintf(&msg[index], "если в инициализаторе встретилась строка, то и дальше "
												  "должны быть только строки");
			break;
		case wrong_init_in_actparam:	//test_exist
			index += sprintf(&msg[index], "в инициализаторе-фактическом параметре функции могут быть "
												  "только константы");
			break;
		case no_comma_or_end:	// need_test
			index += sprintf(&msg[index], "в инициализаторе ожидали , или }");
			break;
		case no_comma_in_act_params_stanfunc: // need_test
			index += sprintf(&msg[index], "в операции над строками после параметра нет , ");
			break;
		case not_string_in_stanfunc:	// test_exist
			index += sprintf(&msg[index], "в операции над строками параметр не строка");
			break;
		case not_int_in_stanfunc:	// test_exist
			index += sprintf(&msg[index], "в этой операции этот параметр должен иметь тип ЦЕЛ");
			break;
		case not_float_in_stanfunc:	// need_test
			index += sprintf(&msg[index], "в этой операции этот параметр должен иметь тип ВЕЩ");
			break;
		case not_point_string_in_stanfunc:	// need_test
			index += sprintf(&msg[index], "в этой операции над строками первый параметр должен быть "
												  "указателем на строку");
			break;
		case not_rowofint_in_stanfunc:	// test_exist
			index += sprintf(&msg[index], "в этой операции этот параметр должен иметь тип массив "
												  "целых");
			break;
		case not_rowoffloat_in_stanfunc:	// need_test
			index += sprintf(&msg[index], "в этой операции этот параметр должен иметь тип массив вещ");
			break;
		case not_array_in_stanfunc:	// need_test
			index += sprintf(&msg[index], "в этой операции этот параметр должен иметь тип массив");
			break;
		default:
			index += sprintf(&msg[index], "этот код ошибки я прозевал");
	}

	log_system_error(tag, msg);
	// exit(2);
}

/*void set_errors_output(compiler_context *context, char *path)
{
	compiler_context_detach_io(context, IO_TYPE_ERROR);
	compiler_context_attach_io(context, path, IO_TYPE_ERROR, IO_SOURCE_FILE);
}*/

int get_exit_code(compiler_context *context)
{
	return context->error_flag != 0;
}
