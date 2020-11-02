/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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

#define MACRODEBUG 1

#define MAXTAB		100000
#define LONGSTR		10000
#define STRIGSIZE	256
#define HASH		256
#define DIP			10
#define CONTROLSIZE 250

#define CANGEEND	  -6
#define MACROEND	  -5
#define MACROUNDEF	  -3
#define MACROCANGE	  -4
#define WHILEBEGIN	  -2
#define MACROFUNCTION 0
#define MACRODEF	  1

#define MTYPE			  2
#define CTYPE			  3
#define IFTYPE			  4
#define WHILETYPE		  5
#define FTYPE			  6
#define TEXTTYPE		  10
#define PREPROCESS_STRING 11
#define FILETYPE		  0


// Ключевые слова

#define SH_MAIN	   -63
#define SH_DEFINE  -64
#define SH_IFDEF   -65
#define SH_IFNDEF  -66
#define SH_IF	   -67
#define SH_ELIF	   -68
#define SH_ENDIF   -69
#define SH_ELSE	   -70
#define SH_MACRO   -71
#define SH_ENDM	   -72
#define SH_WHILE   -73
#define SH_ENDW	   -74
#define SH_SET	   -75
#define SH_UNDEF   -76
#define SH_FOR	   -77
#define SH_ENDF	   -78
#define SH_EVAL	   -79
#define SH_INCLUDE -80
#define SH_FILE	   -81


// Коды ошибок

#define just_kill_yourself					1
#define after_ident_must_be_space1			366
#define ident_begins_with_s					367
#define must_be_endif						368
#define dont_elif							369
#define preproces_words_not_exist			370
#define not_enough_param					371
#define functionid_begins_with_letters		372
#define after_functionid_must_be_comma		373
#define stalpe								374
#define not_relis_if						375
#define before_endif						376
#define repeat_ident						377
#define not_enough_param2					378
#define not_end_fail_define					379
#define scob_not_clous						380
#define after_preproces_words_must_be_space 381
#define ident_begins_with_letters1			382
#define ident_not_exist						383
#define functions_cannot_be_changed			384
#define after_eval_must_be_ckob				385
#define too_many_nuber						386
#define must_be_digit_after_exp1			387
#define not_arithmetic_operations			389
#define not_logical_operations				390
#define comm_not_ended						391