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
	TOK_EOF					= LEOF,			/**< End of file */

	// Keywords [C99 6.4.1]
	TOK_MAIN				= LMAIN,		/**< 'main'		keyword */
	TOK_CHAR				= LCHAR,		/**< 'char'		keyword */
	TOK_DOUBLE				= LDOUBLE,		/**< 'double'	keyword */
	TOK_FLOAT				= LFLOAT,		/**< 'float'	keyword */
	TOK_INT					= LINT,			/**< 'int'		keyword	*/
	TOK_LONG				= LLONG,		/**< 'long'		keyword */
	TOK_STRUCT				= LSTRUCT,		/**< 'struct'	keyword */
	TOK_VOID				= LVOID,		/**< 'void'		keyword */
	TOK_IF					= LIF,			/**< 'if'		keyword */
	TOK_ELSE				= LELSE,		/**< 'else'		keyword */
	TOK_DO					= LDO,			/**< 'do'		keyword */
	TOK_WHILE				= LWHILE,		/**< 'while'	keyword */
	TOK_FOR					= LFOR,			/**< 'for'		keyword */
	TOK_SWITCH				= LSWITCH,		/**< 'switch'	keyword */
	TOK_CASE				= LCASE,		/**< 'case'		keyword */
	TOK_DEFAULT				= LDEFAULT,		/**< 'default'	keyword */
	TOK_BREAK				= LBREAK,		/**< 'break'	keyword */
	TOK_CONTINUE			= LCONTINUE,	/**< 'continue'	keyword */
	TOK_GOTO				= LGOTO,		/**< 'goto'		keyword */
	TOK_RETURN				= LRETURN,		/**< 'return'	keyword */

	// Identifiers [C99 6.4.2]
	TOK_IDENTIFIER			= IDENT,		/**< Identifier [C99 6.4.2] */

	// Constants [C99 6.4.4]
	TOK_INT_CONST			= INT_CONST,	/**< Integer Constant [C99 6.4.4.1] */
	TOK_FLOAT_CONST			= FLOAT_CONST,	/**< Floating Constant [C99 6.4.4.2] */
	TOK_CHAR_CONST			= CHAR_CONST,	/**< Character Constant [C99 6.4.4.4] */
	TOK_STRING				= STRING,		/**< String Literal [C99 6.4.5] */

	// Punctuators [C99 6.4.6]
	TOK_QUESTION			= QUEST,		/**< '?'	punctuator */
	TOK_LSQUARE				= LEFTSQBR,		/**< '['	punctuator */
	TOK_RSQUARE				= RIGHTSQBR,	/**< ']'	punctuator */
	TOK_LPAREN				= LEFTBR,		/**< '('	punctuator */
	TOK_RPAREN				= RIGHTBR,		/**< ')'	punctuator */
	TOK_LBRACE				= BEGIN,		/**< '{'	punctuator */
	TOK_RBRACE				= END,			/**< '}'	punctuator */
	TOK_TILDE				= LNOT,			/**< '~'	punctuator */
	TOK_COLON				= COLON,		/**< ':'	punctuator */
	TOK_SEMICOLON			= SEMICOLON,	/**< ';'	punctuator */
	TOK_COMMA				= COMMA,		/**< ','	punctuator */
	TOK_PERIOD				= DOT,			/**< '.'	punctuator */
	TOK_PLUS				= LPLUS,		/**< '+'	punctuator */
	TOK_MINUS				= LMINUS,		/**< '-'	punctuator */
	TOK_STAR				= LMULT,		/**< '*'	punctuator */
	TOK_SLASH				= LDIV,			/**< '/'	punctuator */
	TOK_PERCENT				= LREM,			/**< '%'	punctuator */
	TOK_EXCLAIM				= LOGNOT,		/**< '!'	punctuator */
	TOK_CARET				= LEXOR,		/**< '^'	punctuator */
	TOK_PIPE				= LOR,			/**< '|'	punctuator */
	TOK_AMP					= LAND,			/**< '&'	punctuator */
	TOK_EQUAL				= ASS,			/**< '='	punctuator */
	TOK_LESS				= LLT,			/**< '<'	punctuator */
	TOK_GREATER				= LGT,			/**< '>'	punctuator */
	TOK_ARROW				= ARROW,		/**< '->'	punctuator */
	TOK_PLUSEQUAL			= PLUSASS,		/**< '+='	punctuator */
	TOK_PLUSPLUS			= INC,			/**< '++'	punctuator */
	TOK_MINUSEQUAL			= MINUSASS,		/**< '-='	punctuator */
	TOK_MINUSMINUS			= DEC,			/**< '--'	punctuator */
	TOK_STAREQUAL			= MULTASS,		/**< '*='	punctuator */
	TOK_SLASHEQUAL			= DIVASS,		/**< '/='	punctuator */
	TOK_PERCENTEQUAL		= REMASS,		/**< '%='	punctuator */
	TOK_EXCLAIMEQUAL		= NOTEQ,		/**< '!='	punctuator */
	TOK_CARETEQUAL			= EXORASS,		/**< '^='	punctuator */
	TOK_PIPEEQUAL			= ORASS,		/**< '|='	punctuator */
	TOK_PIPEPIPE			= LOGOR,		/**< '||'	punctuator */
	TOK_AMPEQUAL			= ANDASS,		/**< '&='	punctuator */
	TOK_AMPAMP				= LOGAND,		/**< '&&'	punctuator */
	TOK_EQUALEQUAL			= EQEQ,			/**< '=='	punctuator */
	TOK_LESSEQUAL			= LLE,			/**< '<='	punctuator */
	TOK_LESSLESS			= LSHL,			/**< '<<'	punctuator */
	TOK_GREATEREQUAL		= LGE,			/**< '>='	punctuator */
	TOK_GREATERGREATER		= LSHR,			/**< '>>'	punctuator */
	TOK_LESSLESSEQUAL		= SHLASS,		/**< '<<='	punctuator */
	TOK_GREATERGREATEREQUAL	= SHRASS,		/**< '>>='	punctuator */

	// Standard Functions [RuC]
	TOK_PRINT				= PRINT,		/**< 'print'	keyword	*/
	TOK_PRINTF				= PRINTF,		/**< 'printf'	keyword */
	TOK_PRINTID				= PRINTID,		/**< 'printid'	keyword */
	TOK_SCANF				= SCANF,		/**< 'scanf'	keyword */
	TOK_GETID				= GETID,		/**< 'getid'	keyword */
	TOK_ABS					= ABS,			/**< 'abs'		keyword */
	TOK_SQRT				= SQRT,			/**< 'sqrt'		keyword */
	TOK_EXP					= EXP,			/**< 'exp'		keyword */
	TOK_SIN					= SIN,			/**< 'sin'		keyword */
	TOK_COS					= COS,			/**< 'cos'		keyword */
	TOK_LOG					= LOG,			/**< 'log'		keyword */
	TOK_LOG10				= LOG10,		/**< 'log10'	keyword */
	TOK_ASIN				= ASIN,			/**< 'asin'		keyword */
	TOK_RAND				= RAND,			/**< 'rand'		keyword */
	TOK_ROUND				= ROUND,		/**< 'round'	keyword */
	TOK_STRCPY				= STRCPY,		/**< 'strcpy'	keyword */
	TOK_STRNCPY				= STRNCPY,		/**< 'strncpy'	keyword */
	TOK_STRCAT				= STRCAT,		/**< 'strcat'	keyword */
	TOK_STRNCAT				= STRNCAT,		/**< 'strncat'	keyword */
	TOK_STRCMP				= STRCMP,		/**< 'strcmp'	keyword */
	TOK_STRNCMP				= STRNCMP,		/**< 'strncmp'	keyword */
	TOK_STRSTR				= STRSTR,		/**< 'strstr'	keyword */
	TOK_STRLEN				= STRLEN,		/**< 'strlen'	keyword */
	TOK_ASSERT				= ASSERT,		/**< 'assert'	keyword */
	TOK_UPB					= UPB,			/**< 'upb'		keyword */

	TOK_CREATEDIRECT		= TCREATEDIRECT,
	TOK_EXITDIRECT			= TEXITDIRECT,

	TOK_MSGSEND				= TMSGSEND,		/**< 't_msg_send'		keyword */
	TOK_MSGRECEIVE			= TMSGRECEIVE,	/**< 't_msg_receive'	keyword */
	TOK_JOIN				= TJOIN,		/**< 't_join'			keyword */
	TOK_SLEEP				= TSLEEP,		/**< 't_sleep'			keyword */
	TOK_SEMCREATE			= TSEMCREATE,	/**< 't_sem_create'		keyword */
	TOK_SEMWAIT				= TSEMWAIT,		/**< 't_sem_wait'		keyword */
	TOK_SEMPOST				= TSEMPOST,		/**< 't_sem_post'		keyword */
	TOK_CREATE				= TCREATE,		/**< 't_create'			keyword */
	TOK_INIT				= TINIT,		/**< 't_init'			keyword */
	TOK_DESTROY				= TDESTROY,		/**< 't_destroy'		keyword */
	TOK_EXIT				= TEXIT,		/**< 't_exit'			keyword */
	TOK_GETNUM				= TGETNUM,		/**< 't_getnum'			keyword */

	TOK_SEND_INT			= SEND_INT,			/**< 'send_int_to_robot'			keyword */
	TOK_SEND_FLOAT			= SEND_FLOAT,		/**< 'send_float_to_robot'			keyword */
	TOK_SEND_STRING			= SEND_STRING,		/**< 'send_string_to_robot'			keyword */
	TOK_RECEIVE_INT			= RECEIVE_INT,		/**< 'receive_int_from_robot'		keyword */
	TOK_RECEIVE_FLOAT		= RECEIVE_FLOAT,	/**< 'receive_float_from_robot'		keyword */
	TOK_RECEIVE_STRING		= RECEIVE_STRING,	/**< 'receive_string_from_robot'	keyword */
} token_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
