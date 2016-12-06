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

extern float numfloat;
extern int line, charnum, cur, next, next1, num, numr, hash, repr, keywordsnum, wasstructdef;
extern int source[SOURCESIZE], lines[LINESSIZE];

extern int nextchar, curchar, func_def;
extern int hashtab[256], reprtab[MAXREPRTAB], rp, identab[MAXIDENTAB], id, modetab[MAXMODETAB], md, startmode,
    functions[], funcnum,
    stack[100], stackop[100], stackoperands[100], ansttype,
    sp, sopnd, aux, lastid, curid, lg, displ, maxdispl, maxdisplg, type, op, inass, firstdecl;
extern int iniprocs[INIPROSIZE], procd, arrdim, arrelemlen, was_struct_with_arr;
extern int mem[MAXMEMSIZE], tree[MAXTREESIZE], tc, functions[FUNCSIZE], funcnum, functype, kw, blockflag,
           entry, wasmain, wasret, wasdefault, wasslice, notcopy, structdispl, notrobot;
extern int adcont, adbreak, adcase;
extern int instring, inswitch, inloop, lexstr[MAXSTRINGL+1];
extern int predef[FUNCSIZE], prdf, emptyarrdef;
extern int pc, g, l, x;
extern int gotost[], pgotost;
extern int anst, anstdispl, ansttype;
// anst = VAL  - значение на стеке
// anst = ADDR - на стеке адрес значения
// anst = IDENT- значение в статике, в anstdisl смещение отl или g
// в ansttype всегда тип возвращаемого значения (сейчас только LINT или LFLOAT)

// box - запрос на значение
// F, если значение можно разместить как угодно,
// VAL, если значение нужно поместить на верхушке стека
// DECX, если значения не нужно
// opassn+11, если нужно присвоить значение по адресу из верхушки стека,
// opassn,    если нужно присвоить переменной, смещение в boxdispl

#endif
