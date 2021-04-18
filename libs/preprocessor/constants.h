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

#define MAXTAB			100000
#define LONGSTR			10000
#define STRING_SIZE		256
#define HASH			256
#define DEPTH_IO_SISE	10

#define MACRO_CANGE		'\r'

#define MACRO_UNDEF		'u'
#define MACRO_FUNCTION	'f'
#define MACRO_DEF		'd'


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
