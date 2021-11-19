/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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

#include "parser.h"
#include <string.h>
#include "error.h"
#include "commenter.h"
#include "keywords.h"
#include "uniprinter.h"
#include "uniscanner.h"


#define CMT_BUFFER_SIZE			MAX_ARG_SIZE + 128
#define AVERAGE_LINE_SIZE		256
#define MAX_IDENT_SIZE			4096
#define MAX_VALUE_SIZE			4096

const size_t FST_LINE_INDEX = 1;
const size_t FST_CHARACTER_INDEX = 0;


/**
 *	Checks if сharacter is separator
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
static bool utf8_is_separator(const char32_t symbol)
{
	return  symbol == U' ' || symbol == U'\t';
}

/**
 *	Checks if сharacter is line breaker
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
static bool utf8_is_line_breaker(const char32_t symbol)
{
	return  symbol == U'\r' || symbol == U'\n';
}


/**
 *	Увеличить значение текущей строки
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_next_line(parser *const prs)
{
	prs->line_position = in_get_position(prs->in);
	prs->line++;
	prs->position = FST_CHARACTER_INDEX;
}

/**
 *	Очистить буффер кода
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_clear_code(parser *const prs)
{
	strings_clear(&prs->code);
	prs->code = strings_create(AVERAGE_LINE_SIZE);
}

/**
 *	Печать в prs.out буффера кода
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_print(parser *const prs)
{
	const size_t size = strings_size(&prs->code);
	for (size_t i = 0; i < size; i++)
	{
		uni_printf(prs->out, "%s", strings_get(&prs->code, i));
	}
}

/**
 *	Добавить строку в буффер кода
 *
 *	@param	prs			Структура парсера
 *	@param	str			Строка
 *
 *	@return	Длина записанной строки
 */
static inline size_t parser_add_string(parser *const prs, const char *const str)
{
	if (str == NULL)
	{
		return 0;
	}

	strings_add(&prs->code, str);
	return strlen(str);
}

/**
 *	Добавить символ в буффер кода
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ
 */
static inline void parser_add_char(parser *const prs, const char32_t cur)
{
	char buffer[MAX_SYMBOL_SIZE];
	utf8_to_string(buffer, cur);

	strings_add(&prs->code, buffer);
	prs->position++;
}

/**
 *	Сдвинуть код разделителями
 *
 *	@param	prs			Структура парсера
 *	@param	size		Размер сдвига
 */
static inline void parser_add_spacers(parser *const prs, const size_t size)
{
	for (size_t i = 0; i < size; i ++)
	{
		parser_add_char(prs, U' ');
	}
}

/**
 *	Добавить комментарий
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_comment(parser *const prs)
{
	comment cmt = cmt_create(prs->path, prs->line);
	char buffer[CMT_BUFFER_SIZE];
	cmt_to_string(&cmt, buffer);
	uni_printf(prs->out, "%s", buffer);
}

/**
 *	Добавить комментарий в буффер кода
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_comment_to_buffer(parser *const prs)
{
	comment cmt = cmt_create(prs->path, prs->line);
	char buffer[CMT_BUFFER_SIZE];
	cmt_to_string(&cmt, buffer);
	parser_add_string(prs, buffer);
}

/**
 *	Добавить макро комментарий в буффер кода
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_macro_comment(parser *const prs)
{
	comment cmt = cmt_create_macro(linker_current_path(prs->lk), prs->line, prs->position);
	char buffer[CMT_BUFFER_SIZE];
	cmt_to_string(&cmt, buffer);
	parser_add_string(prs, buffer);
}

/**
 *	Сохранить считанный код
 *
 *	@param	prs			Структура парсера
 *	@param	str			Строка
 */
static inline size_t parser_add_to_buffer(char *const buffer, const char *const str)
{
	if (str == NULL)
	{
		return 0;
	}

	strcat(buffer, str);
	return strlen(str);
}

/**
 *	Сохранить считанный символ
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ
 */
static inline void parser_add_char_to_buffer(char *const buffer, const char32_t ch)
{
	utf8_to_string(&buffer[strlen(buffer)], ch);
}


/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_macro_error(parser *const prs, const error_t num)
{
	if (!prs->is_recovery_disabled)
	{
		size_t position = in_get_position(prs->in);
		in_set_position(prs->in, prs->line_position);

		char str[256];
		str[0] = '\0';

		char32_t cur = uni_scan_char(prs->in);
		while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			utf8_to_string(&str[strlen(str)], cur);
			cur = uni_scan_char(prs->in);
		}

		prs->was_error = true;
		macro_error(linker_current_path(prs->lk), str, prs->line, prs->position, num);
		in_set_position(prs->in, position);
	}
}

/**
 *	Emit an warning from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_macro_warning(parser *const prs, const warning_t num)
{
	size_t position = in_get_position(prs->in);
	in_set_position(prs->in, prs->line_position);

	char str[256];
	str[0] = '\0';

	char32_t cur = uni_scan_char(prs->in);
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		utf8_to_string(&str[strlen(str)], cur);
		cur = uni_scan_char(prs->in);
	}

	macro_warning(linker_current_path(prs->lk), str, prs->line, prs->position, num);
	in_set_position(prs->in, position);
}


/**
 *	Пропустить строку c текущего символа до '\r', '\n', EOF
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 */
static void parser_skip_line(parser *const prs, char32_t cur)
{
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);	// Необходимо, чтобы следующий символ был переносом строки или концом файла
}

/**
 *	Считать символы до конца строковой константы
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ начала строки
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t position = prs->position;		// Позиция начала строковой константы

	parser_add_char(prs, ch);					// Вывод символа начала строковой константы

	char32_t prev = U'\0';
	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)
		{
			parser_add_char(prs, cur);

			if (prev != U'\\')
			{
				return;							// Строка считана, выход из функции
			}

		}
		else if (utf8_is_line_breaker(cur))		// Ошибка из-за наличия переноса строки
		{
			if (cur == U'\r')					// Обработка переноса строки
			{
				uni_scan_char(prs->in);
			}

			if (prev != U'\\')
			{
				prs->position = position;
				parser_macro_error(prs, PARSER_STRING_NOT_ENDED);

				if (prs->is_recovery_disabled)		// Добавление '\"' в конец незаконченной строковой константы
				{
					parser_add_char(prs, ch);
				}
				
				parser_add_char(prs, U'\n');
				parser_print(prs);
				parser_next_line(prs);
				
				return;
			}
			strings_remove(&prs->code);
		}
		else									// Независимо от корректности строки выводит ее в out
		{
			parser_add_char(prs, cur);
		}

		prev = cur;
		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);

	if (prs->is_recovery_disabled)				// Добавление "\";" в конец незаконченной строковой константы
	{
		parser_add_char(prs, ch);
		parser_add_char(prs, U';');
	}

	parser_print(prs);
	parser_next_line(prs);
}

/**
 *	Пропустить символы до конца комментария ('\n', '\r' или EOF)
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_skip_short_comment(parser *const prs)
{
	parser_skip_line(prs, U'/');
}

/**
 *	Пропустить символы до конца длинного комментария
 *
 *	@param	prs			Структура парсера
 *	@param	line		Номер строки с началом комментария
 */
static size_t parser_skip_long_comment(parser *const prs, const size_t line)
{
	const size_t line_position = prs->line_position;		// Позиция начала строки с началом комментария
	const size_t position = prs->position - 1;				// Позиция начала комментария в строке
	const size_t comment_text_position = in_get_position(prs->in);	// Позиция после символа комментария

	prs->position++;										// '*' был считан снаружи

	char32_t prev;
	char32_t cur = U'*';
	while (cur != (char32_t)EOF)
	{
		prev = cur;
		cur = uni_scan_char(prs->in);
		prs->position++;

		switch (cur)
		{
			case U'\r':
				uni_scan_char(prs->in);
			case U'\n':
				parser_next_line(prs);
				break;

			case U'/':
				if (prev == U'*')
				{
					return prs->line == line
							? prs->position - position
							: prs->position;
				}
				break;
		}
	}

	prs->line_position = line_position;
	prs->line = line;
	prs->position = position;

	parser_macro_error(prs, PARSER_COMM_NOT_ENDED);

	// Пропуск начала комментария, если он был не закрыт
	prs->position += 2;
	in_set_position(prs->in, comment_text_position);
	return 2;
}


/**
 *	Проверить наличие лексем перед директивой препроцессора
 *
 *	@param	prs			Структура парсера
 *	@param	was_lexeme	Флаг, указывающий наличие лексемы
 *
 *	@return	@c 1 если позиция корректна, @c 0 если позиция некорректна 
 */
static inline bool parser_check_kw_position(parser *const prs, const bool was_lexeme)
{
	if (was_lexeme)
	{
		parser_macro_error(prs, PARSER_UNEXPECTED_GRID);
		return false;
	}

	return true;
}

/**
 *	Пропустить разделители и комментарии после директивы препроцессора
 *
 *	@param	prs			Структура парсера
 *	@param	cur		Текущий символ
 *	@param	mode		Условие корректного завершения
 *						@c 0 начало пути файла
 *						@c 1 буква
 *
 *	@return	@c 0,	если последний символ корректный,
 *			@c -1,	если встретился неожиданный символ,
 *			@c -2,	если встретился '\r', '\n', EOF
 */
static int parser_find_ident_begining(parser *const prs, char32_t cur, const int mode)
{
	while (utf8_is_separator(cur) || cur == U'/')
	{
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'*':
					parser_skip_long_comment(prs, prs->line);
					break;
				default:
					return -1;
			}
		}

		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);	// Дальнейшая обработка начнется с символа начала идентификатора

	if (utf8_is_line_breaker(cur) || cur == (char32_t)EOF)
	{
		return -2;
	}
	else if ((mode == 1 && !utf8_is_letter(cur)) || (mode == 0 && cur != U'\"' && cur != U'<'))
	{
		return -1;
	}

	return 0;
}

/**
 *	Пропустить строку с текущего символа, выводит ошибку, если попался не разделитель или комментарий
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 */
static void parser_find_unexpected_lexeme(parser *const prs, char32_t cur)
{
	while (utf8_is_separator(cur) || cur == U'/')
	{
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'/':
					parser_skip_short_comment(prs);
					return;
				case U'*':
					parser_skip_long_comment(prs, prs->line);
					break;
				default:
					parser_macro_warning(prs, PARSER_UNEXPECTED_LEXEME);
					parser_skip_line(prs, cur);
					return;
			}
		}

		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	if (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_macro_warning(prs, PARSER_UNEXPECTED_LEXEME);
	}

	parser_skip_line(prs, cur);
}

/**
 *	Preprocess included file
 *
 *	@param	prs			Parser structure
 *	@param	path		File path
 *	@param	mode		File search mode
 *						@c '\"' internal
 *						@c '>' external
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_preprocess_file(parser *const prs, const char *const path, const char32_t mode)
{
	parser new_prs = parser_create(prs->lk, prs->stg, prs->out);

	universal_io in = linker_add_header(prs->lk, mode == U'\"'
													? linker_search_internal(prs->lk, path)
													: linker_search_external(prs->lk, path));
	if (!in_is_correct(&in))
	{
		parser_macro_error(prs, PARSER_INCLUDE_INCORRECT_FILENAME);
	}

	new_prs.path = linker_current_path(new_prs.lk);

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	int ret = parser_preprocess(&new_prs, &in);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	parser_add_char(prs, U'\n');
	parser_comment_to_buffer(prs);

	return ret;
}

/**
 *	Preprocess buffer
 *
 *	@param	prs			Parser structure
 *	@param	buffer		Code for preprocessing
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_preprocess_buffer(parser *const prs, const char *const buffer)
{
	parser_print(prs);

	parser new_prs = parser_create(prs->lk, prs->stg, prs->out);

	universal_io in = io_create();
	in_set_buffer(&in, buffer);

	new_prs.path = prs->path;

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	int ret = parser_preprocess(&new_prs, &in);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	parser_add_char(prs, U'\n');
	parser_macro_comment(prs);

	return ret;
}


/**
 *	Считать путь к файлу и выполняет его обработку
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Символ после директивы
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_include(parser *const prs, char32_t cur)
{
	const size_t position = prs->position;
	prs->position += strlen(storage_last_read(prs->stg)) + 1;	// Учитывается разделитель после директивы

	// Пропуск разделителей и комментариев
	if (parser_find_ident_begining(prs, cur, 0))
	{
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		parser_skip_line(prs, cur);
		return prs->was_error ? -1 : 0;
	}
	cur = uni_scan_char(prs->in);	// Необходимо считать символ начала строковой константы

	// ОБработка пути
	char path[MAX_ARG_SIZE] = "\0";
	const char32_t ch = cur == U'<' ? U'>' : U'\"';
	storage_search(prs->stg, prs->in, &cur);
	prs->position += parser_add_to_buffer(path, storage_last_read(prs->stg));

	while (cur != ch && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_add_char_to_buffer(path, cur);
		prs->position++;

		storage_search(prs->stg, prs->in, &cur);
		prs->position += parser_add_to_buffer(path, storage_last_read(prs->stg));
	}

	prs->position++;
	if (cur != ch)
	{
		prs->position = position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		parser_skip_line(prs, cur);
		return prs->was_error ? -1 : 0;
	}

	// Парсинг подключенного файла
	size_t temp = prs->position;
	prs->position = position;
	int ret = parser_preprocess_file(prs, path, ch);

	// Пропуск символов за путем
	prs->position = temp;
	parser_find_unexpected_lexeme(prs, uni_scan_char(prs->in));

	return ret;
}

/**
 *	Считать имя идентификатора, его значение
 *
 *	@param	prs			Структура парсера
 *	@param	mode		Режим работы функции
 *						@c KW_DEFINE #define
 *						@c KW_SET	 #set
 *						@c KW_UNDEF	 #undef
 */
/*static void parser_define(parser *const prs, char32_t cur, const keyword_t mode)
{
	const size_t line = prs->line;
	prs->position += strlen(storage_last_read(prs->stg)) + 1;	// Учитывается разделитель после директивы

	// Пропуск разделителей и комментариев
	const int res = parser_find_ident_begining(prs, cur, 1);
	if (res == -1)
	{
		parser_macro_error(prs, PARSER_INCORRECT_IDENT_NAME);
	}
	else if (res == -2)
	{
		parser_macro_error(prs, mode == KW_DEFINE
								? PARSER_DEFINE_NEED_IDENT
								: mode == KW_SET
									? PARSER_SET_NEED_IDENT
									: PARSER_UNDEF_NEED_IDENT);
	}
	else
	{
		char32_t id[MAX_IDENT_SIZE];
		id[0] = U'\0';
		char32_t value[MAX_VALUE_SIZE];
		value[0] = U'\0';

		// Запись идентификатора
		const size_t position = parser_find_id(prs, id, cur);	// Позиция начала идентификатора в строке
		cur = uni_scan_char(prs->in);

		// Проверка существования
		const size_t temp = prs->position;
		const size_t index = storage_get_index(prs->stg, prs->in, id);
		if (mode == KW_DEFINE && index != SIZE_MAX)
		{
			prs->position = position;
			parser_macro_warning(prs, PARSER_DEFINE_EXIST_IDENT);
			prs->position = temp;
		}
		else if (mode == KW_SET && index == SIZE_MAX)
		{
			prs->position = position;
			//parser_macro_warning(prs, PARSER_SET_NOT_EXIST_IDENT);
			prs->position = temp;
		}
		else if (mode == KW_UNDEF)
		{
			if (index == SIZE_MAX)
			{
				prs->position = position;
				parser_macro_warning(prs, PARSER_UNDEF_NOT_EXIST_IDENT);
				prs->position = temp;
			}
			storage_remove_by_index(prs->stg, index);

			// Проверка последующего кода для #undef
			parser_find_unexpected_lexeme(prs, cur);
			return;
		}

		// Запись значения
		parser_find_value(prs, value, cur, mode);

		if (mode == KW_SET)
		{
			// Для случая #set A A + 1
		}

		storage_remove_by_index(prs->stg, index);	// Удаление предыдущего значения, если оно было
		storage_add(prs->stg, id, value);
	}

	if (prs->line != line + 1)	// Было увеличение строки
	{
		parser_comment(prs);
	}
}*/

/**
 *	Разобрать код
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 *	@param	mode		Режим работы функции
 */
static void parser_preprocess_code(parser *const prs, char32_t cur, const keyword_t mode)
{
	bool was_star = false;
	bool was_lexeme = false;
	while (cur != (char32_t)EOF)
	{
		const char32_t prev = cur;
		const size_t index = storage_search(prs->stg, prs->in, &cur);
		if (was_star && prev == U'/' && cur != U'/' && cur != U'*')
		{
			strings_remove(&prs->code);	// '/' был записан в буффер
			strings_remove(&prs->code);	// '*' был записан в буффер
			prs->position -= 2;
			parser_macro_warning(prs, PARSER_UNEXPECTED_COMM_END);

			prs->position += 0;				// Пропуск лишнего закрытия комментария
			parser_add_spacers(prs, 2);
		}

		switch (index)
		{
			case KW_INCLUDE:
				parser_check_kw_position(prs, was_lexeme);	// Проверка на наличие лексем перед директивой
				parser_include(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
		
			case KW_DEFINE:
			case KW_SET:
			case KW_UNDEF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_MACRO:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ENDM:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_error(prs, PARSER_UNEXPECTED_ENDM);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_IFDEF:
			case KW_IFNDEF:
			case KW_IF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ELIF:
			case KW_ELSE:
			case KW_ENDIF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_error(prs, PARSER_UNEXPECTED_ENDIF);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_EVAL:

			case KW_WHILE:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ENDW:
				parser_check_kw_position(prs, was_lexeme);
				if (mode != KW_WHILE)
				{
					parser_macro_error(prs, PARSER_UNEXPECTED_ENDW);
					parser_skip_line(prs, cur);
				}
				was_lexeme = false;
				was_star = false;
				return;

			default:
				if (!utf8_is_separator(cur) || storage_last_read(prs->stg) != NULL)	// Перед '#' могут быть только разделители
				{
					was_lexeme = true;
				}

				if (storage_last_read(prs->stg) != NULL)
				{
					was_star = false;

					if (storage_last_read(prs->stg)[0] == '#')
					{
						if (parser_check_kw_position(prs, was_lexeme))	// Перед '#' есть лексемы -> '#' не на месте
																		// Перед '#' нет лексем   -> неправильная директива
						{
							parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
						}
						parser_skip_line(prs, cur);
						break;
					}

					if (index != SIZE_MAX)
					{
						// Макроподстановка
						parser_preprocess_buffer(prs, storage_get_by_index(prs->stg, index));
					}
					else
					{
						prs->position += parser_add_string(prs, storage_last_read(prs->stg));
					}
				}

				switch (cur)
				{
					case U'#':
						was_star = false;
						parser_macro_error(prs, PARSER_UNEXPECTED_GRID);
						parser_skip_line(prs, cur);
						break;

					case U'\'':
					case U'\"':
						was_star = false;
						parser_skip_string(prs, cur);
						break;

					case U'\r':
							cur = uni_scan_char(prs->in);
						case U'\n':
							was_star = false;
							was_lexeme = false;

							parser_add_char(prs, U'\n');
							parser_print(prs);
							parser_next_line(prs);
							parser_clear_code(prs);
							break;
						case (char32_t)EOF:
							parser_print(prs);
							break;

					case U'/':
						if (prev == U'/')
						{
							was_star = false;
							strings_remove(&prs->code);	// '/' был записан в буффер
							parser_skip_short_comment(prs);
						}
						else
						{
							parser_add_char(prs, cur);
						}
						break;
					case U'*':
						if (prev == U'/')
						{
							was_star = false;
							strings_remove(&prs->code);	// '/' был записан в буффер

							const size_t line = prs->line;
							const size_t size = parser_skip_long_comment(prs, line);

							switch (prs->line - line)				// При печати специального комментария
							{										// используется 2 переноса строки.
								case 2:								// Для комментариев меньше четырехстрочных
									parser_add_char(prs, U'\n');	// специальный комментарий не ставится.
								case 1:
									parser_add_char(prs, U'\n');
								case 0:
									break;
								default:
									parser_add_char(prs, U'\n');
									parser_comment_to_buffer(prs);
									break;
							}

							parser_add_spacers(prs, size);
						}
						else
						{
							was_star = true;
							parser_add_char(prs, cur);
						}
						break;

					default:
						was_star = false;
						parser_add_char(prs, cur);
						break;
				}
		}
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


parser parser_create(linker *const lk, storage *const stg, universal_io *const out)
{
	parser prs; 
	if (!linker_is_correct(lk) || !out_is_correct(out) || !storage_is_correct(stg))
	{
		prs.lk = NULL;
		return prs;
	}

	prs.lk = lk;
	prs.stg = stg;
	
	prs.in = NULL;
	prs.out = out;

	prs.path = NULL;
	prs.line_position = 0;
	prs.line = FST_LINE_INDEX;
	prs.position = FST_CHARACTER_INDEX;
	prs.code = strings_create(AVERAGE_LINE_SIZE);

	prs.is_recovery_disabled = false;
	prs.was_error = false;

	return prs;
} 


int parser_preprocess(parser *const prs, universal_io *const in)
{
	if (!parser_is_correct(prs)|| !in_is_correct(in))
	{
		return -1;
	}

	prs->in = in;
	prs->path = linker_current_path(prs->lk);
	parser_comment(prs);
	parser_print(prs);
	
	strings_clear(&prs->code);
	prs->code = strings_create(AVERAGE_LINE_SIZE);

	char32_t cur = U'\0';
	parser_preprocess_code(prs, cur, 0);

	return !prs->was_error ? 0 : -1;
}


int parser_disable_recovery(parser *const prs)
{
	if (!parser_is_correct(prs))
	{
		return -1;
	}

	prs->is_recovery_disabled = true;
	return 0;
}


bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && storage_is_correct(prs->stg) && out_is_correct(prs->out);
}


int parser_clear(parser *const prs)
{
	return prs != NULL && linker_clear(prs->lk) && storage_clear(prs->stg) &&  strings_clear(&prs->code);
}
