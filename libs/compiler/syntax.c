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
	repr_add_keyword(reprtab, U"main", U"главная", TOK_MAIN);
	repr_add_keyword(reprtab, U"char", U"литера", TOK_CHAR);
	repr_add_keyword(reprtab, U"double", U"двойной", TOK_DOUBLE);
	repr_add_keyword(reprtab, U"float", U"вещ", TOK_FLOAT);
	repr_add_keyword(reprtab, U"int", U"цел", TOK_INT);
	repr_add_keyword(reprtab, U"long", U"длин", TOK_LONG);
	repr_add_keyword(reprtab, U"struct", U"структура", TOK_STRUCT);
	repr_add_keyword(reprtab, U"void", U"пусто", TOK_VOID);
	repr_add_keyword(reprtab, U"if", U"если", TOK_IF);
	repr_add_keyword(reprtab, U"else", U"иначе", TOK_ELSE);
	repr_add_keyword(reprtab, U"do", U"цикл", TOK_DO);
	repr_add_keyword(reprtab, U"while", U"пока", TOK_WHILE);
	repr_add_keyword(reprtab, U"for", U"для", TOK_FOR);
	repr_add_keyword(reprtab, U"switch", U"выбор", TOK_SWITCH);
	repr_add_keyword(reprtab, U"case", U"случай", TOK_CASE);
	repr_add_keyword(reprtab, U"default", U"умолчание", TOK_DEFAULT);
	repr_add_keyword(reprtab, U"break", U"выход", TOK_BREAK);
	repr_add_keyword(reprtab, U"continue", U"продолжить", TOK_CONTINUE);
	repr_add_keyword(reprtab, U"goto", U"переход", TOK_GOTO);
	repr_add_keyword(reprtab, U"return", U"возврат", TOK_RETURN);

	repr_add_keyword(reprtab, U"print", U"печать", TOK_PRINT);
	repr_add_keyword(reprtab, U"printf", U"печатьф", TOK_PRINTF);
	repr_add_keyword(reprtab, U"printid", U"печатьид", TOK_PRINTID);
	repr_add_keyword(reprtab, U"scanf", U"читатьф", TOK_SCANF);
	repr_add_keyword(reprtab, U"getid", U"читатьид", TOK_GETID);
	repr_add_keyword(reprtab, U"abs", U"абс", TOK_ABS);
	repr_add_keyword(reprtab, U"sqrt", U"квкор", TOK_SQRT);
	repr_add_keyword(reprtab, U"exp", U"эксп", TOK_EXP);
	repr_add_keyword(reprtab, U"sin", U"син", TOK_SIN);
	repr_add_keyword(reprtab, U"cos", U"кос", TOK_COS);
	repr_add_keyword(reprtab, U"log", U"лог", TOK_LOG);
	repr_add_keyword(reprtab, U"log10", U"лог10", TOK_LOG10);
	repr_add_keyword(reprtab, U"asin", U"асин", TOK_ASIN);
	repr_add_keyword(reprtab, U"rand", U"случ", TOK_RAND);
	repr_add_keyword(reprtab, U"round", U"округл", TOK_ROUND);
	repr_add_keyword(reprtab, U"strcpy", U"копир_строку", TOK_STRCPY);
	repr_add_keyword(reprtab, U"strncpy", U"копир_н_симв", TOK_STRNCPY);
	repr_add_keyword(reprtab, U"strcat", U"конкат_строки", TOK_STRCAT);
	repr_add_keyword(reprtab, U"strncat", U"конкат_н_симв", TOK_STRNCAT);
	repr_add_keyword(reprtab, U"strcmp", U"сравн_строк", TOK_STRCMP);
	repr_add_keyword(reprtab, U"strncmp", U"сравн_н_симв", TOK_STRNCMP);
	repr_add_keyword(reprtab, U"strstr", U"нач_подстрок", TOK_STRSTR);
	repr_add_keyword(reprtab, U"strlen", U"длина", TOK_STRLEN);

	repr_add_keyword(reprtab, U"t_create_direct", U"н_создать_непоср", TOK_CREATEDIRECT);
	repr_add_keyword(reprtab, U"t_exit_direct", U"н_конец_непоср", TOK_EXITDIRECT);

	repr_add_keyword(reprtab, U"t_msg_send", U"н_послать", TOK_MSG_SEND);
	repr_add_keyword(reprtab, U"t_msg_receive", U"н_получить", TOK_MSG_RECEIVE);
	repr_add_keyword(reprtab, U"t_join", U"н_присоед", TOK_JOIN);
	repr_add_keyword(reprtab, U"t_sleep", U"н_спать", TOK_SLEEP);
	repr_add_keyword(reprtab, U"t_sem_create", U"н_создать_сем", TOK_SEMCREATE);
	repr_add_keyword(reprtab, U"t_sem_wait", U"н_вниз_сем", TOK_SEMWAIT);
	repr_add_keyword(reprtab, U"t_sem_post", U"н_вверх_сем", TOK_SEMPOST);
	repr_add_keyword(reprtab, U"t_create", U"н_создать", TOK_CREATE);
	repr_add_keyword(reprtab, U"t_init", U"н_начать", TOK_INIT);
	repr_add_keyword(reprtab, U"t_destroy", U"н_закончить", TOK_DESTROY);
	repr_add_keyword(reprtab, U"t_exit", U"н_конец", TOK_EXIT);
	repr_add_keyword(reprtab, U"t_getnum", U"н_номер_нити", TOK_GETNUM);

	repr_add_keyword(reprtab, U"assert", U"проверить", TOK_ASSERT);
	repr_add_keyword(reprtab, U"upb", U"кол_во", TOK_UPB);

	repr_add_keyword(reprtab, U"send_int_to_robot", U"послать_цел_на_робот", TOK_SEND_INT);
	repr_add_keyword(reprtab, U"send_float_to_robot", U"послать_вещ_на_робот", TOK_SEND_FLOAT);
	repr_add_keyword(reprtab, U"send_string_to_robot", U"послать_строку_на_робот", TOK_SEND_STRING);
	repr_add_keyword(reprtab, U"receive_int_from_robot", U"получить_цел_от_робота", TOK_RECEIVE_INT);
	repr_add_keyword(reprtab, U"receive_float_from_robot", U"получить_вещ_от_робота", TOK_RECEIVE_FLOAT);
	repr_add_keyword(reprtab, U"receive_string_from_robot", U"получить_строку_от_робота", TOK_RECEIVE_STRING);
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
