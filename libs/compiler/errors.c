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
#include "macro_global_struct.h"
#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>


void printident(compiler_context *context, int r)
{
	r += 2; // ссылка на context->reprtab
	do
	{
		printer_printchar(&context->err_options, REPRTAB[r++]);
	} while (REPRTAB[r] != 0);
}

void warning(compiler_context *context, int ernum)
{
	switch (ernum)
	{
		case too_long_int:
			printer_printf(&context->err_options, "слишком большая целая константа, преобразована в ДЛИН "
												  "(DOUBLE)\n");
			break;

		default:
			break;
	}
}

void show_macro(compiler_context *context, int k, int nwe_line, int *s)
{
	int flag = 1;
	int i = k;
	int j = 0;

	printer_printf(&context->err_options, "line %i) ", nwe_line+1);
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
}

void error(compiler_context *context, int ernum)
{
	context->error_flag = 1;
	context->tc = context->temp_tc;
	if(!context->new_line_flag && context->curchar != EOF)
	{
		while(context->curchar != '\n' && context->curchar != EOF)
		{
			nextch(context); 
		}
		
		if(context->curchar != EOF)
		{
			scaner(context);
		}
	}
	
	if(context->curchar != EOF)
	{
		scaner(context);
	}
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
	int *s = (f->before_source).str;

	printer_printf(&context->err_options, "\n Oшибка в файле: \"%s\" № %i\n \n", name, ernum);
	context->line--;
	if (context->charnum == 0)
	{
		context->charnum = context->charnum_before;
	}
	else
	{
		context->charnum--;
	}

	int nwe_line = context->line;
	int *control_before = (f->cs).str_before;
	int *control_after = (f->cs).str_after;

	while (control_before[i] < context->line + 1)
	{
		nwe_line += control_after[i] - 1;
		i++;
	}
	nwe_line += 1;
	i = 0;
	k = 0;
	if (f->include_source.str[0] != 0)
	{
		k++;

		printer_printf(&context->err_options, "line %i) ", k);

		int *s2 = f->include_source.str;
		while (s2[i] != '\0')
		{
			printer_printchar(&context->err_options, s2[i]);
			if (s2[i] == '\n' && s2[i + 1] == '\0')
			{
				break;
			}
			else if (s2[i] == '\n')
			{
				k++;
				printer_printf(&context->err_options, "line %i) ", k);
			}
			i++;
		}
	}

	i = 0;

	for (int j = 1; j < nwe_line; j++)
	{
		printer_printf(&context->err_options, "line %i) ", j + k);

		while (s[i] != '\n' && s[i] != EOF)
		{
			printer_printchar(&context->err_options, s[i]);
			i++;
		}
		printer_printf(&context->err_options, "\n");
		i++;
	}

	show_macro(context, i, nwe_line, s);
	*/
	printer_printf(&context->err_options, "\x1B[1;31m ошибка:\x1B[0m ");

	switch (ernum)
	{
		case after_type_must_be_ident://OK--
			printer_printf(&context->err_options, "после символа типа должен быть идентификатор или * "
												  "идентификатор\n");
			break;
		case wait_right_sq_br: //OK/2
			printer_printf(&context->err_options, "ожидалась ]\n");
			break;
		case only_functions_may_have_type_VOID:
			printer_printf(&context->err_options, "только функции могут иметь тип ПУСТО\n");
			break;
		case decl_and_def_have_diff_type:
			printer_printf(&context->err_options, "прототип функции и ее описание имеют разные типы\n");
			break;
		case wrong_param_list://ОК??
			printer_printf(&context->err_options, "неправильный список параметров\n");
			break;
		case def_must_end_with_semicomma://OK/1
			printer_printf(&context->err_options, "список описаний должен заканчиваться ;\n");
			break;
		case func_decl_req_params:
			printer_printf(&context->err_options, "вообще-то я думал, что это предописание функции (нет "
												  "идентификаторов-параметров), а тут тело функции\n");
			break;
		case wait_while_in_do_stmt://ОК/14
			printer_printf(&context->err_options, "ждем ПОКА в операторе ЦИКЛ\n");
			break;
		case no_semicolon_after_stmt://OK/15
			printer_printf(&context->err_options, "нет ; после оператора\n");
			break;
		case cond_must_be_in_brkts://OK/3
			printer_printf(&context->err_options, "условие должно быть в ()\n");
			break;
		case repeated_decl:
			printer_printf(&context->err_options, "повторное описание идентификатора ");
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, "\n");
			break;
		case arr_init_must_start_from_BEGIN://OK/16
			printer_printf(&context->err_options, "инициализация массива должна начинаться со {\n");
			break;
		case struct_init_must_start_from_BEGIN://OK?+
			printer_printf(&context->err_options, "инициализация структуры должна начинаться со {\n");
			break;
		case no_comma_in_init_list://OK??
			printer_printf(&context->err_options, "между элементами инициализации массива или структуры "
												  "должна быть ,\n");
			break;
		case ident_is_not_declared:
			printer_printf(&context->err_options, "не описан идентификатор ");
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, "\n");
			break;
		case no_rightsqbr_in_slice://OK/4
			printer_printf(&context->err_options, "не хватает ] в вырезке элемента массива\n");
			break;
		case index_must_be_int:
			printer_printf(&context->err_options, "индекс элемента массива должен иметь тип ЦЕЛ\n");
			break;
		case slice_not_from_array:
			printer_printf(&context->err_options, "попытка вырезки элемента не из массива\n");
			break;
		case call_not_from_function:
			printer_printf(&context->err_options, "попытка вызова не функции\n");
			break;
		case no_comma_in_act_params://OO/5
			printer_printf(&context->err_options, "после фактического параметра должна быть ,\n");
			break;
		case float_instead_int:
			printer_printf(&context->err_options, "формальный параметр имеет тип ЦЕЛ, а фактический - ВЕЩ\n");
			break;
		case wrong_number_of_params://OK--
			printer_printf(&context->err_options, "неправильное количество фактических параметров\n");
			break;
		case wait_rightbr_in_primary: //OK/6
			printer_printf(&context->err_options, "не хватает ) в первичном выражении\n");
			break;
		case unassignable_inc:
			printer_printf(&context->err_options, "++ и -- применимы только к переменным и элементам массива\n");
			break;
		case wrong_addr:
			printer_printf(&context->err_options, "операция получения адреса & применима только к переменным\n");
			break;
		case no_colon_in_cond_expr://OK/7
			printer_printf(&context->err_options, "нет : в условном выражении\n");
			break;
		case no_colon_in_case://OK/8
			printer_printf(&context->err_options, "после выражения в выборе нет :\n");
			break;
		case case_after_default:
			printer_printf(&context->err_options, "встретился выбор после умолчания\n");
			break;
		case no_ident_after_goto:
			printer_printf(&context->err_options, "после goto должна быть метка, т.е. идентификатор\n");
			break;
		case no_leftbr_in_for://OK/9
			printer_printf(&context->err_options, "в операторе цикла ДЛЯ нет (\n");
			break;
		case no_semicolon_in_for://OK/10
			printer_printf(&context->err_options, "в операторе цикла ДЛЯ нет ;\n");
			break;
		case no_rightbr_in_for://ОК/11
			printer_printf(&context->err_options, "в операторе цикла ДЛЯ нет )\n");
			break;
		case int_op_for_float:
			printer_printf(&context->err_options, "операция, применимая только к целым, применена к "
												  "вещественному аргументу\n");
			break;
		case assmnt_float_to_int:
			printer_printf(&context->err_options, "нельзя присваивать целому вещественное значение\n");
			break;
		case more_than_1_main:
			printer_printf(&context->err_options, "в программе может быть только 1 идентификатор ГЛАВНАЯ\n");
			break;
		case no_main_in_program:
			printer_printf(&context->err_options, "в каждой программе должна быть ГЛАВНАЯ функция\n");
			break;
		case no_leftbr_in_printid://OK/12
			printer_printf(&context->err_options, "в колманде ПЕЧАТЬИД или ЧИТАТЬИД нет (\n");
			break;
		case no_rightbr_in_printid://OK/13
			printer_printf(&context->err_options, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )\n");
			break;
		case no_ident_in_printid://OK--
			printer_printf(&context->err_options, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора\n");
			break;
		case float_in_switch:
			printer_printf(&context->err_options, "в условии переключателя можно использовать только типы "
												  "ЛИТЕРА и ЦЕЛ\n");
			break;
		case init_int_by_float:
			printer_printf(&context->err_options, "целая или литерная переменная инициализируется значением "
												  "типа ВЕЩ\n");
			break;
		case must_be_digit_after_exp:
			printer_printf(&context->err_options, "должна быть цифра после e\n");
			break;
		case no_comma_in_setmotor://OK
			printer_printf(&context->err_options, "в команде управления роботом после первого параметра нет ,\n");
			break;
		case param_setmotor_not_int:
			printer_printf(&context->err_options, "в командах МОТОР, УСТНАПРЯЖЕНИЕ, ЦИФРДАТЧИК и АНАЛОГДАТЧИК "
												  "параметры должны быть целыми\n");
			break;
		case no_leftbr_in_stand_func://OK??
			printer_printf(&context->err_options, "в вызове стандартной функции нет (\n");
			break;
		case no_rightbr_in_stand_func://OK
			printer_printf(&context->err_options, "в вызове стандартной функции нет )\n");
			break;
		case bad_param_in_stand_func:
			printer_printf(&context->err_options, "параметры стандартных функций могут быть только целыми и "
												  "вещественными\n");
			break;
		case no_ret_in_func:
			printer_printf(&context->err_options, "в функции, возвращающей непустое значение, нет оператора "
												  "ВОЗВРАТ со значением\n");
			break;
		case bad_type_in_ret:
			printer_printf(&context->err_options, "в функции, возвращающей целое или литерное значение, "
												  "оператор ВОЗВРАТ со значением ВЕЩ\n");
			break;
		case notvoidret_in_void_func:
			printer_printf(&context->err_options, "в функции, возвращающей пустое значение, оператор ВОЗВРАТ "
												  "со значением\n");
			break;
		case bad_escape_sym:
			printer_printf(&context->err_options, "неизвестный служебный символ\n");
			break;
		case no_right_apost://OK?+
			printer_printf(&context->err_options, "символьная константа не заканчивается символом '\n");
			break;
		case decl_after_strmt:
			printer_printf(&context->err_options, "встретилось описание после оператора\n");
			break;
		case too_long_string:
			printer_printf(&context->err_options, "слишком длинная строка ( больше, чем MAXSTRINGL)\n");
			break;
		case aster_before_func:
			printer_printf(&context->err_options, "* перед описанием функции\n");
			break;
		case aster_not_for_pointer:
			printer_printf(&context->err_options, "операция * применяется не к указателю\n");
			break;
		case aster_with_row:
			printer_printf(&context->err_options, "операцию * нельзя применять к массивам\n");
			break;
		case wrong_fun_as_param://OK--
			printer_printf(&context->err_options, "неправильная запись функции, передаваемой параметром в "
												  "другую функцию\n");
			break;
		case no_right_br_in_paramfun://OK
			printer_printf(&context->err_options, "нет ) в функции, передаваемой параметром в другую функцию\n");
			break;
		case par_type_void_with_nofun:
			printer_printf(&context->err_options, "в параметре функции тип пусто может быть только у "
												  "параметра-функции\n");
			break;
		case ident_in_declarator:
			printer_printf(&context->err_options, "в деклараторах (предописаниях) могут быть только типы, но "
												  "без идентификаторов-параметров\n");
			break;
		case array_before_func:
			printer_printf(&context->err_options, "функция не может выдавать значение типа массив\n");
			break;
		case wait_definition:
			printer_printf(&context->err_options, "вообще-то, я думал, что это определение функции, а тут нет "
												  "идентификатора-параметра\n");
			break;
		case wait_declarator:
			printer_printf(&context->err_options, "вообще-то, я думал, что это предописание функции, а тут "
												  "идентификатор-параметр\n");
			break;
		case two_idents_for_1_declarer:
			printer_printf(&context->err_options, "в описании функции на каждый описатель должен быть один "
												  "параметр\n");
			break;
		case function_has_no_body:
			printer_printf(&context->err_options, "есть параметры определения функции, но нет блока, "
												  "являющегося ее телом\n");
			break;
		case diff_formal_param_type_and_actual:
			printer_printf(&context->err_options, "типы формального и фактического параметров различаются\n");
			break;
		case float_in_condition:
			printer_printf(&context->err_options, "условие должно иметь тип ЦЕЛ или ЛИТЕРА\n");
			break;
		case case_or_default_not_in_switch:
			printer_printf(&context->err_options, "метка СЛУЧАЙ или УМОЛЧАНИЕ не в операторе ВЫБОР\n");
			break;
		case break_not_in_loop_or_switch:
			printer_printf(&context->err_options, "оператор ВЫХОД не в цикле и не в операторе ВЫБОР\n");
			break;
		case continue_not_in_loop:
			printer_printf(&context->err_options, "оператор ПРОДОЛЖИТЬ не в цикле\n");
			break;
		case not_primary:
			printer_printf(&context->err_options, "первичное не может начинаться с лексемы %i\n", context->cur);
			break;
		case wrong_operand:
			printer_printf(&context->err_options, "операнд операции может иметь только тип ЦЕЛ, ЛИТ или ВЕЩ\n");
			break;
		case label_not_declared:
			printer_printf(&context->err_options, "в строке %i переход на неописанную метку ", context->hash);
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, "\n");
			break;
		case repeated_label:
			printer_printf(&context->err_options, "повторное описание метки ");
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, "\n");
			break;
		case operand_is_pointer:
			printer_printf(&context->err_options, "операнд бинарной формулы не может быть указателем\n");
			break;
		case pointer_in_print:
			printer_printf(&context->err_options, "указатели нельзя печатать\n");
			break;
		case wrong_struct:
			printer_printf(&context->err_options, "неправильное описание структуры\n");
			break;
		case after_dot_must_be_ident://OK--
			printer_printf(&context->err_options, "после . или -> должен быть идентификатор-имя поля "
												  "структуры\n");
			break;
		case get_field_not_from_struct_pointer:
			printer_printf(&context->err_options, "применять операцию -> можно только к указателю на "
												  "структуру\n");
			break;

		case error_in_initialization:
			printer_printf(&context->err_options, "Несоотетствие типов при инициализации переменной\n");
			break;
		case type_missmatch:
			printer_printf(&context->err_options, "Несоответствие типов\n");
			break;
		case array_assigment:
			printer_printf(&context->err_options, "Присваивание в массив запрещено\n");
			break;
		case wrong_struct_ass:
			printer_printf(&context->err_options, "Для структур и указателей допустима только операция "
												  "присваивания =\n");
			break;
		case wrong_init:
			printer_printf(&context->err_options, "переменные такого типа нельзя инициализировать\n");
			break;
		case no_field:
			printer_printf(&context->err_options, "нет такого поля ");
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, " в структуре");
			printer_printf(&context->err_options, "\n");
			break;
		case slice_from_func:
			printer_printf(&context->err_options, "вырезка элемента из массива, выданного функцией, а функции "
												  "не могут выдавать массивы\n");
			break;
		case bad_toval:
			printer_printf(&context->err_options, "странный toval ansttype=%i\n", context->ansttype);
			break;
		case wait_end://OK?+
			printer_printf(&context->err_options, "в инициализации структуры здесь ожидалась правая фигурная "
												  "скобка }\n");
			break;
		case act_param_not_ident:
			printer_printf(&context->err_options, "фактическим параметром-функцией может быть только "
												  "идентификатор\n");
			break;
		case unassignable:
			printer_printf(&context->err_options, "в левой части присваивания стоит что-то, чему нельзя "
												  "присваивать\n");
			break;
		case pnt_before_array:
			printer_printf(&context->err_options, "в РуСи не бывает указателей на массивы\n");
			break;
		case array_size_must_be_int:
			printer_printf(&context->err_options, "размер массива может иметь тип только ЦЕЛ или ЛИТЕРА\n");
			break;
		case no_semicomma_in_struct://OK?+
			printer_printf(&context->err_options, "описание поля структуры должно заканчиваться ;\n");
			break;
		case wait_ident_after_semicomma_in_struct://OK--
			printer_printf(&context->err_options, "в структуре после типа поля должен идти идентификатор поля\n");
			break;
		case empty_init:
			printer_printf(&context->err_options, "в РуСи можно определять границы массива по инициализации "
												  "только по младшему измерению\n");
			break;
		case ident_not_type:
			printer_printf(&context->err_options, "в качестве описателя можно использовать только "
												  "идентификаторы, описанные как типы\n");
			break;
		case not_decl:
			printer_printf(&context->err_options, "здесь должен быть тип (стандартный или описанный "
												  "пользователем)\n");
			break;
		case predef_but_notdef:
			printer_printf(&context->err_options, "функция ");
			printident(context, REPRTAB_POS);
			printer_printf(&context->err_options, " была предопределена, но не описана\n");
			break;
		case print_without_br://OK
			printer_printf(&context->err_options, "операнд оператора печати должен быть в круглых скобках ()\n");
			break;
		case select_not_from_struct:
			printer_printf(&context->err_options, "выборка поля . не из структуры\n");
			break;
		case init_not_struct:
			printer_printf(&context->err_options, "в РуСи только структуре можно присвоить или передать "
												  "параметром запись {,,,}\n");
			break;
		case param_threads_not_int:
			printer_printf(&context->err_options, "процедуры, управляющие параллельными нитями, могут иметь "
												  "только целые параметры\n");
			break;
		case wrong_arg_in_send:
			printer_printf(&context->err_options, "неправильный тип аргумента в процедуре t_msg_send, должен "
												  "иметь тип msg_info\n");
			break;
		case wrong_arg_in_create:
			printer_printf(&context->err_options, "неправильный тип аргумента в процедуре t_create, должен "
												  "иметь тип void*(void*)\n");
			break;

		case no_leftbr_in_printf://OK
			printer_printf(&context->err_options, "Не хватает открывающей скобки в printf/печатьф\n");
			break;
		case no_rightbr_in_printf:
			printer_printf(&context->err_options, "Не хватает закрывающей скобки в printf/печатьф\n");
			break;
		case wrong_first_printf_param:
			printer_printf(&context->err_options, "Первым параметром в printf/печатьф должна быть константная "
												  "форматная строка\n");
			break;
		case wrong_printf_param_type:
			printer_printf(&context->err_options, "Тип параметра printf/печатьф не соответствует "
												  "спецификатору: %%");
			printer_printchar(&context->err_options, context->bad_printf_placeholder);
			switch (context->bad_printf_placeholder)
			{
				case 'i':
				case 1094: // 'ц'
					printer_printf(&context->err_options, " ожидает целое число\n");
					break;

				case 'c':
					printer_printf(&context->err_options, " (англ.)");
				case 1083: // л
					printer_printf(&context->err_options, " ожидает литеру\n");
					break;

				case 'f':
				case 1074: // в
					printer_printf(&context->err_options, " ожидает вещественное число\n");
					break;

				case 1089: // с
					printer_printf(&context->err_options, " (русск.)");
				case 's':
					printer_printf(&context->err_options, " ожидает строку\n");
					break;
				default:
					printer_printf(&context->err_options, " -- неизвестный спецификатор");
			}
			break;
		case wrong_printf_param_number:
			printer_printf(&context->err_options, "Количество параметров printf/печатьф не соответствует "
												  "количеству спецификаторов\n");
			break;
		case printf_no_format_placeholder:
			printer_printf(&context->err_options, "В printf/печатьф нет спецификатора типа после '%%'\n");
			break;
		case printf_unknown_format_placeholder:
			printer_printf(&context->err_options, "В printf/печатьф неизвестный спецификатор типа %%");
			printer_printchar(&context->err_options, context->bad_printf_placeholder);
			printer_printf(&context->err_options, "\n");
			break;
		case too_many_printf_params:
			printer_printf(&context->err_options, "Максимально в printf/печатьф можно выводить %i значений\n",
						   MAXPRINTFPARAMS);
			break;

		case no_mult_in_cast: //OK
			printer_printf(&context->err_options, "нет * в cast (приведении)\n");
			break;
		case no_rightbr_in_cast://OK
			printer_printf(&context->err_options, "нет ) в cast (приведении)\n");
			break;
		case not_pointer_in_cast:
			printer_printf(&context->err_options, "cast (приведение) может быть применено только к указателю\n");
			break;
		case empty_bound_without_init:
			printer_printf(&context->err_options, "в описании массива границы не указаны, а инициализации нет\n");
			break;
		case begin_with_notarray:
			printer_printf(&context->err_options, "инициализация, начинающаяся с {, должна соответствовать "
												  "массиву или структуре\n");
			break;
		case string_and_notstring:
			printer_printf(&context->err_options, "если в инициализаторе встретилась строка, то и дальше "
												  "должны быть только строки\n");
			break;
		case wrong_init_in_actparam:
			printer_printf(&context->err_options, "в инициализаторе-фактическом параметре функции могут быть "
												  "только константы\n");
			break;
		case no_comma_or_end:
			printer_printf(&context->err_options, "в инициализаторе ожидали , или }\n");
			break;
		case no_comma_in_act_params_stanfunc://OK
			printer_printf(&context->err_options, "в операции над строками после параметра нет , \n");
			break;
		case not_string_in_stanfunc:
			printer_printf(&context->err_options, "в операции над строками параметр не строка\n");
			break;
		case not_int_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции этот параметр должен иметь тип ЦЕЛ\n");
			break;
		case not_float_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции этот параметр должен иметь тип ВЕЩ\n");
			break;
		case not_point_string_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции над строками первый параметр должен быть "
												  "указателем на строку\n");
			break;
		case not_rowofint_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции этот параметр должен иметь тип массив "
												  "целых\n");
			break;
		case not_rowoffloat_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции этот параметр должен иметь тип массив вещ\n");
			break;
		case not_array_in_stanfunc:
			printer_printf(&context->err_options, "в этой операции этот параметр должен иметь тип массив\n");
			break;
		default:
			printer_printf(&context->err_options, "этот код ошибки я прозевал\n");
	}
	//exit(2);
}

void set_errors_output(compiler_context *context, char *path)
{
	compiler_context_detach_io(context, IO_TYPE_ERROR);
	compiler_context_attach_io(context, path, IO_TYPE_ERROR, IO_SOURCE_FILE);
}

int get_exit_code(compiler_context *context)
{
	return context->error_flag2 != 0 || context->error_flag != 0;
}
