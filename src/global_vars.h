//
//  global_vars.h
//  RuC
//
//  Created by Andrey Terekhov on 03/06/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
///Users/ant/Desktop/RuC/RuC/main.c

#include <stdio.h>

#ifndef RuC_global_vars_h
#define RuC_global_vars_h

#include "Defs.h"

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

extern int cycle_jump_reduce;

#endif
