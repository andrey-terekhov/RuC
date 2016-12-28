#include <stdio.h>
#include <stdlib.h>

extern void tablesandtree();
#include "global_vars.h"
extern void printf_char(int wchar);
extern void printident(int r);

void error(int ernum)
{
    int i;
    tablesandtree();
    printf("line %i) ", line);
    for (i=lines[line]; i<charnum; i++)
        printf_char(source[i]);

    printf("\n");
    switch (ernum)
    {
        case INTERNAL_COMPILER_ERROR:
            printf("\x1b[93;41mОбнаружена странная внутренняя проблема РуСи, обратитесь за внешней помощью!\x1b[0m\n");
            exit(-1);
            break;
        case after_type_must_be_ident:
            printf("после символа типа должен быть идентификатор или * идентификатор\n");
            break;
        case wait_right_sq_br:
            printf("ожидалась ]\n");
            break;
        case only_functions_may_have_type_VOID:
            printf("только функции могут иметь тип ПУСТО\n");
            break;
        case decl_and_def_have_diff_type:
            printf("прототип функции и ее описание имеют разные типы\n");
            break;
        case decl_must_start_from_ident_or_decl:
            printf("описание может начинаться только с описателя или идентификатора\n");
            break;
        case no_comma_in_param_list:
            printf("параметры должны разделяться запятыми\n");
            break;
        case wrong_param_list:
            printf("неправильный список параметров\n");
            break;
        case no_comma_in_type_list:
            printf("типы должны разделяться запятыми\n");
            break;
        case wrong_type_list:
            printf("неправильный список типов\n");
            break;
        case func_def_must_be_first:
            printf("определение функции должно быть первым в списке описаний\n");
            break;
        case func_def_must_have_param_list:
            printf("определение функции должно иметь список параметров, а не типов\n");
            break;
        case def_must_end_with_semicomma:
            printf("список описаний должен заканчиваться ;\n");
            break;
        case func_and_protot_have_dif_num_params:
            printf("функция и прототип имеют разное количество параметров\n");
            break;
        case param_types_are_dif:
            printf("не совпадают типы параметров функции и прототипа\n");
            break;
        case wait_ident_after_comma_in_decl:
            printf("в описании после , ожидается идентификатор или * идентификатор\n");
            break;
        case wait_rightbr_in_call:
            printf("нет ) в вызове функции\n");
            break;
        case func_decl_req_params:
            printf("вообще-то я думал, что это предописание функции (нет идентификаторов-параметров), а тут тело функции\n");
            break;
        case wait_while_in_do_stmt:
            printf("ждем ПОКА в операторе ЦИКЛ\n");
            break;
        case no_semicolon_after_stmt:
            printf("нет ; после оператора\n");
            break;
        case cond_must_be_in_brkts:
            printf("условие должно быть в ()\n");
            break;
        case repeated_decl:
            printf("повторное описание идентификатора ");
            printident(repr);
            printf("\n");
            break;
        case arr_init_must_start_from_BEGIN:
            printf("инициализация массива или структуры должна начинаться со {\n");
            break;
        case no_comma_in_init_list:
            printf("между элементами инициализации массива или структуры должна быть ,\n");
            break;
        case ident_is_not_declared:
            printf("не описан идентификатор ");
            printident(repr);
            printf("\n");
            break;
        case no_rightsqbr_in_slice:
            printf("не хватает ] в вырезке элемента массива\n");
            break;
        case void_in_expr:
            printf("в выражении встретился элемент типа ПУСТО\n");
            break;
        case index_must_be_int:
            printf("индекс элемента массива должен иметь тип ЦЕЛ\n");
            break;
        case slice_not_from_array:
            printf("попытка вырезки элемента не из массива\n");
            break;
        case call_not_from_function:
            printf("попытка вызова не функции\n");
            break;
        case no_comma_in_act_params:
            printf("после фактического параметра должна быть ,\n");
            break;
        case float_instead_int:
            printf("формальный параметр имеет тип ЦЕЛ, а фактический - ВЕЩ\n");
            break;
        case wrong_number_of_params:
            printf("неправильное количество фактических параметров\n");
            break;
        case wait_rightbr_in_primary:
            printf("не хватает ) в первичном выражении\n");
            break;
        case unassignable_inc:
            printf("++ и -- применимы только к переменным и элементам массива\n");
            break;
        case wrong_addr:
            printf("операция получения адреса & применима только к переменным\n");
            break;
        case no_colon_in_cond_expr:
            printf("нет : в условном выражении\n");
            break;
        case not_assignable:
            printf("слева от присваивания или операции с присваиванием может быть только переменная или элемент массива\n");
            break;
        case func_not_in_call:
            printf("функция может быть использована только в вызове\n");
            break;
        case no_colon_in_case:
            printf("после выражения в выборе нет :\n");
            break;
        case case_after_default:
            printf("встретился выбор после умолчания\n");
            break;
        case no_ident_after_goto:
            printf("после goto должна быть метка, т.е. идентификатор\n");
            break;
        case no_leftbr_in_for:
            printf("в операторе цикла ДЛЯ нет (\n");
            break;
        case no_semicolon_in_for:
            printf("в операторе цикла ДЛЯ нет ;\n");
            break;
        case no_rightbr_in_for:
            printf("в операторе цикла ДЛЯ нет )\n");
            break;
        case int_op_for_float:
            printf("операция, применимая только к целым, применена к вещественному аргументу\n");
            break;
        case assmnt_float_to_int:
            printf("нельзя присваивать целому вещественное значение\n");
            break;
        case more_than_1_main:
            printf("в программе может быть только 1 идентификатор ГЛАВНАЯ\n");
            break;
        case no_main_in_program:
            printf("в каждой программе должна быть ГЛАВНАЯ функция\n");
            break;
        case no_leftbr_in_printid:
            printf("в колманде ПЕЧАТЬИД или ЧИТАТЬИД нет (\n");
            break;
        case no_rightbr_in_printid:
            printf("в команде ПЕЧАТЬИД или ЧИТАТЬИД нет )\n");
            break;
        case no_ident_in_printid:
            printf("в команде ПЕЧАТЬИД или ЧИТАТЬИД нет идентификатора\n");
            break;
        case float_in_switch:
            printf("в условии переключателя можно использовать только типы ЛИТЕРА и ЦЕЛ\n");
            break;
        case init_int_by_float:
            printf("целая или литерная переменная инициализируется значением типа ВЕЩ\n");
            break;
        case must_be_digit_after_dot:
            printf("должна быть цифра перед или после .\n");
            break;
        case must_be_digit_after_exp:
            printf("должна быть цифра после e\n");
            break;
        case no_leftbr_in_setmotor:
            printf("в команде ПУСКМОТОРА нет(\n");
            break;
        case no_rightbr_in_setmotor:
            printf("в команде ПУСКМОТОРА нет)\n");
            break;
        case no_comma_in_setmotor:
            printf("в команде ПУСКМОТОРА после первого параметра нет ,\n");
            break;
        case param_setmotor_not_int:
            printf("в командах ПУСКМОТОРА, СПАТЬ, ЦИФРДАТЧИК и АНАЛОГДАТЧИК параметры должны быть целыми\n");
            break;
        case no_leftbr_in_sleep:
            printf("в команде СПАТЬ нет(\n");
            break;
        case no_rightbr_in_sleep:
            printf("в команде СПАТЬ нет)\n");
            break;
        case no_leftbr_in_stand_func:
            printf("в вызове  стандартной функции нет (\n");
            break;
        case no_rightbr_in_stand_func:
            printf("в вызове  стандартной функции нет )\n");
            break;
        case bad_param_in_stand_func:
            printf("параметры стандартных функций могут быть только целыми и вещественными\n");
            break;
        case no_ret_in_func:
            printf("в функции, возвращающей непустое значение, нет оператора ВОЗВРАТ со значением\n");
            break;
        case bad_type_in_ret:
            printf("в функции, возвращающей целое или литерное значение, оператор ВОЗВРАТ со значением ВЕЩ\n");
            break;
        case notvoidret_in_void_func:
            printf("в функции, возвращающей пустое значение, оператор ВОЗВРАТ со значением\n");
            break;
        case bad_escape_sym:
            printf("неизвестный служебный символ\n");
            break;
        case no_right_apost:
            printf("символьная константа не заканчивается символом '\n");
            break;
        case decl_after_strmt:
            printf("встретилось описание после оператора\n");
            break;
        case too_long_string:
            printf("слишком длинная строка ( больше, чем MAXSTRINGL)\n");
            break;
        case no_ident_after_aster:
            printf("в описании параметра функции после * нет идентификатора\n");
            break;
        case aster_before_func:
            printf("* перед описанием функции\n");
            break;
        case aster_not_for_pointer:
            printf("операция * применяется не к указателю\n");
            break;
        case aster_with_row:
            printf("операцию * нельзя применять к массивам\n");
            break;
        case wrong_fun_as_param:
            printf("неправильная запись функции, передаваемой параметром в другую функцию\n");
            break;
        case no_right_br_in_paramfun:
            printf("нет ) в функции, передаваемой параметром в другую функцию\n");
            break;
        case no_ident_in_paramfun:
            printf("нет идентификатора в  параметре определения функции\n");
            break;
        case par_type_void_with_nofun:
            printf("в параметре функции тип пусто может быть только у параметра-функции\n");
            break;
        case ident_in_declarator:
            printf("в деклараторах (предописаниях) могут быть только типы, но без идентификаторов-параметров\n");
            break;
        case array_before_func:
            printf("функция не может выдавать значение типа массив\n");
            break;
        case wait_definition:
            printf("вообще-то, я думал, что это определение функции, а тут нет идентификатора-параметра\n");
            break;
        case wait_declarator:
            printf("вообще-то, я думал, что это предописание функции, а тут идентификатор-параметр\n");
            break;
        case two_idents_for_1_declarer:
            printf("в описании функции на каждый описатель должен быть один параметр\n");
            break;
        case function_has_no_body:
            printf("есть параметры определения функции, но нет блока, являющегося ее телом\n");
            break;
        case declarator_in_call:
            printf("предописание нельзя использовать в вызове\n");
            break;
        case diff_formal_param_type_and_actual:
            printf("типы формального и фактического параметров различаются\n");
            break;
        case float_in_condition:
            printf("условие должно иметь тип ЦЕЛ или ЛИТЕРА\n");
            break;
        case case_or_default_not_in_switch:
            printf("метка СЛУЧАЙ или УМОЛЧАНИЕ не в операторе ВЫБОР\n");
            break;
        case break_not_in_loop_or_switch:
            printf("оператор ВЫХОД не в цикле и не в операторе ВЫБОР\n");
            break;
        case continue_not_in_loop:
            printf("оператор ПРОДОЛЖИТЬ не в цикле\n");
            break;
        case not_primary:
            printf("первичное не  может начинаться с лексемы %i\n", cur);
            break;
        case wrong_operand:
            printf("операнд операции может иметь только тип ЦЕЛ, ЛИТ или ВЕЩ\n");
            break;
        case label_not_declared:
            printf("в строке %i переход на неописанную метку ", hash);
            printident(repr);
            printf("\n");
            break;
        case repeated_label:
            printf("повторное описание метки ");
            printident(repr);
            printf("\n");
            break;
        case wrong_pnt_assn:
            printf("в присваивании указателей не совпадают типы\n");
            break;
        case comm_not_ended:
            printf("комментарий, начавшийся с /* , не закрыт\n");
            break;
        case operand_is_pointer:
            printf("операнд бинарной формулы не может быть указателем\n");
            break;
        case pointer_in_print:
            printf("указатели нельзя печатать\n");
            break;
        case wrong_struct:
            printf("неправильное описание структуры\n");
            break;
		case after_dot_must_be_ident:
			printf("после . или -> должен быть идентификатор-имя поля структуры\n");
			break; 
		case field_not_found:
			printf("у структуры нет такого поля\n");
			break;
		case get_field_not_from_struct:
			printf("применять операцию . можно только к структуре\n");
			break;
		case get_field_not_from_struct_pointer:
			printf("применять операцию -> можно только к указателю на структуру\n");
			break;
        case get_field_not_from_struct_pointer1:
            printf("применять операцию -> можно только к указателю на структуру, а тут что-то странное\n");
            break;

		case error_in_initialization:
			printf("Несоотетствие типов при инициализации переменной\n");
			break;
		case error_in_array_initialization:
			printf("Несоответствие типов при инициализации массива\n");
			break;
		case type_missmatch:
			printf("Несоответствие типов\n");
			break;
		case array_assigment:
			printf("Присваивание в массив запрещено\n");
			break;
		case wrong_struct_ass:
			printf("Для структур и указателей допустима только операция присваивания =\n");
			break;
        case not_enough_expr:
            printf("в инициализации структуры меньше выражений, чем полей\n");
            break;
        case wrong_init:
            printf("переменные такого типа нельзя инициализировать\n");
            break;
        case wrong_array_init:
            printf("инициализировать можно только массивы с константными границами\n");
            break;
        case too_many_elems:
            printf("в инициализации массива элементов больше, чем в массиве\n");
            break;
        case no_field:
            printf("нет такого поля ");
            printident(repr);
            printf(" в структуре");
            printf("\n");
            break;
        case slice_from_func:
            printf("вырезка элемента из массива, выданного функцией, а функции не могут выдавать массивы\n");
            break;
        case bad_toval:
            printf("странный toval ansttype=%i\n", ansttype);
            break;
        case wait_end:
            printf("в инициализации структуры здесь ожидалась правая фигурная скобка }\n");
            break;
        case act_param_not_ident:
            printf("фактическим параметром-функцией может быть только идентификатор\n");
            break;
        case unassignable:
            printf("в левой части присваивания стоит что-то, чему нельзя присваивать\n");
            break;
        case pnt_before_array:
            printf("в РуСи не бывает указателей на массивы\n");
            break;
        case array_size_must_be_int:
            printf("размер массива может иметь тип только ЦЕЛ или ЛИТЕРА\n");
            break;
        case no_semicomma_in_struct:
            printf("описание поля структуры должно заканчиваться ;\n");
            break;
        case wait_ident_after_semicomma_in_struct:
            printf("в структуре после типа поля должен идти идентификатор поля\n");
            break;
        case empty_init:
            printf("в РуСи можно определять границы массива по инициализации только для одномерных массивов\n");
            break;
        case ident_not_type:
            printf("в качестве описателя можно использовать только идентификаторы, описанные как типы\n");
            break;
        case not_decl:
            printf("здесь должен быть тип (стандартный или описанный пользователем)\n");
            break;
        case predef_but_notdef:
            printf("функция ");
            printident(repr);
            printf(" была предопределена, но не описана\n");
            break;
        case print_without_br:
            printf("операнд оператора печати должен быть в круглых скобках ()\n");
            break;
            
            
        default: ;
    }
    exit(2);
}
