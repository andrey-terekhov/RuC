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
	eof					= 120,			/**< End of file */

	// Keywords [C99 6.4.1]
	kw_main				= 0,			/**< 'main'		keyword */
	//kw_bool,							/**< 'bool'		keyword */
	//kw_auto,							/**< 'auto'		keyword */
	kw_char				= -2,			/**< 'char'		keyword */
	kw_double			= -5,			/**< 'double'	keyword */
	//kw_enum,							/**< 'enum'		keyword */
	kw_float			= -3,			/**< 'float'	keyword */
	kw_int				= -1,			/**< 'int'		keyword	*/
	kw_long				= -4,			/**< 'long'		keyword */
	//kw_short,							/**< 'short'	keyword */
	kw_struct			= -14,			/**< 'struct'	keyword */
	//kw_union,							/**< 'union'	keyword */
	kw_void				= -6,			/**< 'void'		keyword */
	//kw_signed,						/**< 'signed'	keyword */
	//kw_unsigned,						/**< 'unsigned'	keyword */
	kw_if				= -18,			/**< 'if'		keyword */
	kw_else				= -12,			/**< 'else'		keyword */
	kw_do				= -11,			/**< 'do'		keyword */
	kw_while			= -22,			/**< 'while'	keyword */
	kw_for				= -16,			/**< 'for'		keyword */
	kw_switch			= -21,			/**< 'switch'	keyword */
	kw_case				= -8,			/**< 'case'		keyword */
	kw_default			= -10,			/**< 'default'	keyword */
	kw_break			= -7,			/**< 'break'	keyword */
	kw_continue			= -9,			/**< 'continue'	keyword */
	//kw_const,							/**< 'const'	keyword */
	kw_goto				= -17,			/**< 'goto'		keyword */
	//kw_sizeof,						/**< 'sizeof'	keyword */
	//kw_typedef,						/**< 'typedef'	keyword */
	kw_return			= -19,			/**< 'return'	keyword */
	//kw_extern,						/**< 'extern'	keyword */
	//kw_inline,						/**< 'inline'	keyword */
	//kw_register,						/**< 'register'	keyword */

	// Identifiers [C99 6.4.2]
	identifier			= 109,			/**< Identifier [C99 6.4.2] */

	// Constants [C99 6.4.4]
	int_constant		= 112,			/**< Integer Constant [C99 6.4.4.1] */
	float_constant		= 113,			/**< Floating Constant [C99 6.4.4.2] */
	char_constant		= 111,			/**< Character Constant [C99 6.4.4.4] */
	string_literal		= 107,			/**< String Literal [C99 6.4.5] */

	// Punctuators [C99 6.4.6]
	// Такие коды сделаны для функции token_skip_until()
	// Это позволяет передавать несколько аргументов, разделяя их |
	question			= 101,			/**< '?'	punctuator */
	l_square			= 105,			/**< '['	punctuator */
	r_square			= 0b00001000,	/**< ']'	punctuator */
	l_paren				= 103,			/**< '('	punctuator */
	r_paren				= 0b00000100,	/**< ')'	punctuator */
	l_brace				= 115,			/**< '{'	punctuator */
	r_brace				= 0b00010000,	/**< '}'	punctuator */
	tilde				= LNOT,			/**< '~'	punctuator */
	colon				= 0b00000010,	/**< ':'	punctuator */
	semicolon			= 0b00100000,	/**< ';'	punctuator */
	comma				= 0b00000001,	/**< ','	punctuator */
	period				= 122,			/**< '.'	punctuator */
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
	arrow				= 121,			/**< '->'	punctuator */
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
	kw_print			= -24,			/**< 'print'	keyword	*/
	kw_printf			= -25,			/**< 'printf'	keyword */
	kw_printid			= -23,			/**< 'printid'	keyword */
	kw_scanf			= -26,			/**< 'scanf'	keyword */
	kw_getid			= -27,			/**< 'getid'	keyword */
#define STANDARD_FUNC_START -30
	kw_abs				= -34,			/**< 'abs'		keyword */
	kw_sqrt				= -35,			/**< 'sqrt'		keyword */
	kw_exp				= -36,			/**< 'exp'		keyword */
	kw_sin				= -37,			/**< 'sin'		keyword */
	kw_cos				= -38,			/**< 'cos'		keyword */
	kw_log				= -39,			/**< 'log'		keyword */
	kw_log10			= -40,			/**< 'log10'	keyword */
	kw_asin				= -41,			/**< 'asin'		keyword */
	kw_rand				= -42,			/**< 'rand'		keyword */
	kw_round			= -43,			/**< 'round'	keyword */
	kw_strcpy			= -44,			/**< 'strcpy'	keyword */
	kw_strncpy			= -45,			/**< 'strncpy'	keyword */
	kw_strcat			= -46,			/**< 'strcat'	keyword */
	kw_strncat			= -47,			/**< 'strncat'	keyword */
	kw_strcmp			= -48,			/**< 'strcmp'	keyword */
	kw_strncmp			= -49,			/**< 'strncmp'	keyword */
	kw_strstr			= -50,			/**< 'strstr'	keyword */
	kw_strlen			= -51,			/**< 'strlen'	keyword */

	kw_t_create_direct	= -28,
	kw_t_exit_direct	= -29,

	kw_msg_send			= -52,			/**< 't_msg_send'		keyword */
	kw_msg_receive		= -53,			/**< 't_msg_receive'	keyword */
	kw_join				= -54,			/**< 't_join'			keyword */
	kw_sleep			= -55,			/**< 't_sleep'			keyword */
	kw_sem_create		= -56,			/**< 't_sem_create'		keyword */
	kw_sem_wait			= -57,			/**< 't_sem_wait'		keyword */
	kw_sem_post			= -58,			/**< 't_sem_post'		keyword */
	kw_create			= -59,			/**< 't_create'			keyword */
	kw_init				= -60,			/**< 't_init'			keyword */
	kw_destroy			= -61,			/**< 't_destroy'		keyword */
	kw_exit				= -62,			/**< 't_exit'			keyword */
	kw_getnum			= -63,			/**< 't_getnum'			keyword */

	kw_assert			= -95,			/**< 'assert'		keyword */
	kw_pixel			= -80,			/**< 'pixel'		keyword */
	kw_line				= -81,			/**< 'line'			keyword */
	kw_rectangle		= -82,			/**< 'rectangle'	keyword */
	kw_ellipse			= -83,			/**< 'ellipse'		keyword */
	kw_clear			= -84,			/**< 'clear'		keyword */
	kw_draw_string		= -85,			/**< 'draw_string'	keyword */
	kw_draw_number		= -86,			/**< 'draw_number'	keyword */
	kw_icon				= -87,			/**< 'icon'			keyword */
	kw_upb				= -88,			/**< 'upb'			keyword */
	kw_setsignal		= -79,			/**< 'setsignal'	keyword */
	kw_setmotor			= -30, 			/**< 'setmotor'		keyword */
	kw_setvoltage		= -33,			/**< 'setvoltage'	keyword */
	kw_getdigsensor		= -31,			/**< 'getdigsensor'	keyword */
	kw_getansensor		= -32,			/**< 'getansensor'	keyword */

	kw_wifi_connect			= -71,		/**< 'wifi_connect'					keyword */
	kw_blynk_authorization	= -72,		/**< 'blynk_authorization'			keyword */
	kw_blynk_send			= -73,		/**< 'blynk_send'					keyword */
	kw_blynk_receive		= -74,		/**< 'blynk_receive'				keyword */
	kw_blynk_notification	= -75,		/**< 'blynk_notification'			keyword */
	kw_blynk_property		= -76,		/**< 'blynk_property'				keyword */
	kw_blynk_lcd			= -77,		/**< 'blynk_lcd'					keyword */
	kw_blynk_terminal		= -78,		/**< 'blynk_terminal'				keyword */
	kw_send_int				= -89,		/**< 'send_int_to_robot'			keyword */
	kw_send_float			= -90,		/**< 'send_float_to_robot'			keyword */
	kw_send_string			= -91,		/**< 'send_string_to_robot'			keyword */
	kw_receive_int			= -92,		/**< 'receive_int_from_robot'		keyword */
	kw_receive_float		= -93,		/**< 'receive_float_from_robot'		keyword */
	kw_receive_string		= -94,		/**< 'receive_string_from_robot'	keyword */
} token_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
