#include <stdio.h>
#include <stdlib.h>

extern void tablesandtree();
#include "global_vars.h"
extern void printf_char(int wchar);
extern void fprintf_char(FILE*, int wchar);
void printident(int r)
{
    r += 2;                      // ссылка на reprtab
    do
        fprintf_char(stderr, reprtab[r++]);
    while (reprtab[r] != 0);

}

void warning(int ernum)
{
    switch (ernum)
    {
        case too_long_int:
            fprintf(stderr, "слишком большая целая константа, преобразована в ДЛИН (DOUBLE)\n");
            break;
            
        default:
            break;
    }
}

void error(int ernum)
{
    int i;
    tablesandtree();
    fprintf(stderr, "line %i) ", line);
    for (i=lines[line]; i<charnum; i++)
        fprintf_char(stderr, source[i]);
    fprintf(stderr, "\n");
    switch (ernum)
    {
        case after_type_must_be_ident:
            fprintf(stderr, "после символа типа должен быть идентификатор или * идентификатор\n");
            break;
        case wait_right_sq_br:
            fprintf(stderr, "ожидалась ]\n");
            break;
        case only_functions_may_have_type_VOID:
            fprintf(stderr, "только функции могут иметь тип ПУСТО\n");
            break;
        case decl_and_def_have_diff_type:
            fprintf(stderr, "прототип функции и ее описание имеют разные типы\n");
            break;
        case decl_must_start_from_ident_or_decl:
            fprintf(stderr, "описание может начинаться только с описателя или идентификатора\n");
            break;
        case no_comma_in_param_list:
            fprintf(stderr, "параметры должны разделяться запятыми\n");
            break;
        case wrong_param_list:
            fprintf(stderr, "неправильный список параметров\n");
            break;
        case no_comma_in_type_list:
            fprintf(stderr, "типы должны разделяться запятыми\n");
            break;
        case wrong_type_list:
            fprintf(stderr, "неправильный список типов\n");
            break;
        case func_def_must_be_first:
            fprintf(stderr, "определение функции должно быть первым в списке описаний\n");
            break;
        case func_def_must_have_param_list:
            fprintf(stderr, "определение функции должно иметь список параметров, а не типов\n");
            break;
        case def_must_end_with_semicomma:
            fprintf(stderr, "список описаний должен заканчиваться ;\n");
            break;
        case func_and_protot_have_dif_num_params:
            fprintf(stderr, "функция и прототип имеют разное количество параметров\n");
            break;
        case param_types_are_dif:
            fprintf(stderr, "не совпадают типы параметров функции и прототипа\n");
            break;
        case wait_ident_after_comma_in_decl:
            fprintf(stderr, "в описании после , ожидается идентификатор или * идентификатор\n");
            break;
        case wait_rightbr_in_call:
            fprintf(stderr, "нет ) в вызове функции\n");
            break;
        case func_decl_req_params:
            fprintf(stderr, "вообще-то я думал, что это предописание функции (нет идентификаторов-параметров), а тут тело функции\n");
            break;
        case wait_while_in_do_stmt:
            fprintf(stderr, "ждем ПОКА в операторе ЦИКЛ\n");
            break;
        case no_semicolon_after_stmt:
            fprintf(stderr, "нет ; после оператора\n");
            break;
        case cond_must_be_in_brkts:
            fprintf(stderr, "условие должно быть в ()\n");
            break;
        case repeated_decl:
            fprintf(stderr, "повторное описание идентификатора ");
            printident(repr);
            fprintf(stderr, "\n");
            break;
        case arr_init_must_start_from_BEGIN:
            fprintf(stderr, "инициализация массива или структуры должна начинаться со {\n");
            break;
        case no_comma_in_init_list:
            fprintf(stderr, "между элементами инициализации массива или структуры должна быть ,\n");
            break;
        case ident_is_not_declared:
            fprintf(stderr, "не описан идентификатор ");
            printident(repr);
            fprintf(stderr, "\n");
            break;
        case no_rightsqbr_in_slice:
            fprintf(stderr, "не хватает ] в вырезке элемента массива\n");
            break;
        case void_in_expr:
            fprintf(stderr, "в выражении встретился элемент типа ПУСТО\n");
            break;
        case index_must_be_int:
            fprintf(stderr, "индекс элемента массива должен иметь тип ЦЕЛ\n");
            break;
        case slice_not_from_array:
            fprintf(stderr, "попытка вырезки элемента не из массива\n");
            break;
        case call_not_from_function:
            fprintf(stderr, "попытка вызова не функции\n");
            break;
        case no_comma_in_act_params:
            fprintf(stderr, "после фактического параметра должна быть ,\n");
            break;
        case float_instead_int:
            fprintf(stderr, "формальный параметр имеет тип ЦЕЛ, а фактический - ВЕЩ\n");
            break;
        case wrong_number_of_params:
            fprintf(stderr, "неправильное количество фактических параметров\n");
            break;
        case wait_rightbr_in_primary:
            fprintf(stderr, "не хватает ) в первичном выражении\n");
            break;
        case unassignable_inc:
            fprintf(stderr, "++ и -- применимы только к переменным и элементам массива\n");
            break;
        case wrong_addr:
            fprintf(stderr, "операция получения адреса & применима только к переменным\n");
            break;
        case no_colon_in_cond_expr:
            fprintf(stderr, "нет : в условном выражении\n");
            break;
        case not_assignable:
            fprintf(stderr, "слева от присваивания или операции с присваиванием может быть только переменная или элемент массива\n");
            break;
        case func_not_in_call:
            fprintf(stderr, "функция может быть использована только в вызове\n");
            break;
        case no_colon_in_case:
            fprintf(stderr, "после выражения в выборе нет :\n");
            break;
        case case_after_default:
            fprintf(stderr, "встретился выбор после умолчания\n");
            break;
        case no_ident_after_goto:
            fprintf(stderr, "после goto должна быть метка, т.е. идентификатор\n");
            break;
        case no_leftbr_in_for:
            fprintf(stderr, "в операторе цикла ДЛЯ нет (\n");
            break;
        case no_semicolon_in_for:
            fprintf(stderr, "в операторе цикла ДЛЯ нет ;\n");
            break;
        case no_rightbr_in_for:
            fprintf(stderr, "в операторе цикла ДЛЯ нет )\n");
            break;
        case int_op_for_float:
            fprintf(stderr, "операция, применимая только к целым, применена к вещественному аргументу\n");
            break;
        case assmnt_float_to_int:
            fprintf(stderr, "нельзя присваивать целому вещественное значение\n");
            break;
        case more_than_1_main:
            fprintf(stderr, "в программе может быть только 1 идентификатор ГЛАВНАЯ\n");
            break;
        case no_main_in_program:
            fprintf(stderr, "в каждой программе должна быть ГЛАВНАЯ функция\n");
            break;
        case no_leftbr_in_printid:
            fprintf(stderr, "в колманде ПЕЧАТЬИД или ЧИТАТЬИД нет (\n");
            break;
        case no_rightbr_in_printid:
            fprintf(stderr, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )\n");
            break;
        case no_ident_in_printid:
            fprintf(stderr, "в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора\n");
            break;
        case float_in_switch:
            fprintf(stderr, "в условии переключателя можно использовать только типы ЛИТЕРА и ЦЕЛ\n");
            break;
        case init_int_by_float:
            fprintf(stderr, "целая или литерная переменная инициализируется значением типа ВЕЩ\n");
            break;
        case must_be_digit_after_dot:
            fprintf(stderr, "должна быть цифра перед или после .\n");
            break;
        case must_be_digit_after_exp:
            fprintf(stderr, "должна быть цифра после e\n");
            break;
        case no_leftbr_in_setmotor:
            fprintf(stderr, "в команде ПУСКМОТОРА нет(\n");
            break;
        case no_rightbr_in_setmotor:
            fprintf(stderr, "в команде ПУСКМОТОРА нет)\n");
            break;
        case no_comma_in_setmotor:
            fprintf(stderr, "в команде ПУСКМОТОРА после первого параметра нет ,\n");
            break;
        case param_setmotor_not_int:
            fprintf(stderr, "в командах ПУСКМОТОРА, СПАТЬ, ЦИФРДАТЧИК и АНАЛОГДАТЧИК параметры должны быть целыми\n");
            break;
        case no_leftbr_in_sleep:
            fprintf(stderr, "в команде СПАТЬ нет(\n");
            break;
        case no_rightbr_in_sleep:
            fprintf(stderr, "в команде СПАТЬ нет)\n");
            break;
        case no_leftbr_in_stand_func:
            fprintf(stderr, "в вызове  стандартной функции нет (\n");
            break;
        case no_rightbr_in_stand_func:
            fprintf(stderr, "в вызове  стандартной функции нет )\n");
            break;
        case bad_param_in_stand_func:
            fprintf(stderr, "параметры стандартных функций могут быть только целыми и вещественными\n");
            break;
        case no_ret_in_func:
            fprintf(stderr, "в функции, возвращающей непустое значение, нет оператора ВОЗВРАТ со значением\n");
            break;
        case bad_type_in_ret:
            fprintf(stderr, "в функции, возвращающей целое или литерное значение, оператор ВОЗВРАТ со значением ВЕЩ\n");
            break;
        case notvoidret_in_void_func:
            fprintf(stderr, "в функции, возвращающей пустое значение, оператор ВОЗВРАТ со значением\n");
            break;
        case bad_escape_sym:
            fprintf(stderr, "неизвестный служебный символ\n");
            break;
        case no_right_apost:
            fprintf(stderr, "символьная константа не заканчивается символом '\n");
            break;
        case decl_after_strmt:
            fprintf(stderr, "встретилось описание после оператора\n");
            break;
        case too_long_string:
            fprintf(stderr, "слишком длинная строка ( больше, чем MAXSTRINGL)\n");
            break;
        case no_ident_after_aster:
            fprintf(stderr, "в описании параметра функции после * нет идентификатора\n");
            break;
        case aster_before_func:
            fprintf(stderr, "* перед описанием функции\n");
            break;
        case aster_not_for_pointer:
            fprintf(stderr, "операция * применяется не к указателю\n");
            break;
        case aster_with_row:
            fprintf(stderr, "операцию * нельзя применять к массивам\n");
            break;
        case wrong_fun_as_param:
            fprintf(stderr, "неправильная запись функции, передаваемой параметром в другую функцию\n");
            break;
        case no_right_br_in_paramfun:
            fprintf(stderr, "нет ) в функции, передаваемой параметром в другую функцию\n");
            break;
        case no_ident_in_paramfun:
            fprintf(stderr, "нет идентификатора в  параметре определения функции\n");
            break;
        case par_type_void_with_nofun:
            fprintf(stderr, "в параметре функции тип пусто может быть только у параметра-функции\n");
            break;
        case ident_in_declarator:
            fprintf(stderr, "в деклараторах (предописаниях) могут быть только типы, но без идентификаторов-параметров\n");
            break;
        case array_before_func:
            fprintf(stderr, "функция не может выдавать значение типа массив\n");
            break;
        case wait_definition:
            fprintf(stderr, "вообще-то, я думал, что это определение функции, а тут нет идентификатора-параметра\n");
            break;
        case wait_declarator:
            fprintf(stderr, "вообще-то, я думал, что это предописание функции, а тут идентификатор-параметр\n");
            break;
        case two_idents_for_1_declarer:
            fprintf(stderr, "в описании функции на каждый описатель должен быть один параметр\n");
            break;
        case function_has_no_body:
            fprintf(stderr, "есть параметры определения функции, но нет блока, являющегося ее телом\n");
            break;
        case declarator_in_call:
            fprintf(stderr, "предописание нельзя использовать в вызове\n");
            break;
        case diff_formal_param_type_and_actual:
            fprintf(stderr, "типы формального и фактического параметров различаются\n");
            break;
        case float_in_condition:
            fprintf(stderr, "условие должно иметь тип ЦЕЛ или ЛИТЕРА\n");
            break;
        case case_or_default_not_in_switch:
            fprintf(stderr, "метка СЛУЧАЙ или УМОЛЧАНИЕ не в операторе ВЫБОР\n");
            break;
        case break_not_in_loop_or_switch:
            fprintf(stderr, "оператор ВЫХОД не в цикле и не в операторе ВЫБОР\n");
            break;
        case continue_not_in_loop:
            fprintf(stderr, "оператор ПРОДОЛЖИТЬ не в цикле\n");
            break;
        case not_primary:
            fprintf(stderr, "первичное не  может начинаться с лексемы %i\n", cur);
            break;
        case wrong_operand:
            fprintf(stderr, "операнд операции может иметь только тип ЦЕЛ, ЛИТ или ВЕЩ\n");
            break;
        case label_not_declared:
            fprintf(stderr, "в строке %i переход на неописанную метку ", hash);
            printident(repr);
            fprintf(stderr, "\n");
            break;
        case repeated_label:
            fprintf(stderr, "повторное описание метки ");
            printident(repr);
            fprintf(stderr, "\n");
            break;
        case wrong_pnt_assn:
            fprintf(stderr, "в присваивании указателей не совпадают типы\n");
            break;
        case comm_not_ended:
            fprintf(stderr, "комментарий, начавшийся с /* , не закрыт\n");
            break;
        case operand_is_pointer:
            fprintf(stderr, "операнд бинарной формулы не может быть указателем\n");
            break;
        case pointer_in_print:
            fprintf(stderr, "указатели нельзя печатать\n");
            break;
        case wrong_struct:
            fprintf(stderr, "неправильное описание структуры\n");
            break;
		case after_dot_must_be_ident:
			fprintf(stderr, "после . или -> должен быть идентификатор-имя поля структуры\n");
			break; 
		case field_not_found:
			fprintf(stderr, "у структуры нет такого поля\n");
			break;
		case get_field_not_from_struct:
			fprintf(stderr, "применять операцию . можно только к структуре\n");
			break;
		case get_field_not_from_struct_pointer:
			fprintf(stderr, "применять операцию -> можно только к указателю на структуру\n");
			break;
        case get_field_not_from_struct_pointer1:
            fprintf(stderr, "применять операцию -> можно только к указателю на структуру, а тут что-то странное\n");
            break;

		case error_in_initialization:
			fprintf(stderr, "Несоотетствие типов при инициализации переменной\n");
			break;
		case error_in_array_initialization:
			fprintf(stderr, "Несоответствие типов при инициализации массива\n");
			break;
		case type_missmatch:
			fprintf(stderr, "Несоответствие типов\n");
			break;
		case array_assigment:
			fprintf(stderr, "Присваивание в массив запрещено\n");
			break;
		case wrong_struct_ass:
			fprintf(stderr, "Для структур и указателей допустима только операция присваивания =\n");
			break;
        case not_enough_expr:
            fprintf(stderr, "в инициализации структуры меньше выражений, чем полей\n");
            break;
        case wrong_init:
            fprintf(stderr, "переменные такого типа нельзя инициализировать\n");
            break;
        case wrong_array_init:
            fprintf(stderr, "инициализировать можно только массивы с константными границами\n");
            break;
        case too_many_elems:
            fprintf(stderr, "в инициализации массива элементов больше, чем в массиве\n");
            break;
        case no_field:
            fprintf(stderr, "нет такого поля ");
            printident(repr);
            fprintf(stderr, " в структуре");
            fprintf(stderr, "\n");
            break;
        case slice_from_func:
            fprintf(stderr, "вырезка элемента из массива, выданного функцией, а функции не могут выдавать массивы\n");
            break;
        case bad_toval:
            fprintf(stderr, "странный toval ansttype=%i\n", ansttype);
            break;
        case wait_end:
            fprintf(stderr, "в инициализации структуры здесь ожидалась правая фигурная скобка }\n");
            break;
        case act_param_not_ident:
            fprintf(stderr, "фактическим параметром-функцией может быть только идентификатор\n");
            break;
        case unassignable:
            fprintf(stderr, "в левой части присваивания стоит что-то, чему нельзя присваивать\n");
            break;
        case pnt_before_array:
            fprintf(stderr, "в РуСи не бывает указателей на массивы\n");
            break;
        case array_size_must_be_int:
            fprintf(stderr, "размер массива может иметь тип только ЦЕЛ или ЛИТЕРА\n");
            break;
        case no_semicomma_in_struct:
            fprintf(stderr, "описание поля структуры должно заканчиваться ;\n");
            break;
        case wait_ident_after_semicomma_in_struct:
            fprintf(stderr, "в структуре после типа поля должен идти идентификатор поля\n");
            break;
        case empty_init:
            fprintf(stderr, "в РуСи можно определять границы массива по инициализации только для одномерных массивов\n");
            break;
        case ident_not_type:
            fprintf(stderr, "в качестве описателя можно использовать только идентификаторы, описанные как типы\n");
            break;
        case not_decl:
            fprintf(stderr, "здесь должен быть тип (стандартный или описанный пользователем)\n");
            break;
        case predef_but_notdef:
            fprintf(stderr, "функция ");
            printident(repr);
            fprintf(stderr, " была предопределена, но не описана\n");
            break;
        case print_without_br:
            fprintf(stderr, "операнд оператора печати должен быть в круглых скобках ()\n");
            break;
        case select_not_from_struct:
            fprintf(stderr, "выборка поля . не из структуры\n");
            break;
        case select_from_func_value:
            fprintf(stderr, "в РуСи структуру-значение функции можно только присвоить или передать параметром\n");
            break;
        case init_not_struct:
            fprintf(stderr, "в РуСи только структуре можно присвоить или передать параметром запись {,,,}\n");
            break;
        case param_threads_not_int:
            fprintf(stderr, "процедуры, управляющие параллельными нитями, могут иметь только целые параметры\n");
            break;
        case wrong_arg_in_send:
            fprintf(stderr, "неправильный тип аргумента в процедуре t_msg_send, должен иметь тип msg_info\n");
            break;
        case wrong_arg_in_create:
            fprintf(stderr, "неправильный тип аргумента в процедуре t_create, должен иметь тип void*(void*)\n");
            break;
            
        case else_after_elif:
            fprintf(stderr, "ошибка препроцессора: #elif после #else\n");
            break;
        case sh_if_not_found:
            fprintf(stderr, "ошибка препроцессора: встречено ключевое слово #elif или #else или #endif, но не было #if(или #ifdef)\n");
            break;
        case no_ident_after_define:
            fprintf(stderr, "ошибка препроцессора: не найден идентификатор после #define\n");
            break;
        case endif_not_found:
            fprintf(stderr, "ошибка препроцессора: не найден #endif\n");
            break;
        case macro_params_not_found:
            fprintf(stderr, "ошибка препроцессора: не найдены параметры для макроподстановки\n");
            break;
        case wait_ident_after_comma_in_macro_params:
            fprintf(stderr, "ошибка препроцессора: ожидается идент после запятой в параметрах макроподстановки\n");
            break;
        case wait_rightbr_in_macro_params:
            fprintf(stderr, "ошибка препроцессора: ожидается закрывающая скобка в параметрах макроподстановки\n");
            break;
        case params_count_not_equals_in_macro:
            fprintf(stderr, "ошибка препроцессора: количество параметров в макроподстановке не совпадает с заданным\n");
            break;

        case no_leftbr_in_printf:
            fprintf(stderr, "Не хватает левой скобки в printf/печатьф\n");
            break;
        case no_rightbr_in_printf:
            fprintf(stderr, "Не хватает правой скобки в printf/печатьф\n");
            break;
        case wrong_first_printf_param:
            fprintf(stderr, "Первым параметром в printf/печатьф должна быть константная форматная строка\n");
            break;
        case wrong_printf_param_type:
            fprintf(stderr, "Тип параметра printf/печатьф не соответствует спецификатору\n");
            break;
        case wrong_printf_param_number:
            fprintf(stderr, "Кодичество параметров printf/печатьф не соответствует количеству спецификаторов\n");
            break;
        case printf_no_format_placeholder:
            fprintf(stderr, "В printf/печатьф нет спецификатора типа после '%%'\n");
            break;
        case printf_unknown_format_placeholder:
            fprintf(stderr, "В printf/печатьф неизвестный спецификатор типа '");
            fprintf_char(stderr, bad_placeholder);
            fprintf(stderr, "'\n");
            break;
        case no_mult_in_cast:
            fprintf(stderr, "нет * в cast (приведении)\n");
            break;
        case no_rightbr_in_cast:
            fprintf(stderr, "нет ) в cast (приведении)\n");
            break;
        case not_pointer_in_cast:
            fprintf(stderr, "cast (приведение) может быть применено только к указателю\n");
            break;
            
        default: ;
    }
    exit(2);
}
