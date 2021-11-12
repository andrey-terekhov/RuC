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
 *	Check if error recovery disabled
 *
 *	@param	ws			Compiler workspace
 *
 *	@return	Recovery status
 */
static inline bool recovery_status(const workspace *const ws)
{
	for (size_t i = 0; ; i++)
	{
		const char *flag = ws_get_flag(ws, i);
		if (flag == NULL)
		{
			return false;
		}
		else if (strcmp(flag, "-Wno") == 0)
		{
			return true;
		}
	}
}

/**
 *	Emit an error from lexer
 *
 *	@param	lxr			Lexer
 *	@param	num			Error code
 */
static void lexer_error(lexer *const lxr, error_t num, ...)
{
	if (lxr->is_recovery_disabled && lxr->sx->was_error)
	{
		return;
	}

	va_list args;
	va_start(args, num);

	verror(lxr->sx->io, num, args);
	lxr->sx->was_error = true;

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
 *	Skip until we find '*' and '/' that terminates the comment
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
 *	@return	Keyword token on keyword, @c TK_IDENTIFIER on identifier
 */
static token_t lex_identifier_or_keyword(lexer *const lxr)
{
	uni_unscan_char(lxr->sx->io, lxr->character);
	const size_t repr = repr_reserve(lxr->sx, &lxr->character);
	const item_t ref = repr_get_reference(lxr->sx, repr);
	if (ref >= 0 && repr != ITEM_MAX)
	{
		lxr->repr = repr;
		return TK_IDENTIFIER;
	}
	else
	{
		return (token_t)ref;
	}
}

/**
 *	Lex numeric literal
 *
 *	@param	lxr			Lexer
 *
 *	@return	@c TK_INT_LITERAL on integer literal, @c TK_FLOAT_LITERAL on floating point literal
 */
static token_t lex_numeric_literal(lexer *const lxr)
{
	int64_t int_value = 0;
	double float_value = 0.0;
	bool is_integer = true;
	bool is_out_of_range = false;

	while (utf8_is_digit(lxr->character))
	{
		int_value = int_value * 10 + utf8_to_digit(lxr->character);
		float_value = float_value * 10 + utf8_to_digit(lxr->character);
		scan(lxr);
	}

	if (float_value > (double)INT_MAX)
	{
		is_out_of_range = true;
		is_integer = false;
	}

	if (lxr->character == '.')
	{
		is_integer = false;
		double position_mult = 0.1;
		while (utf8_is_digit(scan(lxr)))
		{
			float_value += utf8_to_digit(lxr->character) * position_mult;
			position_mult *= 0.1;
		}
	}

	if (utf8_is_power(lxr->character))
	{
		int power = 0;
		int sign = 1;
		scan(lxr);

		if (lxr->character == '-')
		{
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
			lexer_error(lxr, must_be_digit_after_exp);
			return TK_FLOAT_LITERAL;
		}

		while (utf8_is_digit(lxr->character))
		{
			power = power * 10 + utf8_to_digit(lxr->character);
			scan(lxr);
		}

		if (is_integer)
		{
			for (int i = 1; i <= power; i++)
			{
				int_value *= 10;
			}
		}

		float_value *= pow(10.0, sign * power);
	}

	if (is_integer)
	{
		lxr->num = int_value;
		return TK_INT_LITERAL;
	}
	else
	{
		lxr->num_double = float_value;
		if (is_out_of_range)
		{
			warning(lxr->sx->io, too_long_int);
		}
		return TK_FLOAT_LITERAL;
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
	else
	{
		return lxr->character;
	}
}

/**
 *	Lex character literal
 *	@note Lexes the remainder of a character literal after apostrophe
 *
 *	@param	lxr			Lexer
 *
 *	@return	@c TK_CHAR_LITERAL
 */
static token_t lex_char_literal(lexer *const lxr)
{
	if (scan(lxr) == '\'')
	{
		lexer_error(lxr, empty_character);
		lxr->char_value = 0;
		return TK_CHAR_LITERAL;
	}

	lxr->char_value = get_next_string_elem(lxr);

	if (scan(lxr) == '\'')
	{
		scan(lxr);
	}
	else
	{
		lexer_error(lxr, expected_apost_after_char_const);
	}
	return TK_CHAR_LITERAL;
}

/**
 *	Lex string literal
 *	@note	Lexes the remainder of a string literal after quote mark
 *
 *	@param	lxr			Lexer 
 *
 *	@return	@c TK_STRING_LITERAL
 */
static token_t lex_string_literal(lexer *const lxr)
{
	vector_resize(&lxr->lexstr, 0);
	while (lxr->character == '\"')
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

	lxr->num_string = string_add(lxr->sx, &lxr->lexstr);
	return TK_STRING_LITERAL;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


lexer lexer_create(const workspace *const ws, syntax *const sx)
{
	lexer lxr;
	lxr.sx = sx;

	lxr.is_recovery_disabled = recovery_status(ws);

	lxr.lexstr = vector_create(MAX_STRING_LENGTH);

	scan(&lxr);

	return lxr;
}

int lexer_clear(lexer *const lxr)
{
	return vector_clear(&lxr->lexstr);
}


token_t lex(lexer *const lxr)
{
	if (lxr == NULL)
	{
		return TK_EOF;
	}

	skip_whitespace(lxr);
	lxr->location = in_get_position(lxr->sx->io);

	switch (lxr->character)
	{
		case (char32_t)EOF:
			return TK_EOF;

		default:
			if (utf8_is_letter(lxr->character) || lxr->character == '#')
			{
				// Keywords and identifiers
				return lex_identifier_or_keyword(lxr);
			}
			else
			{
				lexer_error(lxr, bad_character, lxr->character);
				// Pretending the character didn't exist
				scan(lxr);
				return lex(lxr);
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
			return TK_QUESTION;

		case '[':
			scan(lxr);
			return TK_L_SQUARE;

		case ']':
			scan(lxr);
			return TK_R_SQUARE;

		case '(':
			scan(lxr);
			return TK_L_PAREN;

		case ')':
			scan(lxr);
			return TK_R_PAREN;

		case '{':
			scan(lxr);
			return TK_L_BRACE;

		case '}':
			scan(lxr);
			return TK_R_BRACE;

		case '~':
			scan(lxr);
			return TK_TILDE;

		case ':':
			scan(lxr);
			return TK_COLON;

		case ';':
			scan(lxr);
			return TK_SEMICOLON;

		case ',':
			scan(lxr);
			return TK_COMMA;

		case '.':
			if (utf8_is_digit(lookahead(lxr)))
			{
				return lex_numeric_constant(lxr);
			}
			else
			{
				scan(lxr);
				return TK_PERIOD;
			}

		case '*':
			if (scan(lxr) == '=')
			{
				scan(lxr);
				return TK_STAR_EQUAL;
			}
			else
			{
				return TK_STAR;
			}

		case '!':
			if (scan(lxr) == '=')
			{
				scan(lxr);
				return TK_EXCLAIM_EQUAL;
			}
			else
			{
				return TK_EXCLAIM;
			}

		case '%':
			if (scan(lxr) == '=')
			{
				scan(lxr);
				return TK_PERCENT_EQUAL;
			}
			else
			{
				return TK_PERCENT;
			}

		case '^':
			if (scan(lxr) == '=')
			{
				scan(lxr);
				return TK_CARET_EQUAL;
			}
			else
			{
				return TK_CARET;
			}

		case '=':
			if (scan(lxr) == '=')
			{
				scan(lxr);
				return TK_EQUAL_EQUAL;
			}
			else
			{
				return TK_EQUAL;
			}

		case '+':
			switch (scan(lxr))
			{
				case '=':
					scan(lxr);
					return TK_PLUS_EQUAL;

				case '+':
					scan(lxr);
					return TK_PLUS_PLUS;

				default:
					return TK_PLUS;
			}

		case '|':
			switch (scan(lxr))
			{
				case '=':
					scan(lxr);
					return TK_PIPE_EQUAL;

				case '|':
					scan(lxr);
					return TK_PIPE_PIPE;

				default:
					return TK_PIPE;
			}

		case '&':
			switch (scan(lxr))
			{
				case '=':
					scan(lxr);
					return TK_AMP_EQUAL;

				case '&':
					scan(lxr);
					return TK_AMP_AMP;

				default:
					return TK_AMP;
			}

		case '-':
			switch (scan(lxr))
			{
				case '=':
					scan(lxr);
					return TK_MINUS_EQUAL;

				case '-':
					scan(lxr);
					return TK_MINUS_MINUS;

				case '>':
					scan(lxr);
					return TK_ARROW;

				default:
					return TK_MINUS;
			}

		case '<':
			switch (scan(lxr))
			{
				case '<':
					if (scan(lxr) == '=')
					{
						scan(lxr);
						return TK_LESS_LESS_EQUAL;
					}
					else
					{
						return TK_LESS_LESS;
					}

				case '=':
					scan(lxr);
					return TK_LESS_EQUAL;

				default:
					return TK_LESS;
			}

		case '>':
			switch (scan(lxr))
			{
				case '>':
					if (scan(lxr) == '=')
					{
						scan(lxr);
						return TK_GREATER_GREATER_EQUAL;
					}
					else
					{
						return TK_GREATER_GREATER;
					}

				case '=':
					scan(lxr);
					return TK_GREATER_EQUAL;

				default:
					return TK_GREATER;
			}

		case '/':
			switch (scan(lxr))
			{
				case '=':
					scan(lxr);
					return TK_SLASH_EQUAL;

				// Comments [C99 6.4.9]
				case '/':
					skip_line_comment(lxr);
					return lex(lxr);

				case '*':
					skip_block_comment(lxr);
					return lex(lxr);

				default:
					return TK_SLASH;
			}
	}
}

token_t peek(lexer *const lxr)
{
	const size_t position = in_get_position(lxr->sx->io);
	const char32_t character = lxr->character;
	const token_t peek_token = lex(lxr);
	lxr->character = character;
	in_set_position(lxr->sx->io, position);
	return peek_token;
}
