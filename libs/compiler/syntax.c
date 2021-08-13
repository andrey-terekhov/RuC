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
#include "tokens.h"
#include "tree.h"


static const size_t REPRESENTATIONS_SIZE = 10000;
static const size_t IDENTIFIERS_SIZE = 10000;
static const size_t FUNCTIONS_SIZE = 100;
static const size_t TYPES_SIZE = 1000;
static const size_t TREE_SIZE = 10000;


static void repr_add_keyword(map *const reprtab, const char32_t *const eng, const char32_t *const rus, const token_t token)
{
	char32_t buffer[MAX_STRING_LENGTH];

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

static inline void repr_init(map *const reprtab)
{
	repr_add_keyword(reprtab, U"main", U"главная", TK_MAIN);
	repr_add_keyword(reprtab, U"char", U"литера", TK_CHAR);
	repr_add_keyword(reprtab, U"double", U"двойной", TK_DOUBLE);
	repr_add_keyword(reprtab, U"float", U"вещ", TK_FLOAT);
	repr_add_keyword(reprtab, U"int", U"цел", TK_INT);
	repr_add_keyword(reprtab, U"long", U"длин", TK_LONG);
	repr_add_keyword(reprtab, U"struct", U"структура", TK_STRUCT);
	repr_add_keyword(reprtab, U"void", U"пусто", TK_VOID);
	repr_add_keyword(reprtab, U"file", U"файл", TK_FILE);
	repr_add_keyword(reprtab, U"typedef", U"типопр", TK_TYPEDEF);
	repr_add_keyword(reprtab, U"if", U"если", TK_IF);
	repr_add_keyword(reprtab, U"else", U"иначе", TK_ELSE);
	repr_add_keyword(reprtab, U"do", U"цикл", TK_DO);
	repr_add_keyword(reprtab, U"while", U"пока", TK_WHILE);
	repr_add_keyword(reprtab, U"for", U"для", TK_FOR);
	repr_add_keyword(reprtab, U"switch", U"выбор", TK_SWITCH);
	repr_add_keyword(reprtab, U"case", U"случай", TK_CASE);
	repr_add_keyword(reprtab, U"default", U"умолчание", TK_DEFAULT);
	repr_add_keyword(reprtab, U"break", U"выход", TK_BREAK);
	repr_add_keyword(reprtab, U"continue", U"продолжить", TK_CONTINUE);
	repr_add_keyword(reprtab, U"goto", U"переход", TK_GOTO);
	repr_add_keyword(reprtab, U"return", U"возврат", TK_RETURN);

	repr_add_keyword(reprtab, U"print", U"печать", TK_PRINT);
	repr_add_keyword(reprtab, U"printf", U"печатьф", TK_PRINTF);
	repr_add_keyword(reprtab, U"printid", U"печатьид", TK_PRINTID);
	repr_add_keyword(reprtab, U"scanf", U"читатьф", TK_SCANF);
	repr_add_keyword(reprtab, U"getid", U"читатьид", TK_GETID);
	repr_add_keyword(reprtab, U"abs", U"абс", TK_ABS);
	repr_add_keyword(reprtab, U"upb", U"кол_во", TK_UPB);
	repr_add_keyword(reprtab, U"fread", U"фчитать", TK_FREAD);

	repr_add_keyword(reprtab, U"t_create_direct", U"н_создать_непоср", TK_CREATE_DIRECT);
	repr_add_keyword(reprtab, U"t_exit_direct", U"н_конец_непоср", TK_EXIT_DIRECT);
}


static inline void type_init(syntax *const sx)
{
	vector_increase(&sx->types, 1);
	// занесение в types описателя struct {int numTh; int inf; }
	sx->start_type = vector_add(&sx->types, 0);
	vector_add(&sx->types, TYPE_STRUCTURE);
	vector_add(&sx->types, 2);
	vector_add(&sx->types, 4);
	vector_add(&sx->types, TYPE_INTEGER);
	vector_add(&sx->types, (item_t)map_reserve(&sx->representations, "numTh"));
	vector_add(&sx->types, TYPE_INTEGER);
	vector_add(&sx->types, (item_t)map_reserve(&sx->representations, "data"));

	// занесение в types описателя функции void* interpreter(void* n)
	sx->start_type = vector_add(&sx->types, sx->start_type);
	vector_add(&sx->types, TYPE_FUNCTION);
	vector_add(&sx->types, TYPE_VOID_POINTER);
	vector_add(&sx->types, 1);
	vector_add(&sx->types, TYPE_VOID_POINTER);
}

static inline item_t get_static(syntax *const sx, const item_t type)
{
	const item_t old_displ = sx->displ;
	sx->displ += sx->lg * type_size(sx, type);

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

/**	Check if types are equal */
static inline bool type_is_equal(const syntax *const sx, const size_t first, const size_t second)
{
	if (vector_get(&sx->types, first) != vector_get(&sx->types, second))
	{
		return false;
	}

	size_t length = 1;
	const item_t type = vector_get(&sx->types, first);

	// Определяем, сколько полей надо сравнивать для различных типов записей
	if (type == TYPE_STRUCTURE || type == TYPE_FUNCTION)
	{
		length = 2 + (size_t)vector_get(&sx->types, first + 2);
	}

	for (size_t i = 1; i <= length; i++)
	{
		if (vector_get(&sx->types, first + i) != vector_get(&sx->types, second + i))
		{
			return false;
		}
	}

	return true;
}

static void builtin_add(syntax *const sx, const char32_t *const eng, const char32_t *const rus, const item_t type)
{
	// Добавляем одно из написаний в таблицу representations
	const size_t repr = map_add_by_utf8(&sx->representations, eng, ITEM_MAX);

	// Добавляем идентификатор в identifiers
	const item_t id = (item_t)ident_add(sx, repr, 2, type, 1);

	// Добавляем остальные варианты написания, все будут ссылаться на id
	char32_t buffer[MAX_STRING_LENGTH];

	buffer[0] = utf8_to_upper(eng[0]);
	for (size_t i = 1; eng[i - 1] != '\0'; i++)
	{
		buffer[i] = utf8_to_upper(eng[i]);
	}
	map_add_by_utf8(&sx->representations, buffer, id);

	buffer[0] = utf8_to_upper(rus[0]);
	for (size_t i = 1; rus[i - 1] != '\0'; i++)
	{
		buffer[i] = utf8_to_upper(rus[i]);
	}
	map_add_by_utf8(&sx->representations, rus, id);
	map_add_by_utf8(&sx->representations, buffer, id);
}


static void ident_init(syntax *const sx)
{
	builtin_add(sx, U"assert", U"проверить", type_function(sx, TYPE_VOID, "is"));

	builtin_add(sx, U"asin", U"асин", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"cos", U"кос", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"sin", U"син", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"exp", U"эксп", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"log", U"лог", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"log10", U"лог10", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"sqrt", U"квкор", type_function(sx, TYPE_FLOATING, "f"));
	builtin_add(sx, U"rand", U"случ", type_function(sx, TYPE_FLOATING, ""));
	builtin_add(sx, U"round", U"округл", type_function(sx, TYPE_INTEGER, "f"));

	builtin_add(sx, U"strcpy", U"копир_строку", type_function(sx, TYPE_VOID, "Ss"));
	builtin_add(sx, U"strncpy", U"копир_н_симв", type_function(sx, TYPE_VOID, "Ssi"));
	builtin_add(sx, U"strcat", U"конкат_строки", type_function(sx, TYPE_VOID, "Ss"));
	builtin_add(sx, U"strncat", U"конкат_н_симв", type_function(sx, TYPE_VOID, "Ssi"));
	builtin_add(sx, U"strcmp", U"сравн_строк", type_function(sx, TYPE_INTEGER, "ss"));
	builtin_add(sx, U"strncmp", U"сравн_н_симв", type_function(sx, TYPE_INTEGER, "ssi"));
	builtin_add(sx, U"strstr", U"нач_подстрок", type_function(sx, TYPE_INTEGER, "ss"));
	builtin_add(sx, U"strlen", U"длина", type_function(sx, TYPE_INTEGER, "s"));

	builtin_add(sx, U"send_int_to_robot", U"послать_цел_на_робот", type_function(sx, TYPE_VOID, "iI"));
	builtin_add(sx, U"send_float_to_robot", U"послать_вещ_на_робот", type_function(sx, TYPE_VOID, "iF"));
	builtin_add(sx, U"send_string_to_robot", U"послать_строку_на_робот", type_function(sx, TYPE_VOID, "is"));
	builtin_add(sx, U"receive_int_from_robot", U"получить_цел_от_робота", type_function(sx, TYPE_INTEGER, "i"));
	builtin_add(sx, U"receive_float_from_robot", U"получить_вещ_от_робота", type_function(sx, TYPE_FLOATING, "i"));
	builtin_add(sx, U"receive_string_from_robot", U"получить_строку_от_робота", type_function(sx, TYPE_VOID, "i"));

	builtin_add(sx, U"t_create", U"н_создать", type_function(sx, TYPE_INTEGER, "V"));
	builtin_add(sx, U"t_getnum", U"н_номер_нити", type_function(sx, TYPE_INTEGER, ""));
	builtin_add(sx, U"t_sleep", U"н_спать", type_function(sx, TYPE_VOID, "i"));
	builtin_add(sx, U"t_join", U"н_присоед", type_function(sx, TYPE_VOID, "i"));
	builtin_add(sx, U"t_exit", U"н_конец", type_function(sx, TYPE_VOID, ""));
	builtin_add(sx, U"t_init", U"н_начать", type_function(sx, TYPE_VOID, ""));
	builtin_add(sx, U"t_destroy", U"н_закончить", type_function(sx, TYPE_VOID, ""));

	builtin_add(sx, U"t_sem_create", U"н_создать_сем", type_function(sx, TYPE_INTEGER, "i"));
	builtin_add(sx, U"t_sem_wait", U"н_вниз_сем", type_function(sx, TYPE_VOID, "i"));
	builtin_add(sx, U"t_sem_post", U"н_вверх_сем", type_function(sx, TYPE_VOID, "i"));

	builtin_add(sx, U"t_msg_send", U"н_послать", type_function(sx, TYPE_VOID, "m"));
	builtin_add(sx, U"t_msg_receive", U"н_получить", type_function(sx, TYPE_MSG_INFO, ""));

	builtin_add(sx, U"fopen", U"фоткрыть", type_function(sx, type_pointer(sx, TYPE_FILE), "ss"));
	builtin_add(sx, U"fgetc", U"фчитать_символ", type_function(sx, TYPE_CHARACTER, "P"));
	builtin_add(sx, U"fputc", U"фписать_символ", type_function(sx, TYPE_CHARACTER, "iP"));
	builtin_add(sx, U"fclose", U"фзакрыть", type_function(sx, TYPE_INTEGER, "P"));
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

	sx.predef = vector_create(FUNCTIONS_SIZE);
	sx.functions = vector_create(FUNCTIONS_SIZE);
	vector_increase(&sx.functions, 2);

	sx.tree = vector_create(TREE_SIZE);

	sx.identifiers = vector_create(IDENTIFIERS_SIZE);
	vector_increase(&sx.identifiers, 2);
	sx.cur_id = 2;

	sx.representations = map_create(REPRESENTATIONS_SIZE);
	repr_init(&sx.representations);

	sx.types = vector_create(TYPES_SIZE);
	type_init(&sx);

	ident_init(&sx);

	sx.max_displg = 3;
	sx.ref_main = 0;

	sx.max_displ = 3;
	sx.displ = -3;
	sx.lg = -1;

	return sx;
}

bool sx_is_correct(syntax *const sx)
{
	bool is_correct = true;
	if (sx->ref_main == 0)
	{
		system_error(no_main_in_program);
		is_correct = false;
	}

	for (size_t i = 0; i < vector_size(&sx->predef); i++)
	{
		if (vector_get(&sx->predef, i))
		{
			system_error(predef_but_notdef, repr_get_name(sx, (size_t)vector_get(&sx->predef, i)));
			is_correct = false;
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
	vector_clear(&sx->types);
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


size_t ident_add(syntax *const sx, const size_t repr, const item_t kind, const item_t type, const int func_def)
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
	if (kind != 1 && ident_get_prev(sx, last_id) != ITEM_MAX - 1
		&& prev >= sx->cur_id && (func_def != 1 || ident_get_repr(sx, prev) > 0))
	{
		// Только определение функции может иметь 2 описания, то есть иметь предописание
		return SIZE_MAX - 1;
	}

	ident_set_repr(sx, last_id, (item_t)repr);
	ident_set_type(sx, last_id, type);

	if (kind < 0)
	{
		// Так как < 0, это функция-параметр
		ident_set_displ(sx, last_id, -(sx->displ++));
		sx->max_displ = sx->displ;
	}
	else if (kind == 0)
	{
		ident_set_displ(sx, last_id, get_static(sx, type));
	}
	else if (kind == 1)
	{
		// Это метка
		// В поле mode: 0, если первым встретился goto; когда встретим метку, поставим 1
		// В поле displ: при генерации кода, когда встретим метку, поставим pc
		ident_set_type(sx, last_id, 0);
		ident_set_displ(sx, last_id, 0);
	}
	else if (kind >= 1000)
	{
		// Это описание типа, а (type-1000) – это номер инициирующей процедуры
		ident_set_displ(sx, last_id, kind);
	}
	else if (kind > 1 && type < 1000)
	{
		// Это функция, и в поле displ находится её номер
		ident_set_displ(sx, last_id, kind);

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

item_t ident_get_type(const syntax *const sx, const size_t index)
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

int ident_set_type(syntax *const sx, const size_t index, const item_t type)
{
	return sx != NULL ? vector_set(&sx->identifiers, index + 2, type) : -1;
}

int ident_set_displ(syntax *const sx, const size_t index, const item_t displ)
{
	return sx != NULL ? vector_set(&sx->identifiers, index + 3, displ) : -1;
}


item_t type_add(syntax *const sx, const item_t *const record, const size_t size)
{
	if (sx == NULL || record == NULL)
	{
		return ITEM_MAX;
	}

	sx->start_type = vector_add(&sx->types, (item_t)sx->start_type);
	for (size_t i = 0; i < size; i++)
	{
		vector_add(&sx->types, record[i]);
	}

	// Checking mode duplicates
	size_t old = (size_t)vector_get(&sx->types, sx->start_type);
	while (old)
	{
		if (type_is_equal(sx, sx->start_type + 1, old + 1))
		{
			const size_t start_type = sx->start_type;
			sx->start_type = (size_t)vector_get(&sx->types, sx->start_type);
			vector_resize(&sx->types, start_type);
			return (item_t)old + 1;
		}
		else
		{
			old = (size_t)vector_get(&sx->types, old);
		}
	}

	return (item_t)sx->start_type + 1;
}

item_t type_get(const syntax *const sx, const size_t index)
{
	return sx != NULL ? vector_get(&sx->types, index) : ITEM_MAX;
}

size_t type_size(const syntax *const sx, const item_t type)
{
	if (type_is_structure(sx, type))
	{
		return (size_t)type_get(sx, (size_t)type + 1);
	}
	else if (type_is_floating(type))
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

bool type_is_integer(const item_t type)
{
	return type == TYPE_INTEGER || type == TYPE_CHARACTER;
}

bool type_is_floating(const item_t type)
{
	return type == TYPE_FLOATING;
}

bool type_is_arithmetic(const item_t type)
{
	return type_is_integer(type) || type_is_floating(type);
}

bool type_is_void(const item_t type)
{
	return type == TYPE_VOID;
}

bool type_is_array(const syntax *const sx, const item_t type)
{
	return type > 0 && type_get(sx, (size_t)type) == TYPE_ARRAY;
}

bool type_is_structure(const syntax *const sx, const item_t type)
{
	return type > 0 && type_get(sx, (size_t)type) == TYPE_STRUCTURE;
}

bool type_is_function(const syntax *const sx, const item_t type)
{
	return type > 0 && type_get(sx, (size_t)type) == TYPE_FUNCTION;
}

bool type_is_pointer(const syntax *const sx, const item_t type)
{
	return type > 0 && type_get(sx, (size_t)type) == TYPE_POINTER;
}

bool type_is_scalar(const syntax *const sx, const item_t type)
{
	return type_is_arithmetic(type) || type_is_pointer(sx, type);
}

bool type_is_aggregate(const syntax *const sx, const item_t type)
{
	return type_is_array(sx, type) || type_is_structure(sx, type);
}

bool type_is_string(const syntax *const sx, const item_t type)
{
	return type_is_array(sx, type) && type_is_integer(type_get(sx, (size_t)type + 1));
}

bool type_is_struct_pointer(const syntax *const sx, const item_t type)
{
	return type_is_pointer(sx, type) && type_is_structure(sx, type_get(sx, (size_t)type + 1));
}

bool type_is_file(const item_t type)
{
	return type == TYPE_FILE;
}

item_t type_array(syntax *const sx, const item_t type)
{
	return type_add(sx, (item_t[]){ TYPE_ARRAY, type }, 2);
}

/*
 *	args = list of characters each for one argument
 *		v -> void
 *		V -> void*
 *		s -> char[]
 *		S -> char[]*
 *		i -> int
 *		I -> int[]
 *		f -> float
 *		F -> float[]
 *		m -> msg_info
 *		P -> FILE*
 */
item_t type_function(syntax *const sx, const item_t return_type, const char *const args)
{
	item_t local_modetab[6];
	size_t i = 0;

	local_modetab[0] = TYPE_FUNCTION;
	local_modetab[1] = return_type;

	while (args[i] != '\0')
	{
		switch (args[i])
		{
			case 'v':
				local_modetab[3 + i] = TYPE_VOID;
				break;
			case 'V':
				local_modetab[3 + i] = TYPE_VOID_POINTER;
				break;
			case 's':
				local_modetab[3 + i] = type_array(sx, TYPE_CHARACTER);
				break;
			case 'S':
				local_modetab[3 + i] = type_pointer(sx, type_array(sx, TYPE_CHARACTER));
				break;
			case 'i':
				local_modetab[3 + i] = TYPE_INTEGER;
				break;
			case 'I':
				local_modetab[3 + i] = type_array(sx, TYPE_INTEGER);
				break;
			case 'f':
				local_modetab[3 + i] = TYPE_FLOATING;
				break;
			case 'F':
				local_modetab[3 + i] = type_array(sx, TYPE_FLOATING);
				break;
			case 'm':
				local_modetab[3 + i] = TYPE_MSG_INFO;
				break;
			case 'P':
				local_modetab[3 + i] = type_pointer(sx, TYPE_FILE);
				break;
		}

		i++;
	}

	local_modetab[2] = (item_t)i;

	return type_add(sx, local_modetab, i + 3);
}

item_t type_pointer(syntax *const sx, const item_t type)
{
	return type_add(sx, (item_t[]){ TYPE_POINTER, type }, 2);
}

bool type_is_undefined(const item_t type)
{
	return type == TYPE_UNDEFINED;
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
