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

#ifndef H_GLOBAL
#define H_GLOBAL

#include <stdio.h>
#include "defs.h"


struct
{
	int first;
	int second;
} numr;

extern FILE *input, *output;

extern double numdouble;
extern int line, mline, charnum, m_charnum, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef;

extern int source[SOURCESIZE], lines[LINESSIZE];
extern int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];
extern int nextchar, curchar, func_def;
extern int hashtab[256], reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id, modetab[MAXMODETAB], md, startmode;
extern int stack[100], stackop[100], stackoperands[100], stacklog[100], sp, sopnd, aux, lastid, curid, lg, displ,
	maxdispl, maxdisplg, type, op, inass, firstdecl;
extern int iniprocs[INIPROSIZE], procd, arrdim, arrelemlen, was_struct_with_arr, usual;
extern int instring, inswitch, inloop, lexstr[MAXSTRINGL + 1];
extern int tree[MAXTREESIZE], tc, mtree[MAXTREESIZE], mtc, mem[MAXMEMSIZE], pc, functions[FUNCSIZE], funcnum, functype,
	kw, blockflag, entry, wasmain, wasret, wasdefault, notrobot, prep_flag;
extern int adcont, adbreak, adcase, adandor, switchreg;
extern int predef[FUNCSIZE], prdf, emptyarrdef;
extern int gotost[1000], pgotost;

extern int anst, anstdispl, ansttype, leftansttype;
extern int g, l, x, iniproc;

extern int bad_printf_placeholder;

#endif
