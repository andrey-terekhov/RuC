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
#include "uniprinter.h"
#include "uniscanner.h"


const size_t FST_LINE_INDEX =		1;
const size_t FST_CHARACTER_INDEX =	0;


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
 *	Добавляет символ в string и увеличивает значение position
 */
static inline void parser_add_char(parser *const prs, const char32_t cur)
{
	utf8_to_string(&prs->string[strlen(prs->string)], cur);
	prs->position++;
}


/**
 *	Считывает символы до конца строковой константы и буфферизирует текущую строку кода
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t old_line = prs->line;
	const size_t old_position = prs->position;

	parser_add_char(prs, ch);
	uni_print_char(prs->out, ch);

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)							// Строка считана, выход из функции
		{
			parser_add_char(prs, cur);
			uni_print_char(prs->out, cur);
			return;
		}
		else if (cur == '\r' || cur == '\n')	// Ошибка из-за наличия переноса строки
		{
			const char32_t temp_line = prs->line;
			prs->line = old_line;
			prs->position = old_position;

			parser_macro_error(prs, PARSER_STRING_NOT_ENDED);

			prs->line = temp_line;
			parser_next_string(prs);

			// Специфика последовательности "\r\n"
			if (cur == '\r')
			{
				char32_t next = uni_scan_char(prs->in);
				if (next != '\n')
				{
					uni_unscan_char(prs->in, next);
				}
			}
		}
		else									// Независимо от корректности строки выводит ее в out
		{
			parser_add_char(prs, cur);
			uni_print_char(prs->out, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);
}


/**
 *	Пропускает символы до конца комментария ('\n', '\r' или EOF)
 *
 *	@note Буфферизация строки кода не производится, поскольку, комментарий идет до конца строки
 */
static void parser_skip_short_comment(parser *const prs)
{
	parser_add_char(prs, '\\');

	char32_t cur = uni_scan_char(prs->in);
	while(cur != '\n' && cur != '\r' && cur != (char32_t)EOF)
	{
		cur = uni_scan_char(prs->in);
	}

	if (cur != (char32_t)EOF)
	{
		// Специфика последовательности "\r\n"
		if (cur == '\r')
		{
			char32_t next = uni_scan_char(prs->in);
			if (next != '\n')
			{
				uni_unscan_char(prs->in, next);
			}
		}

		uni_print_char(prs->out, '\n');
		parser_next_string(prs);
	}
}

/**
 *	Считывает символы до конца длинного комментария и буфферизирует текущую строку кода
 */
static void parser_skip_long_comment(parser *const prs)
{
	// Сохранение позиции начала комментария на случай ошибки c возможностью буфферизации до конца строки
	const char32_t old_position = prs->position;
	parser_add_char(prs, '\\');
	parser comm_beginning = *prs;
	parser_add_char(&comm_beginning, '*');

	parser_add_char(prs, '*');

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		switch (cur)
		{
			case '\r':
				uni_scan_char(prs->in);
			case '\n':
				parser_next_string(prs);
					break;

			case '*':
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
					case '/':							// Комментарий считан, выход из функции
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
	parser_macro_error(&comm_beginning, PARSER_COMM_NOT_ENDED);
	prs->was_error = true;
	parser_clear(&comm_beginning);
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
			case '\r':
				uni_scan_char(prs->in);
			case '\n':
				parser_next_string(prs);
				uni_print_char(prs->out, cur);
					break;

			case '#':
				//parser_scan_keyword(prs);
					break;

			case '\'':
				parser_skip_string(prs, '\'');
					break;
			case '\"':
				parser_skip_string(prs, '\"');
					break;

			case '/':
			{
				char32_t next = uni_scan_char(prs->in);
				switch (next)
				{
					case '/':
						parser_skip_short_comment(prs);
							break;
					case '*':
						parser_skip_long_comment(prs);
							break;

					default:	// Если встретился один '/', печатает его в out и обрабатывает следующие символы
						parser_add_char(prs, cur);
						uni_print_char(prs->out, cur);
						uni_unscan_char(prs->in, next);	// Символ next не имеет отношения к комментарию,
														// он будет считан повторно и проанализирован снаружи
				}
			}
			break;

			default:
				parser_add_char(prs, cur);
				uni_print_char(prs->out, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	uni_print_char(prs->out, '\n');
	uni_print_char(prs->out, '\n');

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

void parser_macro_error(parser *const prs, const error_t num)
{
	if (parser_is_correct(prs) && !prs->is_recovery_disabled && !prs->was_error)
	{
		prs->was_error = true;
		macro_error(linker_current_path(prs->lk), (char *)prs->string, prs->line, prs->position, num);
	}
}


bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && storage_is_correct(prs->stg) && out_is_correct(prs->out);
}


int parser_clear(parser *const prs)
{
	return prs != NULL && linker_clear(prs->lk) && storage_clear(prs->stg) && out_clear(prs->out);
}
