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
#include "errors.h"
#include "uniscanner.h"


/**
 *	Emit an error from lexer
 *
 *	@param	lxr			Lexer structure
 *	@param	num			Error code
 */
void lexer_error(lexer *const lxr, error_t num, ...)
{
	lxr->was_error = 1;

	va_list args;
	va_start(args, num);

	verror(lxr->io, num, args);
	
	va_end(args);
}

/**
 *	Skip over a series of whitespace characters
 *
 *	@param	lxr			Lexer structure
 */
void skip_whitespace(lexer *const lxr)
{
	while (lxr->curr_char == ' ' || lxr->curr_char == '\t'
		|| lxr->curr_char == '\n' || lxr->curr_char == '\r')
	{
		get_char(lxr);
	}
}

/**
 *	Skip until we find the newline character that terminates the comment
 *
 *	@param	lxr			Lexer structure
 */
void skip_line_comment(lexer *const lxr)
{
	while (lxr->curr_char != '\n' && lxr->next_char != (char32_t)EOF)
	{
		get_char(lxr);
	}
}

/**
 *	Skip until we find '*' and '/' that terminates the comment
 *
 *	@param	lxr			Lexer structure
 */
void skip_block_comment(lexer *const lxr)
{
	while (lxr->curr_char != '*' && lxr->next_char != '/')
	{
		if (lxr->next_char == (char32_t)EOF)
		{
			lexer_error(lxr, unterminated_block_comment);
			return;
		}
		get_char(lxr);
	}
	get_char(lxr);
}

/**
 *	Lex identifier or keyword [C99 6.4.1 & 6.4.2]
 *
 *	@param	lxr			Lexer structure
 *
 *	@return	keyword number on keyword, @c identifier on identifier
 */
token_t lex_identifier_or_keyword(lexer *const lxr)
{
	char32_t spelling[MAXSTRINGL];
	size_t length = 0;

	do
	{
		spelling[length++] = lxr->curr_char;
		get_char(lxr);
	} while (utf8_is_letter(lxr->curr_char) || utf8_is_digit(lxr->curr_char));
	spelling[length] = '\0';

	const size_t repr = repr_reserve(lxr->sx, spelling);
	const item_t ref = repr_get_reference(lxr->sx, repr);
	if (ref >= 0)
	{
		lxr->repr = repr;
		return identifier;
	}
	else
	{
		return (token_t)ref;
	}
}

/**
 *	Lex numeric constant [C99 6.4.4.1 & 6.4.4.2]
 *
 *	@param	lxr			Lexer structure
 *
 *	@return	@c int_constant on integer, @c float_constant on floating point
 */
token_t lex_numeric_constant(lexer *const lxr)
{
	int num_int = 0;
	double num_double = 0.0;
	int flag_int = 1;
	int flag_too_long = 0;

	while (utf8_is_digit(lxr->curr_char))
	{
		num_int = num_int * 10 + (lxr->curr_char - '0');
		num_double = num_double * 10 + (lxr->curr_char - '0');
		get_char(lxr);
	}

	if (num_double > (double)INT_MAX)
	{
		flag_too_long = 1;
		flag_int = 0;
	}

	if (lxr->curr_char == '.')
	{
		flag_int = 0;
		double position_mult = 0.1;
		while (utf8_is_digit(get_char(lxr)))
		{
			num_double += (lxr->curr_char - '0') * position_mult;
			position_mult *= 0.1;
		}
	}

	if (utf8_is_power(lxr->curr_char))
	{
		int power = 0;
		int sign = 1;
		get_char(lxr);

		if (lxr->curr_char == '-')
		{
			flag_int = 0;
			get_char(lxr);
			sign = -1;
		}
		else if (lxr->curr_char == '+')
		{
			get_char(lxr);
		}

		if (!utf8_is_digit(lxr->curr_char))
		{
			lexer_error(lxr, must_be_digit_after_exp);
			return float_constant;
		}

		while (utf8_is_digit(lxr->curr_char))
		{
			power = power * 10 + (lxr->curr_char - '0');
			get_char(lxr);
		}

		if (flag_int)
		{
			for (int i = 1; i <= power; i++)
			{
				num_int *= 10;
			}
		}
		num_double *= pow(10.0, sign * power);
	}

	if (flag_int)
	{
		lxr->num = num_int;
		return int_constant;
	}
	else
	{
		lxr->num_double = num_double;
		if (flag_too_long)
		{
			warning(lxr->io, too_long_int);
		}
		return float_constant;
	}
}

/**	Get character or escape sequence after '\' */
char32_t get_next_string_elem(lexer *const lxr)
{
	if (lxr->curr_char == '\\')
	{
		switch (get_char(lxr))
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
				return lxr->curr_char;

			default:
				lexer_error(lxr, unknown_escape_sequence);
				return lxr->curr_char;
		}
	}
	else
	{
		return lxr->curr_char;
	}
}

/**
 *	Lex character constant [C99 6.4.4.4]
 *	@note Lexes the remainder of a character constant after apostrophe
 *
 *	@param	lxr			Lexer structure
 *
 *	@return	@c char_constant
 */
token_t lex_char_constant(lexer *const lxr)
{
	if (get_char(lxr) == '\'')
	{
		lexer_error(lxr, empty_character);
		lxr->num = 0;
		return char_constant;
	}

	lxr->num = get_next_string_elem(lxr);

	if (get_char(lxr) == '\'')
	{
		get_char(lxr);
	}
	else
	{
		lexer_error(lxr, expected_apost_after_char_const);
	}
	return char_constant;
}

/**
 *	Lex string literal [C99 6.4.5]
 *	@note	Lexes the remainder of a string literal after quote mark
 *
 *	@param	lxr			Lexer structure
 *
 *	@return	@c string_literal
 */
token_t lex_string_literal(lexer *const lxr)
{
	size_t length = 0;
	int flag_too_long_string = 0;
	while (lxr->curr_char == '\"')
	{
		get_char(lxr);
		while (lxr->curr_char != '"' && lxr->curr_char != '\n' && length < MAXSTRINGL)
		{
			if (!flag_too_long_string)
			{
				lxr->lexstr[length++] = get_next_string_elem(lxr);
			}
			get_char(lxr);
		}
		if (length == MAXSTRINGL)
		{
			lexer_error(lxr, string_too_long);
			flag_too_long_string = 1;
			while (lxr->curr_char != '"' && lxr->curr_char != '\n')
			{
				get_char(lxr);
			}
		}
		if (lxr->curr_char == '"')
		{
			get_char(lxr);
		}
		else
		{
			lexer_error(lxr, missing_terminating_quote_char);
		}
		skip_whitespace(lxr);
	}
	lxr->num = (int)length;
	return string_literal;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


lexer create_lexer(universal_io *const io, syntax *const sx)
{
	lexer lxr;
	lxr.io = io;
	lxr.sx = sx;
	lxr.repr = 0;

	lxr.was_error = 0;

	return lxr;
}

char32_t get_char(lexer *const lxr)
{
	if (lxr == NULL)
	{
		return (char32_t)EOF;
	}

	lxr->curr_char = lxr->next_char;
	lxr->next_char = uni_scan_char(lxr->io);
	return lxr->curr_char;
}

token_t lex(lexer *const lxr)
{
	if (lxr == NULL)
	{
		return eof;
	}

	skip_whitespace(lxr);
	switch (lxr->curr_char)
	{
		case (char32_t)EOF:
			return eof;

		default:
			if (utf8_is_letter(lxr->curr_char) || lxr->curr_char == '#')
			{
				// Keywords [C99 6.4.1]
				// Identifiers [C99 6.4.2]
				return lex_identifier_or_keyword(lxr);
			}
			else
			{
				lexer_error(lxr, bad_character, lxr->curr_char);
				// Pretending the character didn't exist
				get_char(lxr);
				return lex(lxr);
			}

		// Integer Constants [C99 6.4.4.1]
		// Floating Constants [C99 6.4.4.2]
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lex_numeric_constant(lxr);

		// Character Constants [C99 6.4.4.4]
		case '\'':
			return lex_char_constant(lxr);

		// String Literals [C99 6.4.5]
		case '\"':
			return lex_string_literal(lxr);

		// Punctuators [C99 6.4.6]
		case '?':
			get_char(lxr);
			return question;

		case '[':
			get_char(lxr);
			return l_square;

		case ']':
			get_char(lxr);
			return r_square;

		case '(':
			get_char(lxr);
			return l_paren;

		case ')':
			get_char(lxr);
			return r_paren;

		case '{':
			get_char(lxr);
			return l_brace;

		case '}':
			get_char(lxr);
			return r_brace;

		case '~':
			get_char(lxr);
			return tilde;

		case ':':
			get_char(lxr);
			return colon;

		case ';':
			get_char(lxr);
			return semicolon;

		case ',':
			get_char(lxr);
			return comma;

		case '.':
			if (utf8_is_digit(lxr->next_char))
			{
				return lex_numeric_constant(lxr);
			}
			else
			{
				get_char(lxr);
				return period;
			}

		case '*':
			if (get_char(lxr) == '=')
			{
				get_char(lxr);
				return starequal;
			}
			else
			{
				return star;
			}

		case '!':
			if (get_char(lxr) == '=')
			{
				get_char(lxr);
				return exclaimequal;
			}
			else
			{
				return exclaim;
			}

		case '%':
			if (get_char(lxr) == '=')
			{
				get_char(lxr);
				return percentequal;
			}
			else
			{
				return percent;
			}

		case '^':
			if (get_char(lxr) == '=')
			{
				get_char(lxr);
				return caretequal;
			}
			else
			{
				return caret;
			}

		case '=':
			if (get_char(lxr) == '=')
			{
				get_char(lxr);
				return equalequal;
			}
			else
			{
				return equal;
			}

		case '+':
			switch (get_char(lxr))
			{
				case '=':
					get_char(lxr);
					return plusequal;

				case '+':
					get_char(lxr);
					return plusplus;

				default:
					return plus;
			}

		case '|':
			switch (get_char(lxr))
			{
				case '=':
					get_char(lxr);
					return pipeequal;

				case '|':
					get_char(lxr);
					return pipepipe;

				default:
					return pipe;
			}

		case '&':
			switch (get_char(lxr))
			{
				case '=':
					get_char(lxr);
					return ampequal;

				case '&':
					get_char(lxr);
					return ampamp;

				default:
					return amp;
			}

		case '-':
			switch (get_char(lxr))
			{
				case '=':
					get_char(lxr);
					return minusequal;

				case '-':
					get_char(lxr);
					return minusminus;

				case '>':
					get_char(lxr);
					return arrow;

				default:
					return minus;
			}

		case '<':
			switch (get_char(lxr))
			{
				case '<':
					if (get_char(lxr) == '=')
					{
						get_char(lxr);
						return lesslessequal;
					}
					else
					{
						return lessless;
					}

				case '=':
					get_char(lxr);
					return lessequal;

				default:
					return less;
			}

		case '>':
			switch (get_char(lxr))
			{
				case '>':
					if (get_char(lxr) == '=')
					{
						get_char(lxr);
						return greatergreaterequal;
					}
					else
					{
						return greatergreater;
					}

				case '=':
					get_char(lxr);
					return greaterequal;

				default:
					return greater;
			}

		case '/':
			switch (get_char(lxr))
			{
				case '=':
					get_char(lxr);
					return slashequal;

				// Comments [C99 6.4.9]
				case '/':
					skip_line_comment(lxr);
					return lex(lxr);

				case '*':
					skip_block_comment(lxr);
					return lex(lxr);

				default:
					return slash;
			}
	}
}
