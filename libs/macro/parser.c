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
 *	Печатает в prs.out строку кода
 */
static inline void parser_print(parser *const prs)
{
	const size_t size = strings_size(&prs->string);
	for(size_t i = 0; i < size; i++)
	{
		uni_printf(prs->out, "%s", strings_get(&prs->string, i));
	}

	uni_print_char(prs->out, '\n');
}

/**
 *	Увеличивает значение line и сбрасывает значение position
 */
static inline void parser_next_line(parser *const prs)
{
	parser_print(prs);

	prs->line_position = in_get_position(prs->in);
	prs->line++;
	prs->position = FST_CHARACTER_INDEX;
	strings_clear(&prs->string);
	prs->string = strings_create(256);
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

	strings_add(&prs->string, str);
	return strlen(str);
}

/**
 *	Добавляет символ в string и увеличивает значение position
 */
static inline void parser_add_char(parser *const prs, const char32_t cur)
{
	char buffer[9];
	utf8_to_string(buffer, cur);

	strings_add(&prs->string, buffer);
	prs->position++;
}

/**
 *	Заполняет string до конца строки
 */
/*static inline char parser_fill_string(parser *const prs)
{
	char str[256];
	str[0] = '\0';

	char32_t cur = uni_scan_char(prs->in);
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		utf8_to_string(&str[strlen(str)], cur);
		cur = uni_scan_char(prs->in);
	}

	return str;
}*/

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
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_macro_error(parser *const prs, const error_t num)
{
	if (parser_is_correct(prs) && !prs->is_recovery_disabled && !prs->was_error)
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
		macro_error(linker_current_path(prs->lk), str, prs->line, prs->position - 1, num);
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
	if (parser_is_correct(prs))
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

		macro_warning(linker_current_path(prs->lk), str, prs->line, prs->position - 1, num);
		in_set_position(prs->in, position);
	}
}


/**
 *	Считывает символы до конца строковой константы и буфферизирует текущую строку кода
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t old_position = prs->position;

	parser_add_char(prs, ch);

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
			}
			else
			{
				parser_add_char(prs, cur);
				return;
			}
		}
		else if (utf8_is_line_breaker(cur))	// Ошибка из-за наличия переноса строки
		{
			prs->position = old_position;
			parser_macro_error(prs, PARSER_STRING_NOT_ENDED);

			parser_next_line(prs);

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
		}

		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);
}

/**
 *	Пропускает символы до конца комментария ('\n', '\r' или EOF)
 */
static void parser_skip_short_comment(parser *const prs)
{
	char32_t cur = uni_scan_char(prs->in);
	while(!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);
}

/**
 *	Считывает символы до конца длинного комментария и буфферизирует текущую строку кода
 */
static void parser_skip_long_comment(parser *const prs, char32_t *const last)
{
	// Сохранение позиции начала комментария на случай ошибки c возможностью буфферизации до конца строки
	const size_t old_position = prs->position;
	//parser_add_char(prs, U'/');
	parser comm_beginning = *prs;
	//parser_add_char(&comm_beginning, U'*');

	//parser_add_char(prs, U'*');

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		switch (cur)
		{
			case U'\r':
				uni_scan_char(prs->in);
			case U'\n':
				parser_next_line(prs);
					break;

			case U'*':
			{
				//parser_add_char(prs, cur);

				char32_t next = uni_scan_char(prs->in);
				switch (next)
				{
					case U'/':							// Комментарий считан, выход из функции
						//parser_add_char(prs, next);
							return;

					default:							// Если встретился один '*', добавляет его в буффер строки кода
														// и обрабатывает следующие символы
						uni_unscan_char(prs->in, next);	// Символ next не имеет отношения к комментарию,
														// он будет считан повторно и проанализирован снаружи
				}
			}
			break;

			//default:
				//parser_add_char(prs, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	comm_beginning.position = old_position;
	//uni_unscan_char(prs->in, cur);
	*last = (char32_t)EOF;
	parser_macro_error(&comm_beginning, PARSER_COMM_NOT_ENDED);//if (cur == (char32_t)EOF)printf("'%c'\t \n", cur);
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
			parser_skip_long_comment(prs, &cur);
		}
		else
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_add_char(prs, cur);
			prs->position = include_position;
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		}
		storage_search(prs->stg, prs->in, &cur);
	}

	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_add_char(prs, cur);
		prs->position = include_position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
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
			parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		}
	}
	else
	{
		prs->position = include_position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
	}

	// Обработка символов за путем
	storage_search(prs->stg, prs->in, &cur);
	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
	}

	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		if (storage_last_read(prs->stg) != NULL)
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_add_char(prs, cur);
			parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
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
			parser_skip_long_comment(prs, &cur);
			break;
		}
		else
		{
			parser_add_string(prs, storage_last_read(prs->stg));
			parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
		}
		storage_search(prs->stg, prs->in, &cur);
	}
	if (storage_last_read(prs->stg) != NULL)
	{
		parser_add_string(prs, storage_last_read(prs->stg));
		parser_add_char(prs, cur);
		parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
	}

	// Необходимо подключить файл и вызвать parser_preprocess
	printf("\"%s\"\n", buffer);
}

/**
 *	Считывает имя идентификатора, его значение, добавляет в stg
 *
 *	@param	prs			Структура парсера
 *	@param	mode		Режим работы функции
 *						@c KW_DEFINE #define
 *						@c KW_SET	 #set
 *						@c KW_UNDEF	 #undef
 */
static void parser_define(parser *const prs, const keyword_t mode)
{
	char32_t cur = uni_scan_char(prs->in);

	// Пропуск разделителей и комментариев
	while (utf8_is_separator(cur) || cur == U'/')
	{
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'/':
					parser_skip_short_comment(prs);
					break;
				case U'*':
					parser_skip_long_comment(prs, &cur);
					break;
				default:
					parser_add_char(prs, cur);
					uni_unscan_char(prs->in, next);
					parser_macro_error(prs, PARSER_INCORRECT_IDENT_NAME);
			}
		}
		else
		{
			parser_add_char(prs, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	if (utf8_is_line_breaker(cur) || cur == (char32_t)EOF)
	{
		parser_macro_error(prs, mode == KW_DEFINE
								? PARSER_DEFINE_NEED_IDENT
								: mode == KW_SET
									? PARSER_SET_NEED_IDENT
									: PARSER_UNDEF_NEED_IDENT);
	}
	else if (!utf8_is_letter(cur))
	{
		parser_add_char(prs, cur);
		parser_macro_error(prs, PARSER_INCORRECT_IDENT_NAME);
	}
	else
	{
		char32_t id[1024];
		id[0] = U'\0';
		char32_t value[1024];
		value[0] = U'\0';

		// Запись идентификатора
		size_t i = 0;
		while (!utf8_is_separator(cur) && cur != U'/' && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			parser_add_char(prs, cur);
			if (utf8_is_letter(cur) || utf8_is_digit(cur))
			{
				id[i++] = cur;
			}
			else
			{
				parser_macro_error(prs, PARSER_INCORRECT_IDENT_NAME);
				break;
			}

			cur = uni_scan_char(prs->in);
		}
		id[i] = U'\0';

		// Проверка существования
		if (mode == KW_DEFINE && storage_add(prs->stg, id, value) == SIZE_MAX)
		{
			parser_macro_warning(prs, PARSER_DEFINE_EXIST_IDENT);
		}
		else if (mode == KW_SET && storage_add(prs->stg, id, value) != SIZE_MAX)
		{
			parser_macro_warning(prs, PARSER_SET_NOT_EXIST_IDENT);
		}
		else if (mode == KW_UNDEF)
		{
			storage_remove(prs->stg, id);

			// Проверка последующего кода для #undef
			while (utf8_is_separator(cur) || cur == U'/')
			{
				if (cur == U'/')
				{
					char32_t next = uni_scan_char(prs->in);
					switch (next)
					{
						case U'/':
							parser_skip_short_comment(prs);
							break;
						case U'*':
							parser_skip_long_comment(prs, &cur);
							break;
						default:
							parser_add_char(prs, cur);
							uni_unscan_char(prs->in, next);
							parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
					}
				}
				else
				{
					parser_add_char(prs, cur);
				}

				cur = uni_scan_char(prs->in);
			}

			if (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
			{
				parser_add_char(prs, cur);
				parser_macro_error(prs, PARSER_UNEXPECTED_LEXEME);
			}
			return;
		}

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
						parser_skip_long_comment(prs, &cur);
						break;
					default:
						value[j++] = cur;
						uni_unscan_char(prs->in, next);
				}
			}
			else
			{
				value[j++] = cur;
			}

			cur = uni_scan_char(prs->in);
		}

		value[j] = U'\0';

		storage_set(prs->stg, id, value);
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
	prs.string = strings_create(256);

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
	// comment_create

	char32_t cur = U'\0';
	size_t index = 0;
	bool was_slash = false;

	while (cur != (char32_t)EOF)
	{
		index = storage_search(prs->stg, prs->in, &cur);
		switch (index)
		{
			case KW_INCLUDE:
				parser_include(prs);
				break;
		
			case KW_DEFINE:
				parser_define(prs, KW_DEFINE);
				break;
			case KW_SET:
				parser_define(prs, KW_SET);
				break;
			case KW_UNDEF:
				parser_define(prs, KW_UNDEF);
				break;

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
					was_slash = false;

					if (storage_last_read(prs->stg)[0] == '#')
					{
						parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
					}

					if (index != SIZE_MAX)
					{
						// Макроподстановка
						parser_add_string(prs, storage_get_by_index(prs->stg, index));
					}
					else
					{
						parser_add_string(prs, storage_last_read(prs->stg));
					}
				}

				switch (cur)
				{
					case U'#':
						was_slash = false;
						parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
						break;

					case U'\'':
						was_slash = false;
						parser_skip_string(prs, U'\'');
						break;
					case U'\"':
						was_slash = false;
						parser_skip_string(prs, U'\"');
						break;

					case U'\r':
						cur = uni_scan_char(prs->in);
						if (cur != U'\n' && cur != (char32_t)EOF)
						{
							uni_unscan_char(prs->in, cur);
						}
					case U'\n':
					case (char32_t)EOF:
						was_slash = false;
						parser_next_line(prs);
						break;

					case U'/':
						if (was_slash)
						{
							was_slash = false;
							strings_remove(&prs->string);
							parser_skip_short_comment(prs);
						}
						else
						{
							was_slash = true;
							parser_add_char(prs, cur);
						}
						break;
					case U'*':
						if (was_slash)
						{
							was_slash = false;
							strings_remove(&prs->string);
							parser_skip_long_comment(prs, &cur);
							break;
						}
						else
						{
							parser_add_char(prs, cur);
						}
						break;

					default:
						was_slash = false;
						parser_add_char(prs, cur);
				}
		}
	}


	/*

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
					uni_unscan_char(prs->in, cur);

					size_t id = storage_search(prs->stg, prs->in, &cur);
					if (id != SIZE_MAX)
					{
						uni_printf(prs->out, "%s", storage_get_by_index(prs->stg, id));
					}
					else
					{
						prs->position += parser_add_string(prs, storage_last_read(prs->stg));
						if (storage_last_read(prs->stg) != NULL)
						{
							uni_printf(prs->out, "%s", storage_last_read(prs->stg));
						}
					}
					uni_unscan_char(prs->in, cur);
				}
				else
				{
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

					parser_add_char(prs, cur);
					uni_print_char(prs->out, cur);
				}
		}

		cur = uni_scan_char(prs->in);
	}

	uni_print_char(prs->out, U'\n');
	uni_print_char(prs->out, U'\n');
	*/

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
	return prs != NULL && linker_clear(prs->lk) && storage_clear(prs->stg) &&  strings_clear(&prs->string) && out_clear(prs->out);
}
