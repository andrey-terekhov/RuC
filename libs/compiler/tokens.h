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

#pragma once

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum TOKEN
{
	TOK_EOF		= -1,				/**< End of file */

	// Keywords [C99 6.4.1]
	TOK_MAIN	= 0,				/**< 'main'		keyword */
	TOK_CHAR	= -200,				/**< 'char'		keyword */
	TOK_DOUBLE,						/**< 'double'	keyword */
	TOK_FLOAT,						/**< 'float'	keyword */
	TOK_INT,						/**< 'int'		keyword	*/
	TOK_LONG,						/**< 'long'		keyword */
	TOK_STRUCT,						/**< 'struct'	keyword */
	TOK_VOID,						/**< 'void'		keyword */
	TOK_IF,							/**< 'if'		keyword */
	TOK_ELSE,						/**< 'else'		keyword */
	TOK_DO,							/**< 'do'		keyword */
	TOK_WHILE,						/**< 'while'	keyword */
	TOK_FOR,						/**< 'for'		keyword */
	TOK_SWITCH,						/**< 'switch'	keyword */
	TOK_CASE,						/**< 'case'		keyword */
	TOK_DEFAULT,					/**< 'default'	keyword */
	TOK_BREAK,						/**< 'break'	keyword */
	TOK_CONTINUE,					/**< 'continue'	keyword */
	TOK_GOTO,						/**< 'goto'		keyword */
	TOK_RETURN,						/**< 'return'	keyword */

	// Identifiers [C99 6.4.2]
	TOK_IDENTIFIER,					/**< Identifier [C99 6.4.2] */

	// Constants [C99 6.4.4]
	TOK_INT_CONST,					/**< Integer Constant [C99 6.4.4.1] */
	TOK_FLOAT_CONST,				/**< Floating Constant [C99 6.4.4.2] */
	TOK_CHAR_CONST,					/**< Character Constant [C99 6.4.4.4] */
	TOK_STRING,						/**< String Literal [C99 6.4.5] */

	// Punctuators [C99 6.4.6]
	TOK_LSQUARE,					/**< '['	punctuator */
	TOK_LPAREN,						/**< '('	punctuator */
	TOK_LBRACE,						/**< '{'	punctuator */
	TOK_RSQUARE		= 0b00000001,	/**< ']'	punctuator */
	TOK_RPAREN		= 0b00000010,	/**< ')'	punctuator */
	TOK_RBRACE		= 0b00000100,	/**< '}'	punctuator */
	TOK_COMMA		= 0b00001000,	/**< ','	punctuator */
	TOK_COLON		= 0b00010000,	/**< ':'	punctuator */
	TOK_SEMICOLON	= 0b00100000,	/**< ';'	punctuator */
	TOK_QUESTION 	= -100,			/**< '?'	punctuator */
	TOK_TILDE,						/**< '~'	punctuator */
	TOK_PERIOD,						/**< '.'	punctuator */
	TOK_PLUS,						/**< '+'	punctuator */
	TOK_MINUS,						/**< '-'	punctuator */
	TOK_STAR,						/**< '*'	punctuator */
	TOK_SLASH,						/**< '/'	punctuator */
	TOK_PERCENT,					/**< '%'	punctuator */
	TOK_EXCLAIM,					/**< '!'	punctuator */
	TOK_CARET,						/**< '^'	punctuator */
	TOK_PIPE,						/**< '|'	punctuator */
	TOK_AMP,						/**< '&'	punctuator */
	TOK_EQUAL,						/**< '='	punctuator */
	TOK_LESS,						/**< '<'	punctuator */
	TOK_GREATER,					/**< '>'	punctuator */
	TOK_ARROW,						/**< '->'	punctuator */
	TOK_PLUSEQUAL,					/**< '+='	punctuator */
	TOK_PLUSPLUS,					/**< '++'	punctuator */
	TOK_MINUSEQUAL,					/**< '-='	punctuator */
	TOK_MINUSMINUS,					/**< '--'	punctuator */
	TOK_STAREQUAL,					/**< '*='	punctuator */
	TOK_SLASHEQUAL,					/**< '/='	punctuator */
	TOK_PERCENTEQUAL,				/**< '%='	punctuator */
	TOK_EXCLAIMEQUAL,				/**< '!='	punctuator */
	TOK_CARETEQUAL,					/**< '^='	punctuator */
	TOK_PIPEEQUAL,					/**< '|='	punctuator */
	TOK_PIPEPIPE,					/**< '||'	punctuator */
	TOK_AMPEQUAL,					/**< '&='	punctuator */
	TOK_AMPAMP,						/**< '&&'	punctuator */
	TOK_EQUALEQUAL,					/**< '=='	punctuator */
	TOK_LESSEQUAL,					/**< '<='	punctuator */
	TOK_LESSLESS,					/**< '<<'	punctuator */
	TOK_GREATEREQUAL,				/**< '>='	punctuator */
	TOK_GREATERGREATER,				/**< '>>'	punctuator */
	TOK_LESSLESSEQUAL,				/**< '<<='	punctuator */
	TOK_GREATERGREATEREQUAL,		/**< '>>='	punctuator */

	// Standard Functions [RuC]
#define STANDARD_FUNC_START  TOK_PRINT
	TOK_PRINT,						/**< 'print'	keyword	*/
	TOK_PRINTF,						/**< 'printf'	keyword */
	TOK_PRINTID,					/**< 'printid'	keyword */
	TOK_SCANF,						/**< 'scanf'	keyword */
	TOK_GETID,						/**< 'getid'	keyword */
	TOK_ABS,						/**< 'abs'		keyword */
	TOK_SQRT,						/**< 'sqrt'		keyword */
	TOK_EXP,						/**< 'exp'		keyword */
	TOK_SIN,						/**< 'sin'		keyword */
	TOK_COS,						/**< 'cos'		keyword */
	TOK_LOG,						/**< 'log'		keyword */
	TOK_LOG10,						/**< 'log10'	keyword */
	TOK_ASIN,						/**< 'asin'		keyword */
	TOK_RAND,						/**< 'rand'		keyword */
	TOK_ROUND,						/**< 'round'	keyword */
	TOK_STRCPY,						/**< 'strcpy'	keyword */
	TOK_STRNCPY,					/**< 'strncpy'	keyword */
	TOK_STRCAT,						/**< 'strcat'	keyword */
	TOK_STRNCAT,					/**< 'strncat'	keyword */
	TOK_STRCMP,						/**< 'strcmp'	keyword */
	TOK_STRNCMP,					/**< 'strncmp'	keyword */
	TOK_STRSTR,						/**< 'strstr'	keyword */
	TOK_STRLEN,						/**< 'strlen'	keyword */
	TOK_ASSERT,						/**< 'assert'	keyword */
	TOK_UPB,						/**< 'upb'		keyword */

	TOK_CREATEDIRECT,				/**< 't_create_direct'	keyword */
	TOK_EXITDIRECT,					/**< 't_exit_direct'	keyword */

	TOK_MSG_SEND,					/**< 't_msg_send'		keyword */
	TOK_MSG_RECEIVE,				/**< 't_msg_receive'	keyword */
	TOK_JOIN,						/**< 't_join'			keyword */
	TOK_SLEEP,						/**< 't_sleep'			keyword */
	TOK_SEMCREATE,					/**< 't_sem_create'		keyword */
	TOK_SEMWAIT,					/**< 't_sem_wait'		keyword */
	TOK_SEMPOST,					/**< 't_sem_post'		keyword */
	TOK_CREATE,						/**< 't_create'			keyword */
	TOK_INIT,						/**< 't_init'			keyword */
	TOK_DESTROY,					/**< 't_destroy'		keyword */
	TOK_EXIT,						/**< 't_exit'			keyword */
	TOK_GETNUM,						/**< 't_getnum'			keyword */

	TOK_SEND_INT,					/**< 'send_int_to_robot'			keyword */
	TOK_SEND_FLOAT,					/**< 'send_float_to_robot'			keyword */
	TOK_SEND_STRING,				/**< 'send_string_to_robot'			keyword */
	TOK_RECEIVE_INT,				/**< 'receive_int_from_robot'		keyword */
	TOK_RECEIVE_FLOAT,				/**< 'receive_float_from_robot'		keyword */
	TOK_RECEIVE_STRING,				/**< 'receive_string_from_robot'	keyword */
#define STANDARD_FUNC_END TOK_RECEIVE_STRING
} token_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
