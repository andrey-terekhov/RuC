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


const size_t AVERAGE_VALUE_SIZE = 256;
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
	return symbol == ' ' || symbol == '\t';
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
	return symbol == '\r' || symbol == '\n';
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
}

/**
 *	Сделать отступ в начале строки
 *
 *	@param	prs			Структура парсера
 *	@param	size		Размер отступа
 */
static inline void parser_add_spacers(parser *const prs, const size_t size)
{
	for (size_t i = 0; i < size; i ++)
	{
		parser_add_char(prs, ' ');
	}
}

/**
 *	Добавить комментарий
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_comment(parser *const prs)
{
	comment cmt = cmt_create(linker_current_path(prs->lk), prs->line);
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
	comment cmt = cmt_create(linker_current_path(prs->lk), prs->line);
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


static size_t parser_skip_separators(parser *const prs, char32_t *const cur);

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
	prs->position++;

	char32_t prev = '\0';
	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)
		{
			parser_add_char(prs, cur);
			prs->position++;

			if (prev != '\\')
			{
				return;							// Строка считана, выход из функции
			}

		}
		else if (utf8_is_line_breaker(cur))		// Ошибка из-за наличия переноса строки
		{
			if (cur == '\r')					// Обработка переноса строки
			{
				uni_scan_char(prs->in);
			}

			if (prev != '\\')
			{
				prs->position = position;
				parser_macro_error(prs, PARSER_STRING_NOT_ENDED);

				if (prs->is_recovery_disabled)		// Добавление '\"' в конец незаконченной строковой константы
				{
					parser_add_char(prs, ch);
					prs->position++;
				}
				
				parser_add_char(prs, '\n');
				parser_print(prs);
				parser_next_line(prs);
				
				return;
			}
			strings_remove(&prs->code);
		}
		else									// Независимо от корректности строки выводит ее в out
		{
			parser_add_char(prs, cur);
			prs->position++;
		}

		prev = cur;
		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);

	if (prs->is_recovery_disabled)				// Добавление "\";" в конец незаконченной строковой константы
	{
		parser_add_char(prs, ch);
		parser_add_char(prs, ';');
		prs->position += 2;
	}

	parser_print(prs);
	parser_next_line(prs);
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
		if (!utf8_is_separator(cur))
		{
			cur = uni_scan_char(prs->in);
			prs->position++;
		}

		parser_skip_separators(prs, &cur);
	}

	uni_unscan_char(prs->in, cur);	// Необходимо, чтобы следующий символ был переносом строки или концом файла
}

/**
 *	Пропустить символы до конца комментария ('\n', '\r' или EOF)
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_skip_short_comment(parser *const prs)
{
	char32_t cur = '/';
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);	// Необходимо, чтобы следующий символ был переносом строки или концом файла
}

/**
 *	Пропустить символы до конца длинного комментария
 *
 *	@param	prs			Структура парсера
 *	@param	line		Номер строки с началом комментария
 *
 *	@return	Количество символов, заменяемых на ' '
 */
static size_t parser_skip_long_comment(parser *const prs, const size_t line)
{
	const size_t line_position = prs->line_position;		// Позиция начала строки с началом комментария
	const size_t position = prs->position;					// Позиция начала комментария в строке
	const size_t comment_text_position = in_get_position(prs->in);	// Позиция после символа комментария

	prs->position += 2;

	char32_t prev;
	char32_t cur = '\0';
	while (cur != (char32_t)EOF)
	{
		prev = cur;
		cur = uni_scan_char(prs->in);

		switch (cur)
		{
			case '\r':
				uni_scan_char(prs->in);
			case '\n':
				parser_next_line(prs);
				break;

			case '/':
				if (prev == '*')
				{
					if (prs->line == line)
					{
						prs->position++;
						return prs->position - position; 
					}

					return prs->position;
				}
				break;
		}
		prs->position++;
	}

	prs->line_position = line_position;
	prs->line = line;
	prs->position = position;

	parser_macro_error(prs, PARSER_COMM_NOT_ENDED);

	// Пропуск начала комментария, если он был не закрыт
	prs->position += 2;
	in_set_position(prs->in, comment_text_position);
	return prs->position - position;
}

/**
 *	Пропустить разделители и комментарии
 *
 *	@param	prs			Структура парсера
 *	@param	last		Текущий символ
 *
 *	@return	Количество пропущенных символов с начала пропуска, либо с начала последней строки
 */
static size_t parser_skip_separators(parser *const prs, char32_t *const last)
{
	size_t position = 0;
	char32_t cur = *last;
	while (utf8_is_separator(cur) || cur == '/')
	{
		if (cur == '/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case '/':
					parser_skip_short_comment(prs);
					break;
				case '*':
				{
					const size_t line = prs->line;
					const size_t size = parser_skip_long_comment(prs, prs->line);
					position = prs->line == line ? position + size : size;
				}
				break;
				default:
					uni_unscan_char(prs->in, next);
					return position;
			}
		}

		if (utf8_is_separator(cur))
		{
			prs->position++;
			position++;
		}

		cur = uni_scan_char(prs->in);
		*last = cur;
	}

	return position;
}


/**
 *	Пропустить строку с текущего символа, выводит ошибку, если попался не разделитель или комментарий
 *
 *	@param	prs			Структура парсера
 */
static void parser_find_unexpected_lexeme(parser *const prs)
{
	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);

	if (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_macro_warning(prs, PARSER_UNEXPECTED_LEXEME);
	}

	parser_skip_line(prs, cur);
}

/**
 *	Записать идентификатор в буффер
 *
 *	@param	prs			Структура парсера
 *	@param	id			Буффер для записи идентификатора
 *
 *	@return	Количество считанных символов
 */
static size_t parser_find_id(parser *const prs, char32_t *const id)
{
	char32_t cur = uni_scan_char(prs->in);
	if (!utf8_is_letter(cur))
	{
		parser_macro_error(prs, PARSER_NEED_IDENT);
		parser_skip_line(prs, cur);
		return 0;
	}

	size_t i = 0;
	while (utf8_is_letter(cur) || utf8_is_digit(cur))
	{
		if (i == MAX_IDENT_SIZE - 1)
		{
			parser_macro_error(prs, PARSER_BIG_IDENT_NAME);
			parser_skip_line(prs, cur);
			return i;
		}

		id[i++] = cur;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);
	id[i] = U'\0';
	return i;
}

/**
 *	Записать идентификатор в буффер
 *
 *	@param	prs			Структура парсера
 *	@param	val			Буффер для записи идентификатора
 *	@param	mode		Режим работы функции
 *	@param	id			Индекс идентификатора для режима KW_SET
 *
 *	@return	Количество незакрытых #macro
 */
static int parser_find_value(parser *const prs, universal_io *const val
								, const keyword_t mode, const size_t id)
{
	const size_t line = prs->line;

	bool fst_separator_flag = true;
	int macro_flag = mode == KW_MACRO ? 1 : 0;	// Необходим для #macro, вложенных в #macro
	char32_t prev = '\0';	// Необходим для контроля начала строки и слияния двух строк ("\\\n")
	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		const size_t size = parser_skip_separators(prs, &cur);
		if (size != 0 || (size != 0 && prs->line != line))
		{
			if (prev == '\\')
			{
				uni_print_char(val, '\\');
			}

			prev = '\0';
			if (!fst_separator_flag)
			{
				uni_print_char(val, ' ');
			}
		}
		fst_separator_flag = false;

		if (utf8_is_line_breaker(cur))
		{
			if (mode != KW_MACRO && prev != U'\\')	// Выход для #define и #set
			{
				break;
			}
			else if (prev != U'\\')
			{
				uni_print_char(val, '\n');
			}

			if (cur == U'\r')
			{
				cur = uni_scan_char(prs->in);
			}
			parser_next_line(prs);

			prev = '\n';
		}
		else
		{
			if (mode != KW_MACRO && cur == '#')
			{
				parser_macro_error(prs, PARSER_UNEXPECTED_GRID);
				parser_skip_line(prs, cur);
				return macro_flag;
			}
			else if (utf8_is_letter(cur) || (mode == KW_MACRO && cur == '#'))
			{
				if (prev == '\\')
				{
					uni_print_char(val, '\\');
					prev = '\0';
				}

				uni_unscan_char(prs->in, cur);
				const size_t index = storage_search(prs->stg, prs->in, &cur);
				if (mode == KW_SET && index == id)
				{
					uni_printf(val, "%s", storage_get_by_index(prs->stg, index));
					prs->position += strlen(storage_last_read(prs->stg)) - 1;
				}
				else if (storage_last_read(prs->stg) != NULL)
				{
					macro_flag = index == KW_MACRO
									? macro_flag + 1
									: index == KW_ENDM
										? macro_flag - 1
										: macro_flag;
					if (index == KW_ENDM && macro_flag == 0)	// Выход для #macro
					{
						uni_unscan_char(prs->in, cur);
						prs->position += strlen(storage_last_read(prs->stg));
						parser_find_unexpected_lexeme(prs);
						break;
					}

					uni_printf(val, "%s", storage_last_read(prs->stg));
					prs->position += strlen(storage_last_read(prs->stg)) - 1;
				}

				uni_unscan_char(prs->in, cur);
			}
			else if (cur != '\\')	// '\\' будет добавлен при обработке следующего символа
			{
				uni_print_char(val, cur);
				prev = '\0';	// После печати первого символа на новой строке позиция должна увеличиться
			}
		}

		if (prev != '\n')	// Без этого при переносе строки позиция будет сдвинута
		{
			prs->position++;
		}

		prev = cur;
		cur = uni_scan_char(prs->in);
	}

	if (!(mode == KW_MACRO && macro_flag == 0)) uni_unscan_char(prs->in, cur);
	//uni_print_char(val, (char32_t)EOF);
	uni_print_char(val, '\0');

	return macro_flag;
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
	if (!in_is_buffer(prs->in))
	{
		parser_add_char(prs, '\n');
		parser_macro_comment(prs);
	}
	parser_print(prs);

	// Сохранение позиции в исходном файле
	const size_t old_path = linker_get_index(prs->lk);
	const size_t line_position = prs->line_position;
	const size_t line = prs->line;

	// Подготовка к парсингу буффера
	prs->line = FST_LINE_INDEX;
	prs->position = FST_CHARACTER_INDEX;
	prs->line_position = 0;
	parser_clear_code(prs);

	universal_io in = io_create();
	in_set_buffer(&in, buffer);

	int ret = parser_preprocess(prs, &in);

	in_clear(&in);

	// Возврат позиционирования в исходном файле
	linker_set_index(prs->lk, old_path);
	prs->line = line;
	prs->line_position = line_position;
	parser_clear_code(prs);

	// Добавление комментария после прохода файла
	if (!in_is_buffer(prs->in))
	{
		parser_add_char(prs, '\n');
		parser_comment_to_buffer(prs);
	}

	return ret;
}

/**
 *	Preprocess included file
 *
 *	@param	prs			Parser structure
 *	@param	path		File path
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_preprocess_file(parser *const prs, char *const path)
{
	parser_add_char(prs, '\n');	// Необходимо из-за отсутсвия проверки наличия лексем перед '#'
	parser_print(prs);

	// Сохранение позиции в исходном файле
	const size_t old_path = linker_get_index(prs->lk);
	const size_t line_position = prs->line_position;
	const size_t line = prs->line;

	// Подготовка к парсингу нового файла
	prs->line = FST_LINE_INDEX;
	prs->position = FST_CHARACTER_INDEX;
	prs->line_position = 0;
	parser_clear_code(prs);

	path[strlen(path) - 1] = '\0';	// Удаление последней кавычки
	universal_io in = linker_add_header(prs->lk, path[0] == '\"'
													? linker_search_internal(prs->lk, &path[1])
													: linker_search_external(prs->lk, &path[1]));
	if (!in_is_correct(&in))
	{
		parser_macro_error(prs, PARSER_INCLUDE_INCORRECT_FILENAME);
	}

	int ret = parser_preprocess(prs, &in);

	in_clear(&in);

	// Возврат позиционирования в исходном файле
	linker_set_index(prs->lk, old_path);
	prs->line = line;
	prs->line_position = line_position;
	parser_clear_code(prs);

	// Добавление комментария после прохода файла
	parser_add_char(prs, '\n');
	parser_comment_to_buffer(prs);

	return ret;
}


/**
 *	Считать путь к файлу и выполняет его обработку
 *
 *	@param	prs			Структура парсера
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static void parser_include(parser *const prs)
{
	const size_t position = prs->position;
	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	// Обработка пути
	char path[MAX_ARG_SIZE] = "\0";
	const size_t index = storage_search(prs->stg, prs->in, &cur);
	char32_t ch = cur == '<' ? '>' : cur == '\"' ? '\"' : '\0';
	if (index == SIZE_MAX)
	{
		if (storage_last_read(prs->stg) != NULL || ch == '\0')	// Неопределенный идентификатор
		{														// или не '<', или не '\"'
			prs->position = position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
			parser_skip_line(prs, cur);
			return;
		}

		// Запись пути в кавычках
		prs->position++;
		do
		{
			utf8_to_string(&path[strlen(path)], cur);
			cur = uni_scan_char(prs->in);
			prs->position++;
		} while (cur != ch && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF);

		if (cur != ch)
		{
			prs->position = position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
			parser_skip_line(prs, cur);
			return;
		}
		else
		{
			utf8_to_string(&path[strlen(path)], cur);
		}
	}
	else
	{
		// Запись пути, если он был определен через #define
		prs->position += strlen(storage_last_read(prs->stg));
		if (storage_get_by_index(prs->stg, index) != NULL)
		{
			strcpy(path, storage_get_by_index(prs->stg, index));
		}

		uni_unscan_char(prs->in, cur);
	}

	// Парсинг подключенного файла
	size_t temp = prs->position;
	prs->position = position;
	parser_preprocess_file(prs, path);

	// Пропуск символов за путем
	prs->position = temp;
	parser_find_unexpected_lexeme(prs);
}

/**
 *	Считать идентификатор, значение и добавить в хранилище
 *
 *	@param	prs			Структура парсера
 */
static void parser_define(parser *const prs)
{
	const size_t line = prs->line;
	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	// Получение идентификатора
	char32_t id[MAX_IDENT_SIZE];
	id[0] = U'\0';
	prs->position += parser_find_id(prs, id);

	storage_remove(prs->stg, id);

	cur = uni_scan_char(prs->in);
	if (cur == '(')
	{
		//parser_find_args(prs);
	}
	else if (!utf8_is_separator(cur) && cur != '/'
		&& !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_macro_error(prs, PARSER_NEED_SEPARATOR);
		parser_skip_line(prs, cur);
		return;
	}
	uni_unscan_char(prs->in, cur);

	// Получение значения
	universal_io val = io_create();
	out_set_buffer(&val, AVERAGE_LINE_SIZE);
	parser_find_value(prs, &val, KW_DEFINE, SIZE_MAX);

	storage_add(prs->stg, id, out_extract_buffer(&val));

	io_erase(&val);

	switch (prs->line - line)				// При печати специального комментария
	{										// используется 2 переноса строки.
		case 2:								// Для случая, когда пропуск меньше 4 строк,
			parser_add_char(prs, '\n');		// специальный комментарий не ставится.
		case 1:
			parser_add_char(prs, '\n');
		case 0:
			break;
		default:
			parser_add_char(prs, '\n');
			parser_comment_to_buffer(prs);
			break;
	}
}

/**
 *	Считать идентификатор, значение и добавить в хранилище
 *
 *	@param	prs			Структура парсера
 */
static void parser_set(parser *const prs)
{
	const size_t line = prs->line;
	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	const size_t index = storage_search(prs->stg, prs->in, &cur);
	if (storage_last_read(prs->stg) == NULL)
	{
		parser_macro_error(prs, PARSER_NEED_IDENT);
		parser_skip_line(prs, cur);
		return;
	}
	else if (index == SIZE_MAX)
	{
		parser_macro_error(prs, PARSER_SET_NOT_EXIST_IDENT);
		parser_skip_line(prs, cur);
		return;
	}
	else if (storage_get_amount_by_index(prs->stg, index) != 0)
	{
		parser_macro_error(prs, PARSER_SET_WITH_ARGS);
		parser_skip_line(prs, cur);
		return;
	}
	else if (!utf8_is_separator(cur) && cur != '/'
		&& !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_macro_error(prs, PARSER_NEED_SEPARATOR);
		parser_skip_line(prs, cur);
		return;
	}

	prs->position += strlen(storage_last_read(prs->stg));
	uni_unscan_char(prs->in, cur);

	universal_io val = io_create();
	out_set_buffer(&val, AVERAGE_LINE_SIZE);
	parser_find_value(prs, &val, KW_SET, index);

	storage_set_by_index(prs->stg, index, out_extract_buffer(&val));

	io_erase(&val);

	switch (prs->line - line)				// При печати специального комментария
	{										// используется 2 переноса строки.
		case 2:								// Для случая, когда пропуск меньше 4 строк,
			parser_add_char(prs, '\n');		// специальный комментарий не ставится.
		case 1:
			parser_add_char(prs, '\n');
		case 0:
			break;
		default:
			parser_add_char(prs, '\n');
			parser_comment_to_buffer(prs);
			break;
	}
}

/**
 *	Считать идентификатор и удалить из хранилища
 *
 *	@param	prs			Структура парсера
 */
static void parser_undef(parser *const prs)
{
	const size_t line = prs->line;
	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	const size_t index = storage_search(prs->stg, prs->in, &cur);
	if (storage_last_read(prs->stg) == NULL)
	{
		parser_macro_error(prs, PARSER_NEED_IDENT);
		parser_skip_line(prs, cur);
		return;
	}
	else if (index == SIZE_MAX)
	{
		parser_macro_warning(prs, PARSER_UNDEF_NOT_EXIST_IDENT);
	}

	storage_remove_by_index(prs->stg, index);

	prs->position += strlen(storage_last_read(prs->stg));
	uni_unscan_char(prs->in, cur);
	parser_find_unexpected_lexeme(prs);

	switch (prs->line - line)				// При печати специального комментария
	{										// используется 2 переноса строки.
		case 2:								// Для случая, когда пропуск меньше 4 строк,
			parser_add_char(prs, '\n');		// специальный комментарий не ставится.
		case 1:
			parser_add_char(prs, '\n');
		case 0:
			break;
		default:
			parser_add_char(prs, '\n');
			parser_comment_to_buffer(prs);
			break;
	}
}

/**
 *	Считать идентификатор, значение и добавить в хранилище
 *
 *	@param	prs			Структура парсера
 */
static void parser_macro(parser *const prs)
{
	const size_t line = prs->line;
	const size_t position = prs->position;
	const size_t line_position = prs->line_position;
	const size_t separator_position = in_get_position(prs->in);

	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	// Получение идентификатора
	char32_t id[MAX_IDENT_SIZE];
	id[0] = U'\0';
	prs->position += parser_find_id(prs, id);
	parser_find_unexpected_lexeme(prs);

	// Получение значения
	universal_io val = io_create();
	out_set_buffer(&val, AVERAGE_LINE_SIZE);
	const int ret = parser_find_value(prs, &val, KW_MACRO, SIZE_MAX);

	if (ret == 0)
	{
		storage_remove(prs->stg, id);
		storage_add(prs->stg, id, out_extract_buffer(&val));
	}
	else
	{
		prs->line = line;
		prs->position = position;
		prs->line_position = line_position;
		parser_macro_error(prs, PARSER_MACRO_NOT_ENDED);

		in_set_position(prs->in, separator_position);
		uni_scan_char(prs->in);
		parser_skip_line(prs, cur);
	}

	io_erase(&val);

	switch (prs->line - line)				// При печати специального комментария
	{										// используется 2 переноса строки.
		case 2:								// Для случая, когда пропуск меньше 4 строк,
			parser_add_char(prs, '\n');		// специальный комментарий не ставится.
		case 1:
			parser_add_char(prs, '\n');
		case 0:
			break;
		default:
			parser_add_char(prs, '\n');
			parser_comment_to_buffer(prs);
			break;
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
	if (!parser_is_correct(prs) || !in_is_correct(in))
	{
		return -1;
	}

	universal_io *old_in = prs->in;
	prs->in = in;

	if (in_is_file(prs->in))
	{
		parser_comment(prs);
	}

	char32_t cur = '\n';
	while (cur != (char32_t)EOF && cur != '\0')
	{
		const size_t index = storage_search(prs->stg, prs->in, &cur);
		switch (index)
		{
			case KW_INCLUDE:
				uni_unscan_char(prs->in, cur);
				parser_include(prs);
				break;
		
			case KW_DEFINE:
				uni_unscan_char(prs->in, cur);
				parser_define(prs);
				break;
			case KW_SET:
				uni_unscan_char(prs->in, cur);
				parser_set(prs);
				break;
			case KW_UNDEF:
				uni_unscan_char(prs->in, cur);
				parser_undef(prs);
				break;

			case KW_MACRO:
				uni_unscan_char(prs->in, cur);
				parser_macro(prs);
				break;
			case KW_ENDM:
				parser_macro_error(prs, PARSER_UNEXPECTED_ENDM);
				parser_skip_line(prs, cur);
				break;

			case KW_IFDEF:
			case KW_IFNDEF:
			case KW_IF:
			case KW_ELIF:
			case KW_ELSE:
			case KW_ENDIF:

			case KW_EVAL:

			case KW_WHILE:
			case KW_ENDW:
				parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
				parser_skip_line(prs, cur);
				break;

			default:
				if (storage_last_read(prs->stg) != NULL)
				{
					if (index != SIZE_MAX)
					{
						// Макроподстановка
						const size_t line = prs->line;
						size_t size = prs->position + strlen(storage_last_read(prs->stg));

						if (cur == '(')
						{
							const size_t args_size = 0;//parser_find_args(prs);
							size = prs->line == line ? size + args_size : args_size;
						}
						else if (storage_get_amount_by_index(prs->stg, index) != 0)
						{
							parser_macro_error(prs, PARSER_IDENT_NEED_ARGS);
						}

						parser_preprocess_buffer(prs, storage_get_by_index(prs->stg, index));
						parser_add_spacers(prs, size);
						uni_unscan_char(prs->in, cur);
					}
					else
					{
						if (storage_last_read(prs->stg)[0] == '#')
						{
							parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
							prs->position += strlen(storage_last_read(prs->stg));
						}

						prs->position += parser_add_string(prs, storage_last_read(prs->stg));
						uni_unscan_char(prs->in, cur);	// Символ обработается в следующем шаге цикла
					}
				}
				else
				{
					const size_t line = prs->line;
					const size_t size = parser_skip_separators(prs, &cur);

					switch (prs->line - line)				// При печати специального комментария
					{										// используется 2 переноса строки.
						case 2:								// Для случая, когда пропуск меньше 4 строк,
							parser_add_char(prs, '\n');		// специальный комментарий не ставится.
						case 1:
							parser_add_char(prs, '\n');
						case 0:
							break;
						default:
							parser_add_char(prs, '\n');
							parser_comment_to_buffer(prs);
							break;
					}

					if (cur == '\'' || cur == '\"')
					{
						parser_add_spacers(prs, size);
						parser_skip_string(prs, cur);
						break;
					}

					if (utf8_is_line_breaker(cur))
					{
						if (cur == '\r')
						{
							uni_scan_char(prs->in);
						}

						parser_add_char(prs, '\n');
						parser_print(prs);
						parser_next_line(prs);
						parser_clear_code(prs);

						break;
					}

					if (cur == (char32_t)EOF || cur == '\0')
					{
						break;
					}

					parser_add_spacers(prs, size);
					if (utf8_is_letter(cur) || cur == '#')
					{
						uni_unscan_char(prs->in, cur);	// Лексему необходимо пропустить через storage_scan
					}
					else
					{
						parser_add_char(prs, cur);
						prs->position++;
					}
				}
		}
	}

	parser_print(prs);

	prs->in = old_in;
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
