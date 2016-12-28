//  RuC2A
//
//  Created by Andrey Terekhov on 24/Apr/16.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
// http://www.lysator.liu.se/c/ANSI-C-grammar-y.html
#define _CRT_SECURE_NO_WARNINGS
char* name = "../../../tests/bad Misha/print exper.c";

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include "Defs.h"

// Определение глобальных переменных

FILE *input, *output;
float numfloat;
int line=0, charnum=1, cur, next, next1, num, numr, hash, repr, keywordsnum, wasstructdef = 0;
int source[SOURCESIZE], lines[LINESSIZE];
int nextchar, curchar, func_def;
int hashtab[256] = {0, },
    reprtab[MAXREPRTAB] = {0, },
    rp = 1,
    identab[MAXIDENTAB],
    id = 2,
    modetab[MAXMODETAB],
    md = 1,
    startmode = 0;
int stack[100], stackop[100], stackoperands[100], ansttype,
    sp=0, sopnd=-1, aux=0, lastid, curid = 2, lg=-1, displ=-2, maxdispl = 3, maxdisplg = 3, type,
    op = 0, inass = 0, firstdecl;
int iniprocs[INIPROSIZE], procd = 1, arrdim, arrelemlen, was_struct_with_arr;
int instring = 0, inswitch = 0, inloop = 0, lexstr[MAXSTRINGL+1];
int tree[MAXTREESIZE], tc=0, mem[MAXMEMSIZE], pc=0, functions[FUNCSIZE], funcnum = 2, functype, kw = 0, blockflag = 1,
    entry, wasmain = 0, wasret, wasdefault, notcopy, structdispl, notrobot = 1;
int adcont, adbreak, adcase;
int predef[FUNCSIZE], prdf = -1, emptyarrdef;
int gotost[1000], pgotost;
int anst, anstdispl, ansttype;              // anst = VAL  - значение на стеке
int g, l, x, iniproc;                                // anst = ADDR - на стеке адрес значения
                                            // anst = IDENT- значение в статике, в anstdisl смещение от l или g
                                            // в ansttype всегда тип возвращаемого значения
// если значение указателя, адрес массива или строки лежит на верхушке стека, то это VAL, а не ADDR

extern void tablesandcode();
extern void tablesandtree();
extern void import();
extern int  getnext();
extern int  nextch();
extern int  scan();
extern void error(int ernum);
extern void codegen();
extern void ext_decl();

int main()
{
    int i;

    // занесение ключевых слов в reprtab
    keywordsnum = 1;
    
    input =  fopen("../../../keywords.txt", "r");
    if (input == NULL)
    {
        printf(" не найден файл %s\n", "keywords.txt");
        exit(1);
    }

    output = fopen("../../../tree.txt", "wt");

    getnext();
    nextch();
    while (scan() != LEOF)
        ;
    fclose(input);
    
    input =  fopen(name, "r");
    if (input == NULL)
    {
        printf(" не найден файл %s\n", name);
        exit(1);
    }
    modetab[1] = 0;
    keywordsnum = 0;
    lines[line = 1] = 1;
    charnum = 1;
    kw = 1;
    tc = 0;
    getnext();
    nextch();
    next = scan();
    
    ext_decl();
    
    lines[line+1] = charnum;
    tablesandtree();
    fclose(output);
    output = fopen("../../../codes.txt", "wt");
    
    codegen();
    
    tablesandcode();
    
    fclose(input);
    fclose(output);
    
    output = fopen("../../../export.txt", "wt");
    fprintf(output, "%i %i %i %i %i %i %i\n", pc, funcnum, id, rp, md, maxdisplg, wasmain);
    
    for (i=0; i<pc; i++)
        fprintf(output, "%i ", mem[i]);
    fprintf(output, "\n");
    
    for (i=0; i<funcnum; i++)
        fprintf(output, "%i ", functions[i]);
    fprintf(output, "\n");
    
    for (i=0; i<id; i++)
        fprintf(output, "%i ", identab[i]);
    fprintf(output, "\n");
    
    for (i=0; i<rp; i++)
        fprintf(output, "%i ", reprtab[i]);
    
    for (i=0; i<md; i++)
        fprintf(output, "%i ", modetab[i]);
    fprintf(output, "\n");
    
    fclose(output);
   
    if (notrobot)
        import();
    
    return 0;
}

