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
	return  symbol == ' ' || symbol == '\t';
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
	return  symbol == '\r' || symbol == '\n';
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
		parser_add_char(prs, ' ');
		prs->position--;
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
 *	Считать символы до конца строковой константы
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ начала строки
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t position = prs->position;		// Позиция начала строковой константы

	parser_add_char(prs, ch);					// Вывод символа начала строковой константы

	char32_t prev = '\0';
	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)
		{
			parser_add_char(prs, cur);

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
		}

		prev = cur;
		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);

	if (prs->is_recovery_disabled)				// Добавление "\";" в конец незаконченной строковой константы
	{
		parser_add_char(prs, ch);
		parser_add_char(prs, ';');
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
		prs->position++;
		cur = uni_scan_char(prs->in);
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
	parser_skip_line(prs, '/');
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

	char32_t prev;
	char32_t cur = '*';
	while (cur != (char32_t)EOF)
	{
		prev = cur;
		cur = uni_scan_char(prs->in);
		prs->position++;

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
	return prs->position - position;
}

/**
 *	Пропустить разделители и комментарии
 *
 *	@param	prs			Структура парсера
 *	@param	last		Текущий символ
 *
 *	@return	Количество символов, заменяемых на ' '
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
					const size_t size = parser_skip_long_comment(prs, prs->line) - 1;
					position = prs->line == line ? position + size : size;
				}
				break;
				default:
					uni_unscan_char(prs->in, next);
					return position;
			}
		}

		prs->position++;
		position++;
		cur = uni_scan_char(prs->in);
		*last = cur;
	}

	return position;
}


/**
 *	Пропустить строку с текущего символа, выводит ошибку, если попался не разделитель или комментарий
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 */
static void parser_find_unexpected_lexeme(parser *const prs, char32_t cur)
{
	while (utf8_is_separator(cur) || cur == '/')
	{
		if (cur == '/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case '/':
					parser_skip_short_comment(prs);
					return;
				case '*':
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

	universal_io in = linker_add_header(prs->lk, mode == '\"'
													? linker_search_internal(prs->lk, path)
													: linker_search_external(prs->lk, path));
	if (!in_is_correct(&in))
	{
		parser_macro_error(prs, PARSER_INCLUDE_INCORRECT_FILENAME);
	}

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	int ret = parser_preprocess(&new_prs, &in);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	// Добавление комментария после прохода файла
	parser_add_char(prs, '\n');
	prs->position--;
	parser_comment_to_buffer(prs);

	return ret;
}

//static void parser_preprocess_code(parser *const prs, char32_t cur, const keyword_t mode);

/**
 *	Preprocess buffer
 *
 *	@param	prs			Parser structure
 *	@param	buffer		Code for preprocessing
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
/*static int parser_preprocess_buffer(parser *const prs, const char *const buffer)
{
	// Печать кода и комментария перед макроподстановкой
	parser_add_char(prs, '\n');
	prs->position--;
	parser_macro_comment(prs);
	parser_print(prs);
	parser_clear_code(prs);

	parser new_prs = parser_create(prs->lk, prs->stg, prs->out);

	universal_io in = io_create();
	in_set_buffer(&in, buffer);

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	new_prs.in = &in;

	//parser_preprocess_code(&new_prs, '\0', 0);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	// Печать комментария после макроподстановки
	parser_add_char(prs, '\n');
	prs->position--;
	parser_comment_to_buffer(prs);

	return 0;
}*/


/**
 *	Считать путь к файлу и выполняет его обработку
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Символ после директивы
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_include(parser *const prs)
{
	const size_t position = prs->position;
	prs->position += strlen(storage_last_read(prs->stg));

	char32_t cur = uni_scan_char(prs->in);
	prs->position++;

	// Пропуск разделителей и комментариев
	parser_skip_separators(prs, &cur);
	uni_unscan_char(prs->in, cur);

	// Обработка пути
	char path[MAX_ARG_SIZE] = "\0";
	const char32_t index = storage_search(prs->stg, prs->in, &cur);
	const char32_t ch = cur == '<' ? '>' : '\"';
	if (index == SIZE_MAX)
	{
		if (storage_last_read(prs->stg) != NULL)	// Неопределенный идентификатор
		{
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
			parser_skip_line(prs, cur);
			return prs->was_error ? -1 : 0;
		}

		// Запись пути в кавычках
		cur = uni_scan_char(prs->in);
		prs->position++;
		while (cur != ch && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			parser_add_char_to_buffer(path, cur);
			cur = uni_scan_char(prs->in);
			prs->position++;
		}

		if (cur != ch)
		{
			prs->position = position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
			parser_skip_line(prs, cur);
			return prs->was_error ? -1 : 0;
		}
	}
	else
	{
		// Запись пути, если он был определен через #define
		prs->position += strlen(storage_last_read(prs->stg));
		strcpy(path, storage_get_by_index(prs->stg, index));
		path[strlen(path)] = '\0';	// Удаление последней кавычки
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

	parser_comment(prs);

	char32_t cur = '\0';
	while (cur != (char32_t)EOF)
	{
		const size_t index = storage_search(prs->stg, prs->in, &cur);
		switch (index)
		{
			case KW_INCLUDE:
				uni_unscan_char(prs->in, cur);
				parser_include(prs);
				break;
		
			case KW_DEFINE:
			case KW_SET:
			case KW_UNDEF:

			case KW_MACRO:
			case KW_ENDM:

			case KW_IFDEF:
			case KW_IFNDEF:
			case KW_IF:
			case KW_ELIF:
			case KW_ELSE:
			case KW_ENDIF:

			case KW_EVAL:

			case KW_WHILE:
			case KW_ENDW:

			default:
				if (storage_last_read(prs->stg) != NULL)
				{
					/*if (index != SIZE_MAX)
					{
						// Макроподстановка
						//const size_t size = prs->position + strlen(storage_last_read(prs->stg));
						//parser_preprocess_buffer(prs, storage_get_by_index(prs->stg, index));
						//parser_add_spacers(prs, size);
					}
					else*/
					{
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

					parser_add_spacers(prs, size);
					if (utf8_is_letter(cur) || cur == '#')
					{
						uni_unscan_char(prs->in, cur);	// Лексему необходимо пропустить через storage_scan
					}
					else
					{
						parser_add_char(prs, cur);
					}
				}
		}
	}

	prs->in = old_in;
	return -1;
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
