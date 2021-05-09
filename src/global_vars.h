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

#pragma once

#include <stdio.h>
#include "defs.h"


extern FILE *input;
extern FILE *output;

extern double numdouble;
extern float numf;
extern int line, mline, charnum, m_charnum, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef;
extern struct {int first; int second;} numr;
extern int source[SOURCESIZE], lines[LINESSIZE];
extern int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];

extern int nextchar, curchar, func_def;
extern int hashtab[256], reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id, modetab[MAXMODETAB], md, startmode, functions[], funcnum,
    stack[100], stackop[100], stackoperands[100], stacklog[100], ansttype,
    sp, sopnd, aux, lastid, curid, lg, displ, maxdispl, maxdisplg, type, op, inass, firstdecl;
extern int iniprocs[INIPROSIZE], procd, arrdim, arrelemlen, was_struct_with_arr, usual;
extern int mem[MAXMEMSIZE], tree[MAXTREESIZE], tc, mtree[MAXTREESIZE], mtc,
           functions[FUNCSIZE], funcnum, functype, kw, blockflag,
           entry, wasmain, wasret, wasdefault, wasslice, notrobot, prep_flag;
extern int adcont, adbreak, adcase, adandor, switchreg;
extern int instring, inswitch, inloop, lexstr[MAXSTRINGL+1];
extern int predef[FUNCSIZE], prdf;
extern int gotost[], pgotost, regis;
extern int anst, anstdispl, ansttype, leftansttype;

extern int bad_printf_placeholder;

extern int cycle_jump_reduce, check_nested_for, enable_ind_var, delay_slot, cycle_condition_calculation, ind_var_reduction;
