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

#include "global.h"
#include <stdio.h>
#include "defs.h"


// Определение глобальных переменных

FILE *input, *output;

double numdouble;
int line = 0, mline = 0, charnum = 1, m_charnum = 1, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef = 0;

int source[SOURCESIZE], lines[LINESSIZE];
int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];
int nextchar, curchar, func_def;
int hashtab[256], reprtab[MAXREPRTAB], rp = 1, identab[MAXIDENTAB], id = 2, modetab[MAXMODETAB], md = 1, startmode = 1;
int stack[100], stackop[100], stackoperands[100], stacklog[100], sp = 0, sopnd = -1, aux = 0, lastid, curid = 2,
	lg = -1, displ = -3, maxdispl = 3, maxdisplg = 3, type, op = 0, inass = 0, firstdecl;
int iniprocs[INIPROSIZE], procd = 1, arrdim, arrelemlen, was_struct_with_arr, usual;
int instring = 0, inswitch = 0, inloop = 0, lexstr[MAXSTRINGL + 1];
int tree[MAXTREESIZE], tc = 0, mtree[MAXTREESIZE], mtc = 0, mem[MAXMEMSIZE], pc = 4, functions[FUNCSIZE], funcnum = 2,
	functype, kw = 0, blockflag = 1, entry, wasmain = 0, wasret, wasdefault, notrobot = 1, prep_flag = 0;
int adcont, adbreak, adcase, adandor, switchreg;
int predef[FUNCSIZE], prdf = -1, emptyarrdef;
int gotost[1000], pgotost;

/*
 *	anst = VAL - значение на стеке
 *	anst = ADDR - на стеке адрес значения
 *	anst = NUMBER - ответ является константой
 *	anst = IDENT- значение в статике, в anstdisl смещение от l или g
 *	в ansttype всегда тип возвращаемого значения
 *	если значение указателя, адрес массива или строки лежит на верхушке стека, то это VAL, а не ADDR
 */
int anst, anstdispl, ansttype, leftansttype = -1;
int g, l, x, iniproc;

int bad_printf_placeholder = 0;
