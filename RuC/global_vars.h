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

typedef struct numr_obj {
	int first;
	int second;
} numr_obj;

extern numr_obj numr;

extern FILE *input;
extern FILE *output;

extern double numdouble;
extern int line, charnum, cur, next, next1, num, hash, repr, keywordsnum,
		   wasstructdef;
extern int source[SOURCESIZE], lines[LINESSIZE];

extern int nextchar, curchar, func_def;
extern int hashtab[256], reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id,
		   modetab[MAXMODETAB], md, startmode, functions[], funcnum,
    	   stack[100], stackop[100], stackoperands[100], stacklog[100],
    	   ansttype, sp, sopnd, aux, lastid, curid, lg, displ, maxdispl,
    	   maxdisplg, type, op, inass, firstdecl;
extern int iniprocs[INIPROSIZE], procd, arrdim, arrelemlen, was_struct_with_arr;
extern int mem[MAXMEMSIZE], tree[MAXTREESIZE], tc, functions[FUNCSIZE], funcnum,
		   functype, kw, blockflag, entry, wasmain, wasret, wasdefault,
		   wasslice, structdispl, notrobot;
extern int adcont, adbreak, adcase, adandor;
extern int instring, inswitch, inloop, lexstr[MAXSTRINGL+1];
extern int predef[FUNCSIZE], prdf, emptyarrdef;
extern int pc, g, l, x;
extern int gotost[], pgotost;
// anst = VAL  - значение на стеке
// anst = ADDR - на стеке адрес значения
// anst = IDENT- значение в статике, в anstdisl смещение отl или g
// в ansttype всегда тип возвращаемого значения (сейчас только LINT или LFLOAT)
extern int anst, anstdispl, ansttype, leftansttype;

extern int bad_placeholder;

#endif
