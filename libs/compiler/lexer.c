/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include "lexer.h"
#include <math.h>
#include <string.h>
#include "uniscanner.h"


/**
 *	Emit an error from lexer
 *
 *	@param	lxr			Lexer
 *	@param	num			Error code
 */
static void lexer_error(lexer *const lxr, err_t num, ...)
{
	const size_t position = in_get_position(lxr->sx->io);
	const location loc = { position, position + 1 };

	va_list args;
	va_start(args, num);

	report_error(&lxr->sx->rprt, lxr->sx->io, loc, num, args);

	va_end(args);
}

/**
 *	Scan next character from io
 *
 *	@param	lxr			Lexer
 *
 *	@return	Scanned character
 */
static inline char32_t scan(lexer *const lxr)
{
	lxr->character = uni_scan_char(lxr->sx->io);
	return lxr->character;
}

/**
 *	Peek next character from io
 *
 *	@param	lxr			Lexer
 *
 *	@return	Peeked character
 */
static inline char32_t lookahead(lexer *const lxr)
{
	const size_t position = in_get_position(lxr->sx->io);
	const char32_t result = uni_scan_char(lxr->sx->io);
	in_set_position(lxr->sx->io, position);
	return result;
}

/**
 *	Skip over a series of whitespace characters
 *
 *	@param	lxr			Lexer
 */
static inline void skip_whitespace(lexer *const lxr)
{
	while (lxr->character == '\n' || lxr->character == '\r'
		|| lxr->character == '\t' || lxr->character == ' ')
	{
		scan(lxr);
	}
}

/**
 *	Skip until we find the newline character that terminates the comment
 *
 *	@param	lxr			Lexer
 */
static inline void skip_line_comment(lexer *const lxr)
{
	while (lxr->character != '\n' && lxr->character != (char32_t)EOF)
	{
		scan(lxr);
	}
}

/**
 *	Skip until we find '*' and '/' that terminate the comment
 *
 *	@param	lxr			Lexer
 */
static inline void skip_block_comment(lexer *const lxr)
{
	while (lxr->character != '*' || scan(lxr) != '/')
	{
		if (lxr->character == (char32_t)EOF)
		{
			lexer_error(lxr, unterminated_block_comment);
			return;
		}

		scan(lxr);
	}

	scan(lxr);
}

/**
 *	Lex identifier or keyword
 *
 *	@param	lxr			Lexer
 *
 *	@return	Keyword token on keyword, identifier token on identifier
 */
static token lex_identifier_or_keyword(lexer *const lxr)
{
	assert(utf8_is_letter(lxr->character));
	const size_t loc_begin = in_get_position(lxr->sx->io);

	uni_unscan_char(lxr->sx->io, lxr->character);
	const size_t repr = repr_reserve(lxr->sx, &lxr->character);

	const size_t loc_end = in_get_position(lxr->sx->io);
	const item_t ref = repr_get_reference(lxr->sx, repr);

	if (ref >= 0)
	{
		return token_identifier((location){ loc_begin, loc_end }, repr);
	}

	return token_keyword((location){ loc_begin, loc_end }, (token_t)ref);
}

/**
 *	Lex numeric literal
 *
 *	@param	lxr			Lexer
 *
 *	@return	Numeric literal token
 */
static token lex_numeric_literal(lexer *const lxr)
{
	assert(utf8_is_digit(lxr->character) || lxr->character == '.');
	const size_t loc_begin = in_get_position(lxr->sx->io);

	// Основание по умолчанию - 10
	uint8_t base = 10;
	bool was_modifier = false;

	// Если есть ведущий ноль, то после него может быть модификатор счисления
	if (lxr->character == '0')
	{
		scan(lxr);
		switch (lxr->character)
		{
			case 'x':
			case 'X':
				base = 16;
				was_modifier = true;
				scan(lxr);
				break;

			case 'd':
			case 'D':
				was_modifier = true;
				scan(lxr);
				break;

			case 'o':
			case 'O':
				base = 8;
				was_modifier = true;
				scan(lxr);
				break;

			case 'b':
			case 'B':
				base = 2;
				was_modifier = true;
				scan(lxr);
				break;
		}
	}

	// Переменные для подсчета значения
	uint64_t int_value = 0;
	double float_value = 0.0;
	bool is_in_range = true;
	bool is_integer = true;

	while (utf8_is_hexa_digit(lxr->character))
	{
		if (!was_modifier && utf8_is_power(lxr->character))
		{
			// Встретили экспоненту - выходим из цикла
			break;
		}

		const uint8_t digit = utf8_to_number(lxr->character);
		if (digit >= base)
		{
			// Ошибка - цифра не из той системы
			lexer_error(lxr, digit_of_another_base);
			// Пропустим все цифры и вернем токен
			while (utf8_is_hexa_digit(lxr->character))
			{
				scan(lxr);
			}

			const size_t loc_end = in_get_position(lxr->sx->io);
			return token_int_literal((location){ loc_begin, loc_end }, int_value);
		}

		if ((base == 2 && int_value >= 0x8000000000000000)
			|| (base == 8 && int_value >= 0x2000000000000000)
			|| (int_value >= 0x1000000000000000))
		{
			// Переполнение хранилища - конвертируем в double
			float_value = (double)int_value;
			is_in_range = false;
			is_integer = false;
		}

		if (is_integer)
		{
			int_value = int_value * base + digit;
		}
		else
		{
			float_value = float_value * base + digit;
		}

		scan(lxr);
	}

	// Дробная часть разрешена только для чисел без спецификаторов
	if (lxr->character == '.' && !was_modifier)
	{
		if (is_integer)
		{
			float_value = (double)int_value;
		}

		is_integer = false;
		double position_mult = 0.1;
		// Читаем только десятичные цифры
		// Все остальное относится к следюущим токенам
		while (utf8_is_digit(scan(lxr)))
		{
			float_value += utf8_to_number(lxr->character) * position_mult;
			position_mult *= 0.1;
		}
	}

	// Экспонента разрешена только для чисел без спецификаторов
	if (utf8_is_power(lxr->character) && !was_modifier)
	{
		int64_t power = 0;	// Показатель степени
		int sign = 1;		// Знак степени
		scan(lxr);

		if (lxr->character == '-')
		{
			if (is_integer)
			{
				float_value = (double)int_value;
			}

			is_integer = false;
			scan(lxr);
			sign = -1;
		}
		else if (lxr->character == '+')
		{
			scan(lxr);
		}

		if (!utf8_is_digit(lxr->character))
		{
			// Ошибка - после экспоненты должны быть цифры
			lexer_error(lxr, exponent_has_no_digits);
			// Пропустим все лишнее
			while (utf8_is_letter(lxr->character) || utf8_is_digit(lxr->character)
				|| lxr->character == '+' || lxr->character == '-')
			{
				scan(lxr);
			}

			const size_t loc_end = in_get_position(lxr->sx->io);
			return token_float_literal((location){ loc_begin, loc_end }, DBL_MAX);
		}

		while (utf8_is_digit(lxr->character))
		{
			power = power * 10 + utf8_to_number(lxr->character);
			scan(lxr);
		}

		if (is_integer)
		{
			for (int64_t i = 0; i < power; i++)
			{
				int_value *= 10;
			}
		}

		float_value *= pow(10.0, sign * (double)power);
	}

	// Формируем результат
	const size_t loc_end = in_get_position(lxr->sx->io);
	if (is_integer)
	{
		return token_int_literal((location){ loc_begin, loc_end }, int_value);
	}
	else
	{
		if (!is_in_range)
		{
			// Вышли за пределы целого - конвертируем в double
			warning(lxr->sx->io, too_long_int);
		}

		return token_float_literal((location){ loc_begin, loc_end }, float_value);
	}
}

/**	Get character or escape sequence after '\' */
static inline char32_t get_next_string_elem(lexer *const lxr)
{
	if (lxr->character == '\\')
	{
		switch (scan(lxr))
		{
			case 'n':
			case U'н':
				return '\n';

			case 't':
			case U'т':
				return '\t';

			case '0':
				return '\0';

			case '\\':
			case '\'':
			case '\"':
				return lxr->character;

			default:
				lexer_error(lxr, unknown_escape_sequence);
				return lxr->character;
		}
	}

	return lxr->character;
}

/**
 *	Lex character literal
 *
 *	@param	lxr			Lexer
 *
 *	@return	Character literal token
 */
static token lex_char_literal(lexer *const lxr)
{
	assert(lxr->character == '\'');
	const size_t loc_begin = in_get_position(lxr->sx->io);

	if (scan(lxr) == '\'')
	{
		lexer_error(lxr, empty_character_literal);
		scan(lxr);

		const size_t loc_end = in_get_position(lxr->sx->io);
		return token_char_literal((location){ loc_begin, loc_end }, '\0');
	}

	const char32_t value = get_next_string_elem(lxr);

	if (scan(lxr) == '\'')
	{
		scan(lxr);
	}
	else
	{
		lexer_error(lxr, missing_terminating_apost_char);
	}

	const size_t loc_end = in_get_position(lxr->sx->io);
	return token_char_literal((location){ loc_begin, loc_end }, value);
}

/**
 *	Lex string literal
 *
 *	@param	lxr			Lexer
 *
 *	@return	String literal token
 */
static token lex_string_literal(lexer *const lxr)
{
	assert(lxr->character == '"');
	const size_t loc_begin = in_get_position(lxr->sx->io);

	while (lxr->character == '"')
	{
		scan(lxr);
		while (lxr->character != '"' && lxr->character != '\n')
		{
			vector_add(&lxr->lexstr, get_next_string_elem(lxr));
			scan(lxr);
		}
		if (lxr->character == '"')
		{
			scan(lxr);
		}
		else
		{
			lexer_error(lxr, missing_terminating_quote_char);
		}

		skip_whitespace(lxr);
	}

	const size_t loc_end = in_get_position(lxr->sx->io);
	const size_t index = string_add(lxr->sx, &lxr->lexstr);
	vector_resize(&lxr->lexstr, 0);

	return token_string_literal((location){ loc_begin, loc_end }, index);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


lexer lexer_create(syntax *const sx)
{
	lexer lxr = { .sx = sx };
	lxr.lexstr = vector_create(MAX_STRING_LENGTH);

	scan(&lxr);

	return lxr;
}

int lexer_clear(lexer *const lxr)
{
	return vector_clear(&lxr->lexstr);
}


token lex(lexer *const lxr)
{
	if (lxr == NULL)
	{
		return token_eof();
	}

	while (true)
	{
		skip_whitespace(lxr);
		const size_t loc_begin = in_get_position(lxr->sx->io);
		token_t punctuator_kind;

		switch (lxr->character)
		{
			case (char32_t)EOF:
				return token_eof();

			default:
				if (utf8_is_letter(lxr->character))
				{
					// Keywords and identifiers
					return lex_identifier_or_keyword(lxr);
				}
				else
				{
					lexer_error(lxr, bad_character);
					// Pretending the character didn't exist
					scan(lxr);
					continue;
				}

				// Integer and floating literals
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				return lex_numeric_literal(lxr);

			case '\'':	// Character literals
				return lex_char_literal(lxr);

			case '\"':	// String literals
				return lex_string_literal(lxr);

				// Punctuators
			case '?':
				scan(lxr);
				punctuator_kind = TK_QUESTION;
				break;

			case '[':
				scan(lxr);
				punctuator_kind = TK_L_SQUARE;
				break;

			case ']':
				scan(lxr);
				punctuator_kind = TK_R_SQUARE;
				break;

			case '(':
				scan(lxr);
				punctuator_kind = TK_L_PAREN;
				break;

			case ')':
				scan(lxr);
				punctuator_kind = TK_R_PAREN;
				break;

			case '{':
				scan(lxr);
				punctuator_kind = TK_L_BRACE;
				break;

			case '}':
				scan(lxr);
				punctuator_kind = TK_R_BRACE;
				break;

			case '~':
				scan(lxr);
				punctuator_kind = TK_TILDE;
				break;

			case ':':
				scan(lxr);
				punctuator_kind = TK_COLON;
				break;

			case ';':
				scan(lxr);
				punctuator_kind = TK_SEMICOLON;
				break;

			case ',':
				scan(lxr);
				punctuator_kind = TK_COMMA;
				break;

			case '.':
				if (utf8_is_digit(lookahead(lxr)))
				{
					return lex_numeric_literal(lxr);
				}
				else
				{
					scan(lxr);
					punctuator_kind = TK_PERIOD;
					break;
				}

			case '*':
				if (scan(lxr) == '=')
				{
					scan(lxr);
					punctuator_kind = TK_STAR_EQUAL;
				}
				else
				{
					punctuator_kind = TK_STAR;
				}
				break;

			case '!':
				if (scan(lxr) == '=')
				{
					scan(lxr);
					punctuator_kind = TK_EXCLAIM_EQUAL;
				}
				else
				{
					punctuator_kind = TK_EXCLAIM;
				}
				break;

			case '%':
				if (scan(lxr) == '=')
				{
					scan(lxr);
					punctuator_kind = TK_PERCENT_EQUAL;
				}
				else
				{
					punctuator_kind = TK_PERCENT;
				}
				break;

			case '^':
				if (scan(lxr) == '=')
				{
					scan(lxr);
					punctuator_kind = TK_CARET_EQUAL;
				}
				else
				{
					punctuator_kind = TK_CARET;
				}
				break;

			case '=':
				if (scan(lxr) == '=')
				{
					scan(lxr);
					punctuator_kind = TK_EQUAL_EQUAL;
				}
				else
				{
					punctuator_kind = TK_EQUAL;
				}
				break;

			case '+':
				switch (scan(lxr))
				{
					case '=':
						scan(lxr);
						punctuator_kind = TK_PLUS_EQUAL;
						break;

					case '+':
						scan(lxr);
						punctuator_kind = TK_PLUS_PLUS;
						break;

					default:
						punctuator_kind = TK_PLUS;
						break;
				}
				break;

			case '|':
				switch (scan(lxr))
				{
					case '=':
						scan(lxr);
						punctuator_kind = TK_PIPE_EQUAL;
						break;

					case '|':
						scan(lxr);
						punctuator_kind = TK_PIPE_PIPE;
						break;

					default:
						punctuator_kind = TK_PIPE;
						break;
				}
				break;

			case '&':
				switch (scan(lxr))
				{
					case '=':
						scan(lxr);
						punctuator_kind = TK_AMP_EQUAL;
						break;

					case '&':
						scan(lxr);
						punctuator_kind = TK_AMP_AMP;
						break;

					default:
						punctuator_kind = TK_AMP;
						break;
				}
				break;

			case '-':
				switch (scan(lxr))
				{
					case '=':
						scan(lxr);
						punctuator_kind = TK_MINUS_EQUAL;
						break;

					case '-':
						scan(lxr);
						punctuator_kind = TK_MINUS_MINUS;
						break;

					case '>':
						scan(lxr);
						punctuator_kind = TK_ARROW;
						break;

					default:
						punctuator_kind = TK_MINUS;
						break;
				}
				break;

			case '<':
				switch (scan(lxr))
				{
					case '<':
						if (scan(lxr) == '=')
						{
							scan(lxr);
							punctuator_kind = TK_LESS_LESS_EQUAL;
						}
						else
						{
							punctuator_kind = TK_LESS_LESS;
						}
						break;

					case '=':
						scan(lxr);
						punctuator_kind = TK_LESS_EQUAL;
						break;

					default:
						punctuator_kind = TK_LESS;
						break;
				}
				break;

			case '>':
				switch (scan(lxr))
				{
					case '>':
						if (scan(lxr) == '=')
						{
							scan(lxr);
							punctuator_kind = TK_GREATER_GREATER_EQUAL;
						}
						else
						{
							punctuator_kind = TK_GREATER_GREATER;
						}
						break;

					case '=':
						scan(lxr);
						punctuator_kind = TK_GREATER_EQUAL;
						break;

					default:
						punctuator_kind = TK_GREATER;
						break;
				}
				break;

			case '/':
				switch (scan(lxr))
				{
					case '=':
						scan(lxr);
						punctuator_kind = TK_SLASH_EQUAL;
						break;

					case '/':	// Line comment
						skip_line_comment(lxr);
						continue;

					case '*':	// Block comment
						skip_block_comment(lxr);
						continue;

					default:
						punctuator_kind = TK_SLASH;
						break;
				}
				break;
		}

		const size_t loc_end = in_get_position(lxr->sx->io);
		return token_punctuator((location){ loc_begin, loc_end }, punctuator_kind);
	}
}

token_t peek(lexer *const lxr)
{
	const size_t position = in_get_position(lxr->sx->io);
	const char32_t character = lxr->character;
	const token peek_token = lex(lxr);
	lxr->character = character;
	in_set_position(lxr->sx->io, position);
	return token_get_kind(&peek_token);
}
