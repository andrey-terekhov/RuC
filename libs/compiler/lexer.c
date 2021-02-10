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
 *	@param	lexer		Lexer structure
 *	@param	num			Error code
 */
void lexer_error(lexer *const lexer, const int num, ...)
{
	lexer->was_error = 1;

	va_list args;
	va_start(args, num);

	error(lexer->io, num, args);
}

/**
 *	Skip over a series of whitespace characters
 *
 *	@param	lexer		Lexer structure
 *	
 */
void skip_whitespace(lexer *const lexer)
{
	while (lexer->curr_char == ' ' || lexer->curr_char == '\t'
		|| lexer->curr_char == '\n' || lexer->curr_char == '\r')
	{
		get_char(lexer);
	}
}

/**
 *	Skip until we find the newline character that terminates the comment
 *
 *	@param	lexer		Lexer structure
 */
void skip_line_comment(lexer *const lexer)
{
	while (lexer->curr_char != '\n' && lexer->next_char != (char32_t)EOF)
	{
		get_char(lexer);
	}
}

/**
 *	Skip until we find '*' and '/' that terminates the comment
 *
 *	@param	lexer		Lexer structure
 */
void skip_block_comment(lexer *const lexer)
{
	while (lexer->curr_char != '*' && lexer->next_char != '/')
	{
		if (lexer->next_char == (char32_t)EOF)
		{
			lexer_error(lexer, unterminated_block_comment);
			return;
		}
		get_char(lexer);
	}
	get_char(lexer);
}

/**
 *	Lex identifier or keyword [C99 6.4.1 & 6.4.2]
 *
 *	@param	lexer		Lexer structure
 *
 *	@return	keyword number on keyword, @c identifier on identifier
 */
token lex_identifier_or_keyword(lexer *const lexer)
{
	char32_t spelling[MAXSTRINGL];
	size_t length = 0;

	do
	{
		spelling[length++] = lexer->curr_char;
		get_char(lexer);
	} while (utf8_is_letter(lexer->curr_char) || utf8_is_digit(lexer->curr_char));
	spelling[length] = 0;

	const size_t repr = repr_add(lexer->sx, spelling);
	if (repr_get_reference(lexer->sx, repr) < 0)
	{
		return (token)repr_get_reference(lexer->sx, repr);
	}
	else
	{
		lexer->repr = repr;
		return identifier;
	}
}

/**
 *	Lex numeric constant [C99 6.4.4.1 & 6.4.4.2]
 *
 *	@param	lexer		Lexer structure
 *
 *	@return	@c int_constant on integer, @c float_constant on floating point
 */
token lex_numeric_constant(lexer *const lexer)
{
	int num_int = 0;
	double num_double = 0.0;
	int flag_int = 1;
	int flag_too_long = 0;
	
	while (utf8_is_digit(lexer->curr_char))
	{
		num_int = num_int * 10 + (lexer->curr_char - '0');
		num_double = num_double * 10 + (lexer->curr_char - '0');
		get_char(lexer);
	}
	
	if (num_double > (double)INT_MAX)
	{
		flag_too_long = 1;
		flag_int = 0;
	}
	
	if (lexer->curr_char == '.')
	{
		flag_int = 0;
		double position_mult = 0.1;
		while (utf8_is_digit(get_char(lexer)))
		{
			num_double += (lexer->curr_char - '0') * position_mult;
			position_mult *= 0.1;
		}
	}
	
	if (utf8_is_power(lexer->curr_char))
	{
		int power = 0;
		int sign = 1;
		get_char(lexer);
		
		if (lexer->curr_char == '-')
		{
			flag_int = 0;
			get_char(lexer);
			sign = -1;
		}
		else if (lexer->curr_char == '+')
		{
			get_char(lexer);
		}
		
		if (!utf8_is_digit(lexer->curr_char))
		{
			lexer_error(lexer, must_be_digit_after_exp);
			return float_constant;
		}
		
		while (utf8_is_digit(lexer->curr_char))
		{
			power = power * 10 + (lexer->curr_char - '0');
			get_char(lexer);
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
		lexer->num = num_int;
		return int_constant;
	}
	else
	{
		lexer->num_double = num_double;
		if (flag_too_long)
		{
			warning(lexer->io, too_long_int);
		}
		return float_constant;
	}
}

/**	Get character or escape sequence after '\' */
char32_t get_next_string_elem(lexer *const lexer)
{
	if (lexer->curr_char == '\\')
	{
		switch (get_char(lexer))
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
				return lexer->curr_char;

			default:
				lexer_error(lexer, unknown_escape_sequence);
				return lexer->curr_char;
		}
	}
	else
	{
		return lexer->curr_char;
	}
}

/**
 *	Lex character constant [C99 6.4.4.4]
 *
 *	@note Lexes the remainder of a character constant after apostrophe
 *
 *	@param	lexer		Lexer structure
 *
 *	@return	@c char_constant
 */
token lex_char_constant(lexer *const lexer)
{
	if (get_char(lexer) == '\'')
	{
		lexer_error(lexer, empty_character);
		lexer->num = 0;
		return char_constant;
	}
	
	lexer->num = get_next_string_elem(lexer);
	
	if (get_char(lexer) == '\'')
	{
		get_char(lexer);
	}
	else
	{
		lexer_error(lexer, expected_apost_after_char_const);
	}
	return char_constant;
}

/**
 *	Lex string literal [C99 6.4.5]
 *
 *	@note	Lexes the remainder of a string literal after quote mark
 *
 *	@param	lexer		Lexer structure
 *
 *	@return	@c string_literal
 */
token lex_string_literal(lexer *const lexer)
{
	size_t length = 0;
	int flag_too_long_string = 0;
	while (lexer->curr_char == '\"')
	{
		get_char(lexer);
		while (lexer->curr_char != '"' && lexer->curr_char != '\n' && length < MAXSTRINGL)
		{
			if (!flag_too_long_string)
			{
				lexer->lexstr[length++] = get_next_string_elem(lexer);
			}
			get_char(lexer);
		}
		if (length == MAXSTRINGL)
		{
			lexer_error(lexer, string_too_long);
			flag_too_long_string = 1;
			while (lexer->curr_char != '"' && lexer->curr_char != '\n')
			{
				get_char(lexer);
			}
		}
		if (lexer->curr_char == '"')
		{
			get_char(lexer);
		}
		else
		{
			lexer_error(lexer, missing_terminating_quote_char);
		}
		skip_whitespace(lexer);
	}
	lexer->num = (int)length;
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
	lexer lexer;
	lexer.io = io;
	lexer.sx = sx;
	lexer.repr = 0;

	lexer.was_error = 0;
	
	return lexer;
}

char32_t get_char(lexer *const lexer)
{
	if (lexer == NULL)
	{
		return (char32_t)EOF;
	}

	lexer->curr_char = lexer->next_char;
	lexer->next_char = uni_scan_char(lexer->io);
	return lexer->curr_char;
}

token lex(lexer *const lexer)
{
	if (lexer == NULL)
	{
		return eof;
	}
	
	skip_whitespace(lexer);
	switch (lexer->curr_char)
	{
		case (char32_t)EOF:
			return eof;

		default:
			if (utf8_is_letter(lexer->curr_char) || lexer->curr_char == '#')
			{
				// Keywords [C99 6.4.1]
				// Identifiers [C99 6.4.2]
				return lex_identifier_or_keyword(lexer);
			}
			else
			{
				lexer_error(lexer, bad_character, lexer->curr_char);
				// Pretending the character didn't exist
				get_char(lexer);
				return lex(lexer);
			}

		// Integer Constants [C99 6.4.4.1]
		// Floating Constants [C99 6.4.4.2]
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lex_numeric_constant(lexer);

		// Character Constants [C99 6.4.4.4]
		case '\'':
			return lex_char_constant(lexer);

		// String Literals [C99 6.4.5]
		case '\"':
			return lex_string_literal(lexer);

		// Punctuators [C99 6.4.6]
		case '?':
			get_char(lexer);
			return question;

		case '[':
			get_char(lexer);
			return l_square;

		case ']':
			get_char(lexer);
			return r_square;

		case '(':
			get_char(lexer);
			return l_paren;

		case ')':
			get_char(lexer);
			return r_paren;

		case '{':
			get_char(lexer);
			return l_brace;

		case '}':
			get_char(lexer);
			return r_brace;

		case '~':
			get_char(lexer);
			return tilde;

		case ':':
			get_char(lexer);
			return colon;

		case ';':
			get_char(lexer);
			return semicolon;

		case ',':
			get_char(lexer);
			return comma;

		case '.':
			if (utf8_is_digit(lexer->next_char))
			{
				return lex_numeric_constant(lexer);
			}
			else
			{
				get_char(lexer);
				return period;
			}

		case '*':
			if (get_char(lexer) == '=')
			{
				get_char(lexer);
				return starequal;
			}
			else
			{
				return star;
			}

		case '!':
			if (get_char(lexer) == '=')
			{
				get_char(lexer);
				return exclaimequal;
			}
			else
			{
				return exclaim;
			}

		case '%':
			if (get_char(lexer) == '=')
			{
				get_char(lexer);
				return percentequal;
			}
			else
			{
				return percent;
			}

		case '^':
			if (get_char(lexer) == '=')
			{
				get_char(lexer);
				return caretequal;
			}
			else
			{
				return caret;
			}

		case '=':
			if (get_char(lexer) == '=')
			{
				get_char(lexer);
				return equalequal;
			}
			else
			{
				return equal;
			}

		case '+':
			switch (get_char(lexer))
			{
				case '=':
					get_char(lexer);
					return plusequal;

				case '+':
					get_char(lexer);
					return plusplus;

				default:
					return plus;
			}

		case '|':
			switch (get_char(lexer))
			{
				case '=':
					get_char(lexer);
					return pipeequal;

				case '|':
					get_char(lexer);
					return pipepipe;

				default:
					return pipe;
			}

		case '&':
			switch (get_char(lexer))
			{
				case '=':
					get_char(lexer);
					return ampequal;

				case '&':
					get_char(lexer);
					return ampamp;

				default:
					return amp;
			}

		case '-':
			switch (get_char(lexer))
			{
				case '=':
					get_char(lexer);
					return minusequal;

				case '-':
					get_char(lexer);
					return minusminus;

				case '>':
					get_char(lexer);
					return arrow;

				default:
					return minus;
			}

		case '<':
			switch (get_char(lexer))
			{
				case '<':
					if (get_char(lexer) == '=')
					{
						get_char(lexer);
						return lesslessequal;
					}
					else
					{
						return lessless;
					}

				case '=':
					get_char(lexer);
					return lessequal;

				default:
					return less;
			}

		case '>':
			switch (get_char(lexer))
			{
				case '>':
					if (get_char(lexer) == '=')
					{
						get_char(lexer);
						return greatergreaterequal;
					}
					else
					{
						return greatergreater;
					}

				case '=':
					get_char(lexer);
					return greaterequal;

				default:
					return greater;
			}

		case '/':
			switch (get_char(lexer))
			{
				case '=':
					get_char(lexer);
					return slashequal;

				// Comments [C99 6.4.9]
				case '/':
					skip_line_comment(lexer);
					return lex(lexer);

				case '*':
					skip_block_comment(lexer);
					return lex(lexer);

				default:
					return slash;
			}
	}
}
