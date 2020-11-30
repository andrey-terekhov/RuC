/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov
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
#include "syntax.h"
#include "uniio.h"


#define REPRTAB		(context->sx->reprtab.table)
#define REPRTAB_POS (context->sx->reprtab.pos)
#define REPRTAB_LEN (context->sx->reprtab.len)


#ifdef __cplusplus
extern "C" {
#endif

/** Определение глобальных переменных */
typedef struct compiler_context
{
	universal_io *io;
	syntax *sx;

	int line;
	int charnum;
	int charnum_before;				// useless
	int cur;
	int next;
	int num;
	int hash;
	int keywordsnum;
	int wasstructdef;
	struct
	{
		int first;
		int second;
	} numr;
	int last_line[LINESSIZE * 2];	// useless
	int nextchar;
	int curchar;
	int func_def;
	int hashtab[256];
	int startmode;
	int stack[100];
	int stackop[100];
	int stackoperands[100];
	int stacklog[100];
	int sp;
	int sopnd;
	int lastid;
	int curid;
	int lg;
	int displ;
	int maxdispl;
	int type;
	int op;
	int inass;
	int firstdecl;
	int arrdim;
	int was_struct_with_arr;
	int usual;
	int instring;
	int inswitch;
	int inloop;
	int lexstr[MAXSTRINGL + 1];
	int functype;
	int kw;							// useless
	int blockflag;
	int wasret;
	int wasdefault;
	int notrobot;					// useless
	//int prep_flag;
	int predef[FUNCSIZE];
	int prdf;
	int gotost[1000];
	int pgotost;
	int anst;
	int ansttype;
	int leftansttype; // anst = VAL  - значение на стеке
	int x;							// useless
	int iniproc; // anst = ADDR - на стеке адрес значения
				 // anst = IDENT- значение в статике,
				 // в anstdisl смещение от l или g
				 // в ansttype всегда тип возвращаемого значения
				 // если значение указателя, адрес массива или строки
				 //лежит на верхушке стека, то это VAL, а не ADDR
	int bad_printf_placeholder;
	int onlystrings;

	int buf_flag;
	int buf_cur;
	int temp_tc;
	int error_flag;
	int new_line_flag;				// useless
} compiler_context;


/**
 *	Create RuC context
 *
 *	@param	io		Universal io structure
 *	@param	sx		Syntax structure
 *
 *	@return	RuC context structure
 */
compiler_context compiler_context_create(universal_io *const io, syntax *const sx);

#ifdef __cplusplus
} /* extern "C" */
#endif
