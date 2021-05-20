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
	eof					= LEOF,			/**< End of file */

	// Keywords [C99 6.4.1]
	kw_main				= LMAIN,		/**< 'main'		keyword */
	//kw_bool,							/**< 'bool'		keyword */
	//kw_auto,							/**< 'auto'		keyword */
	kw_char				= LCHAR,		/**< 'char'		keyword */
	kw_double			= LDOUBLE,		/**< 'double'	keyword */
	//kw_enum,							/**< 'enum'		keyword */
	kw_float			= LFLOAT,		/**< 'float'	keyword */
	kw_int				= LINT,			/**< 'int'		keyword	*/
	kw_long				= LLONG,		/**< 'long'		keyword */
	//kw_short,							/**< 'short'	keyword */
	kw_struct			= LSTRUCT,		/**< 'struct'	keyword */
	//kw_union,							/**< 'union'	keyword */
	kw_void				= LVOID,		/**< 'void'		keyword */
	//kw_signed,						/**< 'signed'	keyword */
	//kw_unsigned,						/**< 'unsigned'	keyword */
	kw_if				= LIF,			/**< 'if'		keyword */
	kw_else				= LELSE,		/**< 'else'		keyword */
	kw_do				= LDO,			/**< 'do'		keyword */
	kw_while			= LWHILE,		/**< 'while'	keyword */
	kw_for				= LFOR,			/**< 'for'		keyword */
	kw_switch			= LSWITCH,		/**< 'switch'	keyword */
	kw_case				= LCASE,		/**< 'case'		keyword */
	kw_default			= LDEFAULT,		/**< 'default'	keyword */
	kw_break			= LBREAK,		/**< 'break'	keyword */
	kw_continue			= LCONTINUE,	/**< 'continue'	keyword */
	//kw_const,							/**< 'const'	keyword */
	kw_goto				= LGOTO,		/**< 'goto'		keyword */
	//kw_sizeof,						/**< 'sizeof'	keyword */
	//kw_typedef,						/**< 'typedef'	keyword */
	kw_return			= LRETURN,		/**< 'return'	keyword */
	//kw_extern,						/**< 'extern'	keyword */
	//kw_inline,						/**< 'inline'	keyword */
	//kw_register,						/**< 'register'	keyword */

	// Identifiers [C99 6.4.2]
	identifier			= IDENT,		/**< Identifier [C99 6.4.2] */

	// Constants [C99 6.4.4]
	int_constant		= INT_CONST,	/**< Integer Constant [C99 6.4.4.1] */
	float_constant		= FLOAT_CONST,	/**< Floating Constant [C99 6.4.4.2] */
	char_constant		= CHAR_CONST,	/**< Character Constant [C99 6.4.4.4] */
	string_literal		= STRING,		/**< String Literal [C99 6.4.5] */

	// Punctuators [C99 6.4.6]
	question			= QUEST,		/**< '?'	punctuator */
	l_square			= LEFTSQBR,		/**< '['	punctuator */
	r_square			= RIGHTSQBR,	/**< ']'	punctuator */
	l_paren				= LEFTBR,		/**< '('	punctuator */
	r_paren				= RIGHTBR,		/**< ')'	punctuator */
	l_brace				= BEGIN,		/**< '{'	punctuator */
	r_brace				= END,			/**< '}'	punctuator */
	tilde				= LNOT,			/**< '~'	punctuator */
	colon				= COLON,		/**< ':'	punctuator */
	semicolon			= SEMICOLON,	/**< ';'	punctuator */
	comma				= COMMA,		/**< ','	punctuator */
	period				= DOT,			/**< '.'	punctuator */
	plus				= LPLUS,		/**< '+'	punctuator */
	minus				= LMINUS,		/**< '-'	punctuator */
	star				= LMULT,		/**< '*'	punctuator */
	slash				= LDIV,			/**< '/'	punctuator */
	percent				= LREM,			/**< '%'	punctuator */
	exclaim				= LOGNOT,		/**< '!'	punctuator */
	caret				= LEXOR,		/**< '^'	punctuator */
	pipe				= LOR,			/**< '|'	punctuator */
	amp					= LAND,			/**< '&'	punctuator */
	equal				= ASS,			/**< '='	punctuator */
	less				= LLT,			/**< '<'	punctuator */
	greater				= LGT,			/**< '>'	punctuator */
	arrow				= ARROW,		/**< '->'	punctuator */
	plusequal			= PLUSASS,		/**< '+='	punctuator */
	plusplus			= INC,			/**< '++'	punctuator */
	minusequal			= MINUSASS,		/**< '-='	punctuator */
	minusminus			= DEC,			/**< '--'	punctuator */
	starequal			= MULTASS,		/**< '*='	punctuator */
	slashequal			= DIVASS,		/**< '/='	punctuator */
	percentequal		= REMASS,		/**< '%='	punctuator */
	exclaimequal		= NOTEQ,		/**< '!='	punctuator */
	caretequal			= EXORASS,		/**< '^='	punctuator */
	pipeequal			= ORASS,		/**< '|='	punctuator */
	pipepipe			= LOGOR,		/**< '||'	punctuator */
	ampequal			= ANDASS,		/**< '&='	punctuator */
	ampamp				= LOGAND,		/**< '&&'	punctuator */
	equalequal			= EQEQ,			/**< '=='	punctuator */
	lessequal			= LLE,			/**< '<='	punctuator */
	lessless			= LSHL,			/**< '<<'	punctuator */
	greaterequal		= LGE,			/**< '>='	punctuator */
	greatergreater		= LSHR,			/**< '>>'	punctuator */
	lesslessequal		= SHLASS,		/**< '<<='	punctuator */
	greatergreaterequal	= SHRASS,		/**< '>>='	punctuator */

	// Standard Functions [RuC]
	kw_print			= PRINT,		/**< 'print'	keyword	*/
	kw_printf			= PRINTF,		/**< 'printf'	keyword */
	kw_printid			= PRINTID,		/**< 'printid'	keyword */
	kw_scanf			= SCANF,		/**< 'scanf'	keyword */
	kw_getid			= GETID,		/**< 'getid'	keyword */
	kw_abs				= ABS,			/**< 'abs'		keyword */
	kw_sqrt				= SQRT,			/**< 'sqrt'		keyword */
	kw_exp				= EXP,			/**< 'exp'		keyword */
	kw_sin				= SIN,			/**< 'sin'		keyword */
	kw_cos				= COS,			/**< 'cos'		keyword */
	kw_log				= LOG,			/**< 'log'		keyword */
	kw_log10			= LOG10,		/**< 'log10'	keyword */
	kw_asin				= ASIN,			/**< 'asin'		keyword */
	kw_rand				= RAND,			/**< 'rand'		keyword */
	kw_round			= ROUND,		/**< 'round'	keyword */
	kw_strcpy			= STRCPY,		/**< 'strcpy'	keyword */
	kw_strncpy			= STRNCPY,		/**< 'strncpy'	keyword */
	kw_strcat			= STRCAT,		/**< 'strcat'	keyword */
	kw_strncat			= STRNCAT,		/**< 'strncat'	keyword */
	kw_strcmp			= STRCMP,		/**< 'strcmp'	keyword */
	kw_strncmp			= STRNCMP,		/**< 'strncmp'	keyword */
	kw_strstr			= STRSTR,		/**< 'strstr'	keyword */
	kw_strlen			= STRLEN,		/**< 'strlen'	keyword */

	kw_t_create_direct	= TCREATEDIRECT,
	kw_t_exit_direct	= TEXITDIRECT,

	kw_msg_send			= TMSGSEND,		/**< 't_msg_send'		keyword */
	kw_msg_receive		= TMSGRECEIVE,	/**< 't_msg_receive'	keyword */
	kw_join				= TJOIN,		/**< 't_join'			keyword */
	kw_sleep			= TSLEEP,		/**< 't_sleep'			keyword */
	kw_sem_create		= TSEMCREATE,	/**< 't_sem_create'		keyword */
	kw_sem_wait			= TSEMWAIT,		/**< 't_sem_wait'		keyword */
	kw_sem_post			= TSEMPOST,		/**< 't_sem_post'		keyword */
	kw_create			= TCREATE,		/**< 't_create'			keyword */
	kw_init				= TINIT,		/**< 't_init'			keyword */
	kw_destroy			= TDESTROY,		/**< 't_destroy'		keyword */
	kw_exit				= TEXIT,		/**< 't_exit'			keyword */
	kw_getnum			= TGETNUM,		/**< 't_getnum'			keyword */

	kw_assert			= ASSERT,		/**< 'assert'		keyword */
	kw_pixel			= PIXEL,		/**< 'pixel'		keyword */
	kw_line				= LINE,			/**< 'line'			keyword */
	kw_rectangle		= RECTANGLE,	/**< 'rectangle'	keyword */
	kw_ellipse			= ELLIPS,		/**< 'ellipse'		keyword */
	kw_clear			= CLEAR,		/**< 'clear'		keyword */
	kw_draw_string		= DRAW_STRING,	/**< 'draw_string'	keyword */
	kw_draw_number		= DRAW_NUMBER,	/**< 'draw_number'	keyword */
	kw_icon				= ICON,			/**< 'icon'			keyword */
	kw_upb				= UPB,			/**< 'upb'			keyword */
	kw_setsignal		= SETSIGNAL,	/**< 'setsignal'	keyword */
	kw_setmotor			= SETMOTOR, 	/**< 'setmotor'		keyword */
	kw_setvoltage		= VOLTAGE,		/**< 'setvoltage'	keyword */
	kw_getdigsensor		= GETDIGSENSOR,	/**< 'getdigsensor'	keyword */
	kw_getansensor		= GETANSENSOR,	/**< 'getansensor'	keyword */

	kw_wifi_connect			= WIFI_CONNECT,			/**< 'wifi_connect'					keyword */
	kw_blynk_authorization	= BLYNK_AUTORIZATION,	/**< 'blynk_authorization'			keyword */
	kw_blynk_send			= BLYNK_SEND,			/**< 'blynk_send'					keyword */
	kw_blynk_receive		= BLYNK_RECEIVE,		/**< 'blynk_receive'				keyword */
	kw_blynk_notification	= BLYNK_NOTIFICATION,	/**< 'blynk_notification'			keyword */
	kw_blynk_property		= BLYNK_PROPERTY,		/**< 'blynk_property'				keyword */
	kw_blynk_lcd			= BLYNK_LCD,			/**< 'blynk_lcd'					keyword */
	kw_blynk_terminal		= BLYNK_TERMINAL,		/**< 'blynk_terminal'				keyword */
	kw_send_int				= SEND_INT,				/**< 'send_int_to_robot'			keyword */
	kw_send_float			= SEND_FLOAT,			/**< 'send_float_to_robot'			keyword */
	kw_send_string			= SEND_STRING,			/**< 'send_string_to_robot'			keyword */
	kw_receive_int			= RECEIVE_INT,			/**< 'receive_int_from_robot'		keyword */
	kw_receive_float		= RECEIVE_FLOAT,		/**< 'receive_float_from_robot'		keyword */
	kw_receive_string		= RECEIVE_STRING,		/**< 'receive_string_from_robot'	keyword */

	kw_fopen,										/**< 'fopen'	keyword */
	kw_fclose,										/**< 'fclose'	keyword */
	kw_fprintf,										/**< 'fprintf'	keyword */
	kw_fscanf,										/**< 'fscanf'	keyword */
} token_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
