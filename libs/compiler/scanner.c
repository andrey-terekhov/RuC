/*
 *	Copyright 2014 Andrey Terekhov
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

#include "analyzer.h"
#include "errors.h"
#include "defs.h"
#include "uniscanner.h"
#include "utf8.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


char32_t getnext(analyzer *const context)
{
	char32_t ret = uni_scan_char(context->io);
	context->nextchar = ret;
	return ret;
}

char32_t get_char(analyzer *const context)
{
	context->curchar = context->nextchar;
	context->nextchar = uni_scan_char(context->io);
	return context->curchar;
}

void onemore(analyzer *context)
{
	context->curchar = context->nextchar;
	context->nextchar = getnext(context);
	if (0)
	{
		// context->kw)
		printf("context->curchar=%c %i context->nextchar=%c %i\n", context->curchar, context->curchar,
			   context->nextchar, context->nextchar);
	}
}

/*void endofline(analyzer *context)
{
	if (context->prep_flag == 1)
	{
		int j;
		uni_printf(context->io, "line %i) ", context->line - 1);
		for (j = context->lines[context->line - 1]; j < context->lines[context->line]; j++)
		{
			if (context->source[j] != EOF)
			{
				uni_print_char(context->io, context->source[j]);
			}
		}
	}
	// fflush(stdout);
}*/

void endnl(analyzer *context)
{
	if (context->kw)
	{
		++context->line;
		context->charnum_before = context->charnum;
		context->charnum = 0;
	}
	/*context->lines[++context->line] = context->charnum;
	context->lines[context->line + 1] = context->charnum;
	if (context->kw)
	{
		endofline(context);
	}*/
}

void nextch(analyzer *context)
{
	onemore(context);
	if (context->curchar == (char32_t) EOF)
	{
		onemore(context);
		endnl(context);
		// uni_printf(context->io, "\n");
		return;
	}
	if (context->kw)
	{
		context->last_line[context->charnum++] = context->curchar;
	}
	if (context->instring)
	{
		return;
	}

	if (context->curchar == '/' && context->nextchar == '/')
	{
		do
		{
			onemore(context);
			if (context->curchar == (char32_t) EOF)
			{
				endnl(context);
				// uni_printf(context->io, "\n");
				return;
			}
		} while (context->curchar != '\n');
		endnl(context);
		return;
	}
	
	if (context->kw && context->curchar == '\n')
	{
		context->temp_tc = context->sx->tc;
		endnl(context);
	}
	return;
}

int ispower(const char32_t symbol)
{
	return symbol == 'e' || symbol == 'E' || symbol == U'е' || symbol == U'Е';
}


/**
 *	Skip over a series of whitespace characters
 *
 *	@param	context	Analyzer structure
 */
void skip_whitespace(analyzer *const context)
{
	while (context->curchar == ' ' || context->curchar == '\t' || context->curchar == '\n')
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


int repr_is_equal(analyzer *const context, size_t i, size_t j)
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
	size_t r;
	size_t hash = 0;
	REPRTAB_LEN += 2;
	
	do
	{
		hash += context->curchar;
		REPRTAB[REPRTAB_LEN++] = context->curchar;
		nextch(context);
	} while (utf8_is_letter(context->curchar) || utf8_is_digit(context->curchar));
	
	context->hash = (hash &= 255);
	REPRTAB[REPRTAB_LEN++] = 0;
	r = context->hashtab[hash];
	
	if (r)
	{
		do
		{
			if (repr_is_equal(context, r, old_repr))
			{
				REPRTAB_LEN = old_repr;
				if (REPRTAB[r + 1] < 0)
				{
					return REPRTAB[r + 1];
				}
				else
				{
					REPRTAB_POS = r;
					return IDENT;
				}
			}
			else
			{
				r = REPRTAB[r];
			}
		} while (r);
	}
	
	REPRTAB[old_repr] = context->hashtab[hash];
	REPRTAB_POS = context->hashtab[hash] = old_repr;
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
	double k;
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
		k = 0.1;
		while (utf8_is_digit(get_char(context)))
		{
			num_double += (context->curchar - '0') * k;
			k *= 0.1;
		}
	}
	
	if (ispower(context->curchar))
	{
		int d = 0;
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
			d = d * 10 + (context->curchar - '0');
			get_char(context);
		}
		
		if (flag_int)
		{
			for (int i = 1; i <= d; i++)
			{
				context->num *= 10;
			}
		}
		num_double *= pow(10.0, sign * d);
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
	get_char(context);
	while (1)
	{
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
		if (context->curchar == '\"')
		{
			get_char(context);
		}
		else
		{
			break;
		}
	}
	context->num = length;
	return STRING;
}


int scan(analyzer *context)
{
	int cr;
	context->new_line_flag = 0;
	while (context->curchar == ' ' || context->curchar == '\t' || context->curchar == '\n')
	{
		if (context->curchar == '\n')
		{
			context->new_line_flag = 1;
		}
		nextch(context);
	}
	// printf("scan context->curchar = %c %i\n", context->curchar, context->curchar);
	switch (context->curchar)
	{
		case EOF:
		{
			return LEOF;
		}
		case ',':
		{
			nextch(context);
			return COMMA;
		}

		case '=':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = EQEQ;
			}
			else
			{
				cr = ASS;
			}
			return cr;

		case '*':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = MULTASS;
			}
			else
			{
				cr = LMULT;
			}
			return cr;

		case '/':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = DIVASS;
			}
			else
			{
				cr = LDIV;
			}
			return cr;

		case '%':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = REMASS;
			}
			else
			{
				cr = LREM;
			}
			return cr;

		case '+':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = PLUSASS;
			}
			else if (context->curchar == '+')
			{
				nextch(context);
				cr = INC;
			}
			else
			{
				cr = LPLUS;
			}
			return cr;

		case '-':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = MINUSASS;
			}
			else if (context->curchar == '-')
			{
				nextch(context);
				cr = DEC;
			}
			else if (context->curchar == '>')
			{
				nextch(context);
				cr = ARROW;
			}
			else
			{
				cr = LMINUS;
			}
			return cr;

		case '<':
			nextch(context);
			if (context->curchar == '<')
			{
				nextch(context);
				if (context->curchar == '=')
				{
					nextch(context);
					cr = SHLASS;
				}
				else
				{
					cr = LSHL;
				}
			}
			else if (context->curchar == '=')
			{
				nextch(context);
				cr = LLE;
			}
			else
			{
				cr = LLT;
			}

			return cr;

		case '>':
			nextch(context);
			if (context->curchar == '>')
			{
				nextch(context);
				if (context->curchar == '=')
				{
					nextch(context);
					cr = SHRASS;
				}
				else
				{
					cr = LSHR;
				}
			}
			else if (context->curchar == '=')
			{
				nextch(context);
				cr = LGE;
			}
			else
			{
				cr = LGT;
			}
			return cr;

		case '&':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = ANDASS;
			}
			else if (context->curchar == '&')
			{
				nextch(context);
				cr = LOGAND;
			}
			else
			{
				cr = LAND;
			}
			return cr;

		case '^':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = EXORASS;
			}
			else
			{
				cr = LEXOR;
			}
			return cr;

		case '|':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = ORASS;
			}
			else if (context->curchar == '|')
			{
				nextch(context);
				cr = LOGOR;
			}
			else
			{
				cr = LOR;
			}
			return cr;

		case '!':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = NOTEQ;
			}
			else
			{
				cr = LOGNOT;
			}
			return cr;
			
		// Character Constants [C99 6.4.4.4]
		case '\'':
			return lex_char_constant(context);
			
		// String Literals [C99 6.4.5]
		case '\"':
			return lex_string_literal(context);
			
		case '(':
		{
			nextch(context);
			return LEFTBR;
		}

		case ')':
		{
			nextch(context);
			return RIGHTBR;
		}

		case '[':
		{
			nextch(context);
			return LEFTSQBR;
		}

		case ']':
		{
			nextch(context);
			return RIGHTSQBR;
		}

		case '~':
		{
			nextch(context);
			return LNOT; // поразрядное отрицание
		}
		case '{':
		{
			nextch(context);
			return BEGIN;
		}
		case '}':
		{
			nextch(context);
			return END;
		}
		case ';':
		{
			nextch(context);
			return SEMICOLON;
		}
		case '?':
		{
			nextch(context);
			return QUEST;
		}
		case ':':
		{
			nextch(context);
			return COLON;
		}
		case '.':
			if (utf8_is_digit(context->nextchar))
			{
				return lex_numeric_constant(context);
			}
			else
			{
				nextch(context);
				return DOT;
			}

		// Integer Constants [C99 6.4.4.1]
		// Floating Constants [C99 6.4.4.2]
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lex_numeric_constant(context);

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
				nextch(context);
				return scan(context);
			}
	}
}

int scaner(analyzer *context)
{
	context->cur = context->next;
	if (!context->buf_flag)
	{
		context->next = scan(context);
	}
	else
	{
		context->next = context->buf_cur;
		context->buf_flag--;
	}

	//	 if(context->kw)
	//			printf("scaner context->cur %i context->next %i buf_flag %i\n",
	//			context->cur, context->next, context->buf_flag);
	return context->cur;
}
