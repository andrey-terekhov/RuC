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
#include "keywords.h"
#include "uniio.h"
#include "uniprinter.h"
#include "uniscanner.h"


const size_t FST_LINE_INDEX =		1;
const size_t FST_CHARACTER_INDEX =	0;


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
 *	Checks if сharacter is line_breaker
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
 *	Увеличивает значение line и сбрасывает значение position
 */
static inline void parser_next_string(parser *const prs)
{
	prs->line++;
	prs->position = FST_CHARACTER_INDEX;
	prs->string[0] = '\0';
}

/**
 *	Добавляет str в string
 */
static inline size_t parser_add_string(parser *const prs, const char *const str)
{
	if (str == NULL)
	{
		return 0;
	}

	strcat(prs->string, str);
	return strlen(str);
}

/**
 *	Добавляет символ в string и увеличивает значение position
 */
static inline void parser_add_char(parser *const prs, const char32_t cur)
{
	utf8_to_string(&prs->string[strlen(prs->string)], cur);
	prs->position++;
}

/**
 *	Заполняет string до конца строки
 */
static inline void parser_fill_string(parser *const prs)
{
	char32_t cur = uni_scan_char(prs->in);
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		utf8_to_string(&prs->string[strlen(prs->string)], cur);
		cur = uni_scan_char(prs->in);
	}
}

/**
 *	Сохраняет считанный код
 */
static inline void parser_add_to_buffer(char *const buffer, const char *const string)
{
	if (string == NULL)
	{
		return;
	}

	strcat(buffer, string);
}

/**
 *	Сохраняет считанный символ
 */
static inline void parser_add_char_to_buffer(const char32_t ch, char *const buffer)
{
	utf8_to_string(&buffer[strlen(buffer)], ch);
}


/**
 *	Ошибка парсера препроцессора
 */
static void parser_macro_error(parser *const prs, const error_t num, const bool need_skip)
{
	if (parser_is_correct(prs) && !prs->is_recovery_disabled && !prs->was_error)
	{
		if (need_skip)
		{
			parser_fill_string(prs);
		}
		prs->was_error = true;
		macro_error(linker_current_path(prs->lk), (char *)prs->string, prs->line, prs->position, num);
	}
}

/**
 *	Предупреждение парсера препроцессора
 */
static void parser_macro_warning(parser *const prs, const error_t num, const bool need_skip)
{
	if (parser_is_correct(prs))
	{
		if (need_skip)
		{
			parser_fill_string(prs);
		}
		macro_warning(linker_current_path(prs->lk), (char *)prs->string, prs->line, prs->position, num);
	}
}


/**
 *	Считывает символы до конца строковой константы и буфферизирует текущую строку кода
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t old_position = prs->position;

	parser_add_char(prs, ch);
	uni_print_char(prs->out, ch);

	bool was_slash = false;

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)							// Строка считана, выход из функции
		{
			if (was_slash)
			{
				was_slash = false;
				parser_add_char(prs, cur);
				uni_print_char(prs->out, cur);
			}
			else
			{
				parser_add_char(prs, cur);
				uni_print_char(prs->out, cur);
				return;
			}
		}
		else if (utf8_is_line_breaker(cur))	// Ошибка из-за наличия переноса строки
		{
			prs->position = old_position;
			parser_macro_error(prs, PARSER_STRING_NOT_ENDED, false);

			parser_next_string(prs);

			// Специфика последовательности "\r\n"
			if (cur == U'\r')
			{
				char32_t next = uni_scan_char(prs->in);
				if (next != U'\n')
				{
					uni_unscan_char(prs->in, next);
				}
			}
		}
		else									// Независимо от корректности строки выводит ее в out
		{
			was_slash = cur == U'\\' ? true : false;
			parser_add_char(prs, cur);
			uni_print_char(prs->out, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF, false);
}

/**
 *	Пропускает символы до конца комментария ('\n', '\r' или EOF)
 *
 *	@note Буфферизация строки кода не производится, поскольку, комментарий идет до конца строки
 */
static void parser_skip_short_comment(parser *const prs)
{
	parser_add_char(prs, U'/');
	parser_add_char(prs, U'/');

	char32_t cur = uni_scan_char(prs->in);
	while(!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_add_char(prs, cur);
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);
}

/**
 *	Считывает символы до конца длинного комментария и буфферизирует текущую строку кода
 */
static void parser_skip_long_comment(parser *const prs)
{
	// Сохранение позиции начала комментария на случай ошибки c возможностью буфферизации до конца строки
	const size_t old_position = prs->position;
	parser_add_char(prs, U'/');
	parser comm_beginning = *prs;
	parser_add_char(&comm_beginning, U'*');

	parser_add_char(prs, U'*');

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		switch (cur)
		{
			case U'\r':
				uni_scan_char(prs->in);
			case U'\n':
				parser_next_string(prs);
					break;

			case U'*':
			{
				parser_add_char(prs, cur);

				// Буфферизация комментария для вывода полной строки кода в случае незавершенного комментария
				if (prs->line == comm_beginning.line)
				{
					parser_add_char(&comm_beginning, cur);
				}

				char32_t next = uni_scan_char(prs->in);
				switch (next)
				{
					case U'/':							// Комментарий считан, выход из функции
						parser_add_char(prs, next);
							return;

					default:							// Если встретился один '*', добавляет его в буффер строки кода
														// и обрабатывает следующие символы
						uni_unscan_char(prs->in, next);	// Символ next не имеет отношения к комментарию,
														// он будет считан повторно и проанализирован снаружи
				}
			}
			break;

			default:
				parser_add_char(prs, cur);

				if (prs->line == comm_beginning.line)
				{
					parser_add_char(&comm_beginning, cur);
				}
		}

		cur = uni_scan_char(prs->in);
	}

	comm_beginning.position = old_position;
	parser_macro_error(&comm_beginning, PARSER_COMM_NOT_ENDED, false);
	prs->was_error = true;
	parser_clear(&comm_beginning);
}


/**
 *	Считывает путь к файлу и выполняет его обработку
 */
static void parser_include(parser *const prs)
{
	const size_t include_position = prs->position;

	char32_t cur = U'\0';
	storage_search(prs->stg, prs->in, &cur);
	
	// Пропуск разделителей и комментариев
	while (cur != U'\"' && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		if (storage_last_read(prs->stg) == NULL && utf8_is_separator(cur))
		{
			parser_add_char(prs, cur);
		}
		else if (cur == U'/' && uni_scan_char(prs->in) == U'*' && storage_last_read(prs->stg) == NULL)
		{
			parser_add_char(prs, cur);
			parser_skip_long_comment(prs);
		}
		else
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_add_char(prs, cur);
			prs->position = include_position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME, true);
		}
		storage_search(prs->stg, prs->in, &cur);
	}

	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_add_char(prs, cur);
		prs->position = include_position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME, true);
	}

	// ОБработка пути
	char buffer[1024] = "\0";
	if (cur == U'\"')
	{
		parser_add_char(prs, cur);
		storage_search(prs->stg, prs->in, &cur);
		parser_add_to_buffer(buffer, storage_last_read(prs->stg));

		while (cur != U'\"' && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			parser_add_char_to_buffer(cur, buffer);
			parser_add_char(prs, cur);

			storage_search(prs->stg, prs->in, &cur);
			parser_add_to_buffer(buffer, storage_last_read(prs->stg));

			prs->position += parser_add_string(prs, storage_last_read(prs->stg));
		}

		parser_add_char(prs, cur);
		if (cur != U'\"')
		{
			prs->position = include_position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME, true);
		}
	}
	else
	{
		prs->position = include_position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME, true);
	}

	// Обработка символов за путем
	storage_search(prs->stg, prs->in, &cur);
	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME, true);
	}

	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		if (storage_last_read(prs->stg) != NULL)
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_add_char(prs, cur);
			parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME, true);
		}
		if (utf8_is_separator(cur))
		{
			parser_add_char(prs, cur);
		}
		else if (cur == U'/' && uni_scan_char(prs->in) == U'/')
		{
			break;
		}
		else if (cur == U'/' && uni_scan_char(prs->in) == U'*')
		{
			parser_add_char(prs, cur);
			parser_skip_long_comment(prs);
			break;
		}
		else
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME, true);
		}
		storage_search(prs->stg, prs->in, &cur);
	}
	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_add_char(prs, cur);
		parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME, true);
	}

	// Необходимо подключить файл и вызвать parser_preprocess
	printf("\"%s\"\n", buffer);
}

/**
 *	Считывает имя идентификатора, его значение, добавляет в stg
 */
static void parser_define(parser *const prs)
{
	char32_t cur = uni_scan_char(prs->in);

	// Пропуск разделителей и комментариев
	while (utf8_is_separator(cur) || cur == U'/')
	{
		parser_add_char(prs, cur);
		
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'/':
					parser_skip_short_comment(prs);
					break;
				case U'*':
					parser_skip_long_comment(prs);
					break;
				default:
					uni_unscan_char(prs->in, next);
					parser_macro_error(prs, PARSER_DEFINE_INCORRECT_IDENT_NAME, true);
			}
		}

		cur = uni_scan_char(prs->in);
	}

	if (utf8_is_line_breaker(cur) || cur == (char32_t)EOF)
	{
		parser_macro_error(prs, PARSER_DEFINE_NEED_IDENT, true);
	}
	else if (!utf8_is_letter(cur))
	{
		parser_add_char(prs, cur);
		parser_macro_error(prs, PARSER_DEFINE_INCORRECT_IDENT_NAME, true);
	}
	else
	{
		char32_t id[1024];
		char32_t value[1024];
		//value[0] = U'q';
		value[0] = U'\0';

		// Запись идентификатора
		size_t i = 0;
		while (!utf8_is_separator(cur) && cur != U'/' && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			parser_add_char(prs, cur);
			if (utf8_is_letter(cur) || utf8_is_digit(cur))
			{
				id[i] = cur;
			}
			else
			{
				parser_add_char(prs, cur);
				parser_macro_error(prs, PARSER_DEFINE_INCORRECT_IDENT_NAME, true);
				break;
			}

			cur = uni_scan_char(prs->in);
			i++;
		}

		if (storage_add(prs->stg, id, value) == SIZE_MAX)	// Проверка существования
		{
			parser_macro_error(prs, PARSER_DEFINE_EXIST_IDENT, true);
		}
		else
		{
			// Запись значения
			size_t j = 0;
			while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
			{
				parser_add_char(prs, cur);

				if (cur == U'/')
				{
					char32_t next = uni_scan_char(prs->in);
					switch (next)
					{
						case U'/':
							parser_skip_short_comment(prs);
							break;
						case U'*':
							parser_skip_long_comment(prs);
							break;
						default:
							value[j] = cur;
							uni_unscan_char(prs->in, next);
					}
				}
				else
				{
					value[j] = cur;
				}

				cur = uni_scan_char(prs->in);
				j++;
			}

			value[j] = U'\0';
		}

		storage_remove(prs->stg, id);
		storage_add(prs->stg, id, value);
		
	}
}


/**
 *	Определяет тип комментария и пропускает его
 */
static void parser_scan_comment(parser *const prs)
{
	char32_t next = uni_scan_char(prs->in);
	switch (next)
	{
		case U'/':
			parser_skip_short_comment(prs);
				break;
		case U'*':
			parser_skip_long_comment(prs);
				break;

		default:	// Если встретился один '/', печатает его в out и обрабатывает следующие символы
			parser_add_char(prs, U'/');
			uni_print_char(prs->out, U'/');
			uni_unscan_char(prs->in, next);	// Символ next не имеет отношения к комментарию,
											// он будет считан повторно и проанализирован снаружи
	}
}

/**
 *	Определяет тип ключевого слова и разбирает дальнейшее выражение
 */
static void parser_scan_keyword(parser *const prs)
{
	char32_t last;
	size_t res = storage_search(prs->stg, prs->in, &last);

	size_t length = parser_add_string(prs, storage_last_read(prs->stg));	// Буфферизация считанного ключевого слова

	if (res == SIZE_MAX)
	{
		parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD, true);
	}

	parser_add_char(prs, last);	// Добавление символа после ключевого слова
	prs->position += length - 1;

	switch (res)
	{
		case KW_INCLUDE:
			parser_include(prs);
			break;
	
		case KW_DEFINE:
			parser_define(prs);
			break;
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
		printf("\n");
	}

	uni_unscan_char(prs->in, last);
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

	prs.line = FST_LINE_INDEX;
	prs.position = FST_CHARACTER_INDEX;
	prs.string[0] = '\0';

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

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		switch (cur)
		{
			case U'\r':
				uni_scan_char(prs->in);
			case U'\n':
				parser_next_string(prs);
				uni_print_char(prs->out, U'\n');
					break;

			case U'#':
				uni_unscan_char(prs->in, cur);
				parser_scan_keyword(prs);
					break;

			case U'\'':
				parser_skip_string(prs, U'\'');
					break;
			case U'\"':
				parser_skip_string(prs, U'\"');
					break;

			case U'/':
				parser_scan_comment(prs);
					break;

			default:
				if (utf8_is_letter(cur))	//	Здесь будет макроподстановка
				{
					parser_add_char(prs, cur);
					uni_print_char(prs->out, cur);
				}
				else
				{
					parser_add_char(prs, cur);
					uni_print_char(prs->out, cur);

					if (cur == U'*')	// Проверка на наличие конца длинного комментария без его начала
					{
						char32_t next = uni_scan_char(prs->in);
						if (next == U'/')
						{
							parser_add_char(prs, next);
							prs->position -= 2;	// Сдвигает position до '*'
							parser_macro_warning(prs, PARSER_COMM_END_WITHOUT_BEGINNING, true);
							prs->position += 2;
						}
						else
						{
							uni_unscan_char(prs->in, next);	// Символ next не имеет отношения к комментарию,
															// он будет считан повторно и проанализирован снаружи
						}
					}
				}
		}

		cur = uni_scan_char(prs->in);
	}

	uni_print_char(prs->out, U'\n');
	uni_print_char(prs->out, U'\n');

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

/*void parser_macro_error(parser *const prs, const error_t num)
{
	if (parser_is_correct(prs) && !prs->is_recovery_disabled && !prs->was_error)
	{
		prs->was_error = true;
		macro_error(linker_current_path(prs->lk), (char *)prs->string, prs->line, prs->position, num);
	}
}*/


bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && storage_is_correct(prs->stg) && out_is_correct(prs->out);
}


int parser_clear(parser *const prs)
{
	return prs != NULL && linker_clear(prs->lk) && storage_clear(prs->stg) && out_clear(prs->out);
}
