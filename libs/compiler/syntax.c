/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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

#include "syntax.h"
#include <stdlib.h>
#include "defs.h"
#include "errors.h"
#include "tokens.h"
#include "old_tree.h"


void repr_add_keyword(map *const reprtab, const char32_t *const eng, const char32_t *const rus, const token_t token)
{
	char32_t buffer[MAXSTRINGL];

	buffer[0] = utf8_to_upper(eng[0]);
	for (size_t i = 1; eng[i - 1] != '\0'; i++)
	{
		buffer[i] = utf8_to_upper(eng[i]);
	}
	map_add_by_utf8(reprtab, eng, token);
	map_add_by_utf8(reprtab, buffer, token);

	buffer[0] = utf8_to_upper(rus[0]);
	for (size_t i = 1; rus[i - 1] != '\0'; i++)
	{
		buffer[i] = utf8_to_upper(rus[i]);
	}
	map_add_by_utf8(reprtab, rus, token);
	map_add_by_utf8(reprtab, buffer, token);
}

void repr_init(map *const reprtab)
{
	repr_add_keyword(reprtab, U"main", U"главная", kw_main);
	// repr_add_keyword(reprtab, U"auto", U"авто", kw_auto);
	// repr_add_keyword(reprtab, U"bool", U"булево", kw_bool);
	repr_add_keyword(reprtab, U"char", U"литера", kw_char);
	repr_add_keyword(reprtab, U"double", U"двойной", kw_double);
	// repr_add_keyword(reprtab, U"enum", U"перечень", kw_enum);
	repr_add_keyword(reprtab, U"float", U"вещ", kw_float);
	repr_add_keyword(reprtab, U"int", U"цел", kw_int);
	repr_add_keyword(reprtab, U"long", U"длин", kw_long);
	// repr_add_keyword(reprtab, U"short", U"корот", kw_short);
	repr_add_keyword(reprtab, U"struct", U"структура", kw_struct);
	// repr_add_keyword(reprtab, U"union", U"объединение", kw_union);
	repr_add_keyword(reprtab, U"void", U"пусто", kw_void);
	// repr_add_keyword(reprtab, U"signed", U"знаковый", kw_signed);
	// repr_add_keyword(reprtab, U"unsigned", U"беззнаковый", kw_unsigned);
	repr_add_keyword(reprtab, U"if", U"если", kw_if);
	repr_add_keyword(reprtab, U"else", U"иначе", kw_else);
	repr_add_keyword(reprtab, U"do", U"цикл", kw_do);
	repr_add_keyword(reprtab, U"while", U"пока", kw_while);
	repr_add_keyword(reprtab, U"for", U"для", kw_for);
	repr_add_keyword(reprtab, U"switch", U"выбор", kw_switch);
	repr_add_keyword(reprtab, U"case", U"случай", kw_case);
	repr_add_keyword(reprtab, U"default", U"умолчание", kw_default);
	repr_add_keyword(reprtab, U"break", U"выход", kw_break);
	repr_add_keyword(reprtab, U"continue", U"продолжить", kw_continue);
	// repr_add_keyword(reprtab, U"const", U"конст", kw_const);
	repr_add_keyword(reprtab, U"goto", U"переход", kw_goto);
	// repr_add_keyword(reprtab, U"sizeof", U"размер", kw_sizeof);
	// repr_add_keyword(reprtab, U"typedef", U"опртипа", kw_typedef);
	repr_add_keyword(reprtab, U"return", U"возврат", kw_return);
	// repr_add_keyword(reprtab, U"extern", U"внешн", kw_extern);
	// repr_add_keyword(reprtab, U"inline", U"встр", kw_inline);
	// repr_add_keyword(reprtab, U"register", U"регистровый", kw_register);

	repr_add_keyword(reprtab, U"print", U"печать", kw_print);
	repr_add_keyword(reprtab, U"printf", U"печатьф", kw_printf);
	repr_add_keyword(reprtab, U"printid", U"печатьид", kw_printid);
	repr_add_keyword(reprtab, U"scanf", U"читатьф", kw_scanf);
	repr_add_keyword(reprtab, U"getid", U"читатьид", kw_getid);
	repr_add_keyword(reprtab, U"abs", U"абс", kw_abs);
	repr_add_keyword(reprtab, U"sqrt", U"квкор", kw_sqrt);
	repr_add_keyword(reprtab, U"exp", U"эксп", kw_exp);
	repr_add_keyword(reprtab, U"sin", U"син", kw_sin);
	repr_add_keyword(reprtab, U"cos", U"кос", kw_cos);
	repr_add_keyword(reprtab, U"log", U"лог", kw_log);
	repr_add_keyword(reprtab, U"log10", U"лог10", kw_log10);
	repr_add_keyword(reprtab, U"asin", U"асин", kw_asin);
	repr_add_keyword(reprtab, U"rand", U"случ", kw_rand);
	repr_add_keyword(reprtab, U"round", U"округл", kw_round);
	repr_add_keyword(reprtab, U"strcpy", U"копир_строку", kw_strcpy);
	repr_add_keyword(reprtab, U"strncpy", U"копир_н_симв", kw_strncpy);
	repr_add_keyword(reprtab, U"strcat", U"конкат_строки", kw_strcat);
	repr_add_keyword(reprtab, U"strncat", U"конкат_н_симв", kw_strncat);
	repr_add_keyword(reprtab, U"strcmp", U"сравн_строк", kw_strcmp);
	repr_add_keyword(reprtab, U"strncmp", U"сравн_н_симв", kw_strncmp);
	repr_add_keyword(reprtab, U"strstr", U"нач_подстрок", kw_strstr);
	repr_add_keyword(reprtab, U"strlen", U"длина", kw_strlen);

	repr_add_keyword(reprtab, U"t_create_direct", U"н_создать_непоср", kw_t_create_direct);
	repr_add_keyword(reprtab, U"t_exit_direct", U"н_конец_непоср", kw_t_exit_direct);

	repr_add_keyword(reprtab, U"t_msg_send", U"н_послать", kw_msg_send);
	repr_add_keyword(reprtab, U"t_msg_receive", U"н_получить", kw_msg_receive);
	repr_add_keyword(reprtab, U"t_join", U"н_присоед", kw_join);
	repr_add_keyword(reprtab, U"t_sleep", U"н_спать", kw_sleep);
	repr_add_keyword(reprtab, U"t_sem_create", U"н_создать_сем", kw_sem_create);
	repr_add_keyword(reprtab, U"t_sem_wait", U"н_вниз_сем", kw_sem_wait);
	repr_add_keyword(reprtab, U"t_sem_post", U"н_вверх_сем", kw_sem_post);
	repr_add_keyword(reprtab, U"t_create", U"н_создать", kw_create);
	repr_add_keyword(reprtab, U"t_init", U"н_начать", kw_init);
	repr_add_keyword(reprtab, U"t_destroy", U"н_закончить", kw_destroy);
	repr_add_keyword(reprtab, U"t_exit", U"н_конец", kw_exit);
	repr_add_keyword(reprtab, U"t_getnum", U"н_номер_нити", kw_getnum);

	repr_add_keyword(reprtab, U"assert", U"проверить", kw_assert);
	repr_add_keyword(reprtab, U"pixel", U"пиксель", kw_pixel);
	repr_add_keyword(reprtab, U"line", U"линия", kw_line);
	repr_add_keyword(reprtab, U"rectangle", U"прямоугольник", kw_rectangle);
	repr_add_keyword(reprtab, U"ellipse", U"эллипс", kw_ellipse);
	repr_add_keyword(reprtab, U"clear", U"очистить", kw_clear);
	repr_add_keyword(reprtab, U"draw_string", U"нарисовать_строку", kw_draw_string);
	repr_add_keyword(reprtab, U"draw_number", U"нарисовать_число", kw_draw_number);
	repr_add_keyword(reprtab, U"icon", U"иконка", kw_icon);
	repr_add_keyword(reprtab, U"upb", U"кол_во", kw_upb);
	repr_add_keyword(reprtab, U"setsignal", U"сигнал", kw_setsignal);
	repr_add_keyword(reprtab, U"setmotor", U"мотор", kw_setmotor);
	repr_add_keyword(reprtab, U"setvoltage", U"устнапряжение", kw_setvoltage);
	repr_add_keyword(reprtab, U"getdigsensor", U"цифрдатчик", kw_getdigsensor);
	repr_add_keyword(reprtab, U"getansensor", U"аналогдатчик", kw_getansensor);

	repr_add_keyword(reprtab, U"wifi_connect", U"wifi_подключить", kw_wifi_connect);
	repr_add_keyword(reprtab, U"blynk_authorization", U"blynk_авторизация", kw_blynk_authorization);
	repr_add_keyword(reprtab, U"blynk_send", U"blynk_послать", kw_blynk_send);
	repr_add_keyword(reprtab, U"blynk_receive", U"blynk_получить", kw_blynk_receive);
	repr_add_keyword(reprtab, U"blynk_notification", U"blynk_уведомление", kw_blynk_notification);
	repr_add_keyword(reprtab, U"blynk_property", U"blynk_свойство", kw_blynk_property);
	repr_add_keyword(reprtab, U"blynk_lcd", U"blynk_дисплей", kw_blynk_lcd);
	repr_add_keyword(reprtab, U"blynk_terminal", U"blynk_терминал", kw_blynk_terminal);
	repr_add_keyword(reprtab, U"send_int_to_robot", U"послать_цел_на_робот", kw_send_int);
	repr_add_keyword(reprtab, U"send_float_to_robot", U"послать_вещ_на_робот", kw_send_float);
	repr_add_keyword(reprtab, U"send_string_to_robot", U"послать_строку_на_робот", kw_send_string);
	repr_add_keyword(reprtab, U"receive_int_from_robot", U"получить_цел_от_робота", kw_receive_int);
	repr_add_keyword(reprtab, U"receive_float_from_robot", U"получить_вещ_от_робота", kw_receive_float);
	repr_add_keyword(reprtab, U"receive_string_from_robot", U"получить_строку_от_робота", kw_receive_string);
}


void mode_init(syntax *const sx)
{
	vector_increase(&sx->modes, 1);
	// занесение в modetab описателя struct {int numTh; int inf; }
	vector_add(&sx->modes, 0);
	vector_add(&sx->modes, mode_struct);
	vector_add(&sx->modes, 2);
	vector_add(&sx->modes, 4);
	vector_add(&sx->modes, mode_integer);
	vector_add(&sx->modes, (item_t)map_reserve(&sx->representations, "numTh"));
	vector_add(&sx->modes, mode_integer);
	vector_add(&sx->modes, (item_t)map_reserve(&sx->representations, "data"));

	// занесение в modetab описателя функции void t_msg_send(struct msg_info m)
	vector_add(&sx->modes, 1);
	vector_add(&sx->modes, mode_function);
	vector_add(&sx->modes, mode_void);
	vector_add(&sx->modes, 1);
	vector_add(&sx->modes, 2);

	// занесение в modetab описателя функции void* interpreter(void* n)
	vector_add(&sx->modes, 9);
	vector_add(&sx->modes, mode_function);
	vector_add(&sx->modes, mode_void_pointer);
	vector_add(&sx->modes, 1);
	vector_add(&sx->modes, mode_void_pointer);

	sx->start_mode = 14;
}

item_t get_static(syntax *const sx, const item_t type)
{
	const item_t old_displ = sx->displ;
	sx->displ += sx->lg * size_of(sx, type);

	if (sx->lg > 0)
	{
		sx->max_displ = sx->displ > sx->max_displ ? sx->displ : sx->max_displ;
	}
	else
	{
		sx->max_displg = -sx->displ;
	}

	return old_displ;
}

/**	Check if modes are equal */
int mode_is_equal(const syntax *const sx, const size_t first, const size_t second)
{
	if (vector_get(&sx->modes, first) != vector_get(&sx->modes, second))
	{
		return 0;
	}

	size_t length = 1;
	const item_t mode = vector_get(&sx->modes, first);

	// Определяем, сколько полей надо сравнивать для различных типов записей
	if (mode == mode_struct || mode == mode_function)
	{
		length = 2 + (size_t)vector_get(&sx->modes, first + 2);
	}

	for (size_t i = 1; i <= length; i++)
	{
		if (vector_get(&sx->modes, first + i) != vector_get(&sx->modes, second + i))
		{
			return 0;
		}
	}

	return 1;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


syntax sx_create()
{
	syntax sx;
	sx.procd = 1;

	sx.predef = vector_create(FUNCSIZE);
	sx.functions = vector_create(FUNCSIZE);
	vector_increase(&sx.functions, 2);

	sx.tree = vector_create(MAXTREESIZE);

	sx.identifiers = vector_create(MAXIDENTAB);
	vector_increase(&sx.identifiers, 2);
	sx.cur_id = 2;

	sx.representations = map_create(MAXREPRTAB);
	repr_init(&sx.representations);

	sx.modes = vector_create(MAXMODETAB);
	mode_init(&sx);

	sx.max_displg = 3;
	sx.ref_main = 0;

	sx.max_displ = 3;
	sx.displ = -3;
	sx.lg = -1;

	return sx;
}

int sx_is_correct(syntax *const sx)
{
	int is_correct = 1;
	if (sx->ref_main == 0)
	{
		system_error(no_main_in_program);
		is_correct = 0;
	}

	for (size_t i = 0; i < vector_size(&sx->predef); i++)
	{
		if (vector_get(&sx->predef, i))
		{
			system_error(predef_but_notdef, repr_get_name(sx, (size_t)vector_get(&sx->predef, i)));
			is_correct = 0;
		}
	}

	return is_correct;
}

int sx_clear(syntax *const sx)
{
	if (sx == NULL)
	{
		return -1;
	}

	vector_clear(&sx->predef);
	vector_clear(&sx->functions);

	vector_clear(&sx->tree);

	vector_clear(&sx->identifiers);
	vector_clear(&sx->modes);
	map_clear(&sx->representations);

	return 0;
}


int func_add(syntax *const sx, const item_t ref)
{
	return sx != NULL ? vector_add(&sx->functions, ref) != SIZE_MAX ? 0 : -1 : -1;
}

int func_set(syntax *const sx, const size_t index, const item_t ref)
{
	return sx != NULL ? vector_set(&sx->functions, index, ref) : -1;
}

item_t func_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->functions, index) : ITEM_MAX;
}

size_t func_reserve(syntax *const sx)
{
	if (sx == NULL)
	{
		return SIZE_MAX;
	}

	vector_increase(&sx->functions, 1);
	return vector_size(&sx->functions) - 1;
}


size_t ident_add(syntax *const sx, const size_t repr, const item_t type, const item_t mode, const int func_def)
{
	const size_t last_id = vector_size(&sx->identifiers);
	const item_t ref = repr_get_reference(sx, repr);
	vector_add(&sx->identifiers, ref == ITEM_MAX ? ITEM_MAX - 1 : ref);
	vector_increase(&sx->identifiers, 3);

	if (ref == 0) // это может быть только MAIN
	{
		if (sx->ref_main)
		{
			return SIZE_MAX;
		}
		sx->ref_main = last_id;
	}

	// Ссылка на описание с таким же представлением в предыдущем блоке
	const size_t prev = (size_t)ident_get_prev(sx, last_id);
	if (prev)
	{
		// prev == 0 только для main, эту ссылку портить нельзя
		// иначе это ссылка на текущее описание с этим представлением
		repr_set_reference(sx, repr, (item_t)last_id);
	}

	// Один и тот же идентификатор м.б. переменной и меткой
	if (type != 1 && ident_get_prev(sx, last_id) != ITEM_MAX - 1
		&& prev >= sx->cur_id && (func_def != 1 || ident_get_repr(sx, prev) > 0))
	{
		// Только определение функции может иметь 2 описания, то есть иметь предописание
		return SIZE_MAX - 1;
	}

	ident_set_repr(sx, last_id, (item_t)repr);
	ident_set_mode(sx, last_id, mode);

	if (type < 0)
	{
		// Так как < 0, это функция-параметр
		ident_set_displ(sx, last_id, -(sx->displ++));
		sx->max_displ = sx->displ;
	}
	else if (type == 0)
	{
		ident_set_displ(sx, last_id, get_static(sx, mode));
	}
	else if (type == 1)
	{
		// Это метка
		// В поле mode: 0, если первым встретился goto; когда встретим метку, поставим 1
		// В поле displ: при генерации кода, когда встретим метку, поставим pc
		ident_set_mode(sx, last_id, 0);
		ident_set_displ(sx, last_id, 0);
	}
	else if (type >= 1000)
	{
		// Это описание типа, а (type-1000) – это номер инициирующей процедуры
		ident_set_displ(sx, last_id, type);
	}
	else if (type > 1 && type < 1000)
	{
		// Это функция, и в поле displ находится её номер
		ident_set_displ(sx, last_id, type);

		if (func_def == 2)
		{
			// Это предописание функции
			ident_set_repr(sx, last_id, -ident_get_repr(sx, last_id));
			vector_add(&sx->predef, (item_t)repr);
		}
		else
		{
			// Это описание функции
			for (size_t i = 0; i < vector_size(&sx->predef); i++)
			{
				if ((size_t)vector_get(&sx->predef, i) == repr)
				{
					vector_set(&sx->predef, i, 0);
				}
			}
		}
	}
	return last_id;
}

item_t ident_get_prev(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->identifiers, index) : ITEM_MAX;
}

item_t ident_get_repr(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->identifiers, index + 1) : ITEM_MAX;
}

item_t ident_get_mode(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->identifiers, index + 2) : ITEM_MAX;
}

item_t ident_get_displ(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->identifiers, index + 3) : ITEM_MAX;
}

int ident_set_repr(syntax *const sx, const size_t index, const item_t repr)
{
	return sx != NULL ? vector_set(&sx->identifiers, index + 1, repr) : -1;
}

int ident_set_mode(syntax *const sx, const size_t index, const item_t mode)
{
	return sx != NULL ? vector_set(&sx->identifiers, index + 2, mode) : -1;
}

int ident_set_displ(syntax *const sx, const size_t index, const item_t displ)
{
	return sx != NULL ? vector_set(&sx->identifiers, index + 3, displ) : -1;
}


size_t size_of(const syntax *const sx, const item_t mode)
{
	return mode > 0 && mode_get(sx, (size_t)mode) == mode_struct
		? (size_t)mode_get(sx, (size_t)mode + 1)
		: mode == mode_float
			? 2
			: 1;
}

size_t mode_add(syntax *const sx, const item_t *const record, const size_t size)
{
	if (sx == NULL || record == NULL)
	{
		return SIZE_MAX;
	}

	vector_add(&sx->modes, (item_t)sx->start_mode);
	sx->start_mode = vector_size(&sx->modes) - 1;
	for (size_t i = 0; i < size; i++)
	{
		vector_add(&sx->modes, record[i]);
	}

	// Checking mode duplicates
	size_t old = (size_t)vector_get(&sx->modes, sx->start_mode);
	while (old)
	{
		if (mode_is_equal(sx, sx->start_mode + 1, old + 1))
		{
			vector_resize(&sx->modes, sx->start_mode + 1);
			sx->start_mode = (size_t)vector_get(&sx->modes, sx->start_mode);
			return old + 1;
		}
		else
		{
			old = (size_t)vector_get(&sx->modes, old);
		}
	}

	return sx->start_mode + 1;
}

item_t mode_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->modes, index) : ITEM_MAX;
}


size_t repr_reserve(syntax *const sx, const char32_t *const spelling)
{
	return map_reserve_by_utf8(&sx->representations, spelling);
}

const char *repr_get_name(const syntax *const sx, const size_t index)
{
	return map_to_string(&sx->representations, index);
}

item_t repr_get_reference(const syntax *const sx, const size_t index)
{
	return map_get_by_index(&sx->representations, index);
}

int repr_set_reference(syntax *const sx, const size_t index, const item_t ref)
{
	return map_set_by_index(&sx->representations, index, ref);
}


int scope_block_enter(syntax *const sx, item_t *const displ, item_t *const lg)
{
	if (sx == NULL || displ == NULL || lg == NULL)
	{
		return -1;
	}

	sx->cur_id = vector_size(&sx->identifiers);
	*displ = sx->displ;
	*lg = sx->lg;
	return 0;
}

int scope_block_exit(syntax *const sx, const item_t displ, const item_t lg)
{
	if (sx == NULL)
	{
		return -1;
	}

	for (size_t i = vector_size(&sx->identifiers) - 4; i >= sx->cur_id; i -= 4)
	{
		const item_t prev = ident_get_prev(sx, i);
		repr_set_reference(sx, (size_t)ident_get_repr(sx, i), prev == ITEM_MAX - 1 ? ITEM_MAX : prev);
	}

	sx->displ = displ;
	sx->lg = lg;
	return 0;
}

item_t scope_func_enter(syntax *const sx)
{
	if (sx == NULL)
	{
		return ITEM_MAX;
	}

	const item_t displ = sx->displ;
	sx->cur_id = vector_size(&sx->identifiers);
	sx->displ = 3;
	sx->max_displ = 3;
	sx->lg = 1;

	return displ;
}

item_t scope_func_exit(syntax *const sx, const item_t displ)
{
	if (sx == NULL)
	{
		return ITEM_MAX;
	}

	for (size_t i = vector_size(&sx->identifiers) - 4; i >= sx->cur_id; i -= 4)
	{
		const item_t prev = ident_get_prev(sx, i);
		repr_set_reference(sx, (size_t)ident_get_repr(sx, i), prev == ITEM_MAX - 1 ? ITEM_MAX : prev);
	}

	sx->cur_id = 2;	// Все функции описываются на одном уровне
	sx->lg = -1;
	sx->displ = displ;

	return sx->max_displ;
}
