/*
 *	Copyright 2014 Andrey Terekhov, Ilya Andreev
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

#include "scanner.h"
#include "analyzer.h"
#include "errors.h"
#include "uniscanner.h"
#include <math.h>
#include <string.h>


/**
 *	Skip over a series of whitespace characters
 *
 *	@param	context	Analyzer structure
 */
void skip_whitespace(analyzer *const context)
{
	while (context->curchar == ' ' || context->curchar == '\t'
		|| context->curchar == '\n' || context->curchar == '\r')
	{
		get_char(context);
	}
}

/**
 *	Skip until we find the newline character that terminates the comment
 *
 *	@param	context	Analyzer structure
 */
void skip_line_comment(analyzer *const context)
{
	while (context->curchar != '\n' && context->nextchar != (char32_t)EOF)
	{
		get_char(context);
	}
}

/**
 *	Skip until we find '*' and '/' that terminates the comment
 *
 *	@param	context	Analyzer structure
 */
void skip_block_comment(analyzer *const context)
{
	while (context->curchar != '*' && context->nextchar != '/')
	{
		if (context->nextchar == (char32_t)EOF)
		{
			error(context->io, unterminated_block_comment);
			context->error_flag = 1;
			return;
		}
		get_char(context);
	}
	get_char(context);
}


int repr_is_equal(const analyzer *const context, size_t i, size_t j)
{
	i += 2;
	j += 2;
	/* Assuming allocated */
	while (REPRTAB[i] == REPRTAB[j])
	{
		i++;
		j++;
		if (REPRTAB[i] == 0 && REPRTAB[j] == 0)
		{
			return 1;
		}
	}
	return 0;
}

/**
 *	Lex identifier or keyword [C99 6.4.1 & 6.4.2]
 *
 *	@param	context	Analyzer structure
 *
 *	@return	keyword number on keyword, @c IDENT on identifier
 */
int lex_identifier_or_keyword(analyzer *const context)
{
	size_t old_repr = REPRTAB_LEN;
	size_t hash = 0;
	REPRTAB_LEN += 2;
	
	do
	{
		hash += context->curchar;
		REPRTAB[REPRTAB_LEN++] = context->curchar;
		get_char(context);
	} while (utf8_is_letter(context->curchar) || utf8_is_digit(context->curchar));
	
	hash &= 255;
	context->hash = hash;
	REPRTAB[REPRTAB_LEN++] = 0;
	
	size_t cur_repr = context->hashtab[hash];
	if (cur_repr)
	{
		do
		{
			if (repr_is_equal(context, cur_repr, old_repr))
			{
				REPRTAB_LEN = (int)old_repr;
				if (REPRTAB[cur_repr + 1] < 0)
				{
					return REPRTAB[cur_repr + 1];
				}
				else
				{
					REPRTAB_POS = (int)cur_repr;
					return IDENT;
				}
			}
			else
			{
				cur_repr = REPRTAB[cur_repr];
			}
		} while (cur_repr);
	}
	
	REPRTAB[old_repr] = (int)context->hashtab[hash];
	REPRTAB_POS = (int)old_repr;
	context->hashtab[hash] = old_repr;
	// 0 - только MAIN, (< 0) - ключевые слова, 1 - обычные иденты
	REPRTAB[REPRTAB_POS + 1] = (context->keywordsnum) ? -((++context->keywordsnum - 2) / 4) : 1;
	return IDENT;
}


/**
 *	Lex numeric constant [C99 6.4.4.1 & 6.4.4.2]
 *
 *	@param	context	Analyzer structure
 *
 *	@return	@c NUMBER
 */
int lex_numeric_constant(analyzer *const context)
{
	int num_int = 0;
	double num_double = 0.0;
	int flag_int = 1;
	int flag_too_long = 0;
	
	while (utf8_is_digit(context->curchar))
	{
		num_int = num_int * 10 + (context->curchar - '0');
		num_double = num_double * 10 + (context->curchar - '0');
		get_char(context);
	}
	
	if (num_double > (double)INT_MAX)
	{
		flag_too_long = 1;
		flag_int = 0;
	}
	
	if (context->curchar == '.')
	{
		flag_int = 0;
		double position_mult = 0.1;
		while (utf8_is_digit(get_char(context)))
		{
			num_double += (context->curchar - '0') * position_mult;
			position_mult *= 0.1;
		}
	}
	
	if (utf8_is_power(context->curchar))
	{
		int power = 0;
		int sign = 1;
		get_char(context);
		
		if (context->curchar == '-')
		{
			flag_int = 0;
			get_char(context);
			sign = -1;
		}
		else if (context->curchar == '+')
		{
			get_char(context);
		}
		
		if (!utf8_is_digit(context->curchar))
		{
			error(context->io, must_be_digit_after_exp);
			context->error_flag = 1;
			return NUMBER;
		}
		
		while (utf8_is_digit(context->curchar))
		{
			power = power * 10 + (context->curchar - '0');
			get_char(context);
		}
		
		if (flag_int)
		{
			for (int i = 1; i <= power; i++)
			{
				context->num *= 10;
			}
		}
		num_double *= pow(10.0, sign * power);
	}
	
	if (flag_int)
	{
		context->ansttype = LINT;
		context->num = num_int;
	}
	else
	{
		context->ansttype = LFLOAT;
		memcpy(&context->numr, &num_double, sizeof(double));
		if (flag_too_long)
		{
			warning(context->io, too_long_int);
		}
	}
	return NUMBER;
}


/**	Get character or escape sequence after '\' */
char32_t get_next_string_elem(analyzer *const context)
{
	if (context->curchar == '\\')
	{
		switch (get_char(context))
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
				return context->curchar;

			default:
				error(context->io, unknown_escape_sequence);
				context->error_flag = 1;
				return context->curchar;
		}
	}
	else
	{
		return context->curchar;
	}
}

/**
 *	Lex character constant [C99 6.4.4.4]
 *
 *	@note Lexes the remainder of a character constant after apostrophe
 *
 *	@param	context	Analyzer structure
 *
 *	@return	@c NUMBER
 */
int lex_char_constant(analyzer *const context)
{
	context->ansttype = LCHAR;
	if (get_char(context) == '\'')
	{
		error(context->io, empty_char_const);
		context->error_flag = 1;
		context->num = 0;
		return NUMBER;
	}
	
	context->num = get_next_string_elem(context);
	
	if (get_char(context) == '\'')
	{
		get_char(context);
	}
	else
	{
		error(context->io, expected_apost_after_char_const);
		context->error_flag = 1;
	}
	return NUMBER;
}

/**
 *	Lex string literal [C99 6.4.5]
 *
 *	@note	Lexes the remainder of a string literal after quote mark
 *
 *	@param	context	Analyzer structure
 *
 *	@return	@c STRING
 */
int lex_string_literal(analyzer *const context)
{
	size_t length = 0;
	int flag_too_long_string = 0;
	while (context->curchar == '\"')
	{
		get_char(context);
		while (context->curchar != '"' && context->curchar != '\n' && length < MAXSTRINGL)
		{
			if (!flag_too_long_string)
			{
				context->lexstr[length++] = get_next_string_elem(context);
			}
			get_char(context);
		}
		if (length == MAXSTRINGL)
		{
			error(context->io, too_long_string);
			context->error_flag = 1;
			flag_too_long_string = 1;
			while (context->curchar != '"' && context->curchar != '\n')
			{
				get_char(context);
			}
		}
		if (context->curchar == '"')
		{
			get_char(context);
		}
		else
		{
			error(context->io, missing_terminating_quote_char);
			context->error_flag = 1;
		}
		skip_whitespace(context);
	}
	context->num = (int)length;
	return STRING;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


char32_t get_char(analyzer *const context)
{
	if (context == NULL)
	{
		return (char32_t)EOF;
	}
	context->curchar = context->nextchar;
	context->nextchar = uni_scan_char(context->io);
	return context->curchar;
}


int lex(analyzer *const context)
{
	if (context == NULL)
	{
		return LEOF;
	}
	skip_whitespace(context);
	switch (context->curchar)
	{
		case EOF:
			return LEOF;

		default:
			if (utf8_is_letter(context->curchar) || context->curchar == '#')
			{
				// Keywords [C99 6.4.1]
				// Identifiers [C99 6.4.2]
				return lex_identifier_or_keyword(context);
			}
			else
			{
				error(context->io, bad_character, context->curchar);
				context->error_flag = 1;
				// Притворимся, что плохого символа не было
				get_char(context);
				return lex(context);
			}

		// Integer Constants [C99 6.4.4.1]
		// Floating Constants [C99 6.4.4.2]
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lex_numeric_constant(context);

		// Character Constants [C99 6.4.4.4]
		case '\'':
			return lex_char_constant(context);

		// String Literals [C99 6.4.5]
		case '\"':
			return lex_string_literal(context);

		// Punctuators [C99 6.4.6]
		case '?':
			get_char(context);
			return QUEST;

		case '[':
			get_char(context);
			return LEFTSQBR;

		case ']':
			get_char(context);
			return RIGHTSQBR;

		case '(':
			get_char(context);
			return LEFTBR;

		case ')':
			get_char(context);
			return RIGHTBR;

		case '{':
			get_char(context);
			return BEGIN;

		case '}':
			get_char(context);
			return END;

		case '~':
			get_char(context);
			return LNOT;

		case ':':
			get_char(context);
			return COLON;

		case ';':
			get_char(context);
			return SEMICOLON;

		case ',':
			get_char(context);
			return COMMA;

		case '.':
			if (utf8_is_digit(context->nextchar))
			{
				lex_numeric_constant(context);
				return NUMBER;
			}
			else
			{
				get_char(context);
				return DOT;
			}

		case '*':
			if (get_char(context) == '=')
			{
				get_char(context);
				return MULTASS;
			}
			else
			{
				return LMULT;
			}

		case '!':
			if (get_char(context) == '=')
			{
				get_char(context);
				return NOTEQ;
			}
			else
			{
				return LOGNOT;
			}

		case '%':
			if (get_char(context) == '=')
			{
				get_char(context);
				return REMASS;
			}
			else
			{
				return LREM;
			}

		case '^':
			if (get_char(context) == '=')
			{
				get_char(context);
				return EXORASS;
			}
			else
			{
				return LEXOR;
			}

		case '=':
			if (get_char(context) == '=')
			{
				get_char(context);
				return EQEQ;
			}
			else
			{
				return ASS;
			}

		case '+':
			switch (get_char(context))
			{
				case '=':
					get_char(context);
					return PLUSASS;

				case '+':
					get_char(context);
					return INC;

				default:
					return LPLUS;
			}

		case '|':
			switch (get_char(context))
			{
				case '=':
					get_char(context);
					return ORASS;

				case '|':
					get_char(context);
					return LOGOR;

				default:
					return LOR;
			}

		case '&':
			switch (get_char(context))
			{
				case '=':
					get_char(context);
					return ANDASS;

				case '&':
					get_char(context);
					return LOGAND;

				default:
					return LAND;
			}

		case '-':
			switch (get_char(context))
			{
				case '=':
					get_char(context);
					return MINUSASS;

				case '-':
					get_char(context);
					return DEC;

				case '>':
					get_char(context);
					return ARROW;

				default:
					return LMINUS;
			}

		case '<':
			switch (get_char(context))
			{
				case '<':
					if (get_char(context) == '=')
					{
						get_char(context);
						return SHLASS;
					}
					else
					{
						return LSHL;
					}

				case '=':
					get_char(context);
					return LLE;

				default:
					return LLT;
			}

		case '>':
			switch (get_char(context))
			{
				case '>':
					if (get_char(context) == '=')
					{
						get_char(context);
						return SHRASS;
					}
					else
					{
						return LSHR;
					}

				case '=':
					get_char(context);
					return LGE;

				default:
					return LGT;
			}

		case '/':
			switch (get_char(context))
			{
				case '=':
					get_char(context);
					return DIVASS;

				case '+':
					get_char(context);
					return INC;

				// Comments [C99 6.4.9]
				case '/':
					skip_line_comment(context);
					return lex(context);

				case '*':
					skip_block_comment(context);
					return lex(context);

				default:
					return LDIV;
			}
	}
}
