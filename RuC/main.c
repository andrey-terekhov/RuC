//  RuC
//
//  Created by Andrey Terekhov on 24/Apr/16.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
// http://www.lysator.liu.se/c/ANSI-C-grammar-y.html

#define _CRT_SECURE_NO_WARNINGS

const char * name =
//"tests/Egor/string/strcat.c";
"tests/Fadeev/draw.c";
//"tests/arrstruct1.c";
//"../../../tests/mips/0test.c";

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <errno.h>
#include "Defs.h"

// Определение глобальных переменных

FILE *input, *output;
double numdouble;
int line=0, mline = 0, charnum=1, m_charnum = 1, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef = 0;
struct {int first; int second;} numr;
int source[SOURCESIZE], lines[LINESSIZE];
int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];
int nextchar, curchar, func_def;
int hashtab[256], reprtab[MAXREPRTAB], rp = 1, identab[MAXIDENTAB], id = 2,
    modetab[MAXMODETAB], md = 1, startmode = 1;
int stack[100], stackop[100], stackoperands[100], stacklog[100], ansttype,
    sp=0, sopnd=-1, aux=0, lastid, curid = 2, lg=-1, displ=-3, maxdispl = 3, maxdisplg = 3, type,
    op = 0, inass = 0, firstdecl;
int iniprocs[INIPROSIZE], procd = 1, arrdim, arrelemlen, was_struct_with_arr, usual;
int instring = 0, inswitch = 0, inloop = 0, lexstr[MAXSTRINGL+1];
int tree[MAXTREESIZE], tc=0, mtree[MAXTREESIZE], mtc=0,
    mem[MAXMEMSIZE], pc=4, functions[FUNCSIZE], funcnum = 2, functype, kw = 0, blockflag = 1,
    entry, wasmain = 0, wasret, wasdefault, notrobot = 1, prep_flag = 0;
int adcont, adbreak, adcase, adandor, switchreg;
int predef[FUNCSIZE], prdf = -1, emptyarrdef;
int gotost[1000], pgotost;
int anst, anstdispl, ansttype, leftansttype = -1;         // anst = VAL  - значение на стеке
int g, l, x, iniproc;                                     // anst = ADDR - на стеке адрес значения
                                                          // anst = IDENT- значение в статике, в anstdisl смещение от l или g
                                                          // в ansttype всегда тип возвращаемого значения
// если значение указателя, адрес массива или строки лежит на верхушке стека, то это VAL, а не ADDR

int bad_printf_placeholder = 0;

extern void preprocess_file();

extern void tablesandcode();
extern void tablesandtree();
extern void import();
extern int  getnext();
extern int  nextch();
extern int  scan();
extern void error(int ernum);
extern void codegen();
extern void mipsopt();
extern void mipsgen();
extern void ext_decl();

int toreprtab(char str[])
{
    int i, oldrepr = rp;
    hash = 0;
    rp += 2;
    for (i=0; str[i] != 0; i++)
    {
        hash += str[i];
        reprtab[rp++] = str[i];
    }
    hash &= 255;
    reprtab[rp++] = 0;
    reprtab[oldrepr] = hashtab[hash] ;
    reprtab[oldrepr+1] = 1;
    return hashtab[hash] = oldrepr;
}

/* Первичная инициализация глобальных объектов */
void init()
{
    memset(hashtab, 0, sizeof(hashtab));
}

/* Инициализация modetab */
void
init_modetab()
{
    // занесение в modetab описателя struct {int numTh; int inf; }
    modetab[1] = 0;
    modetab[2] = MSTRUCT;
    modetab[3] = 2;
    modetab[4] = 4;
    modetab[5] = modetab[7] = LINT;
    modetab[6] = toreprtab("numTh");
    modetab[8] = toreprtab("data");

    // занесение в modetab описателя функции void t_msg_send(struct msg_info m)
    modetab[9] = 1;
    modetab[10] = MFUNCTION;
    modetab[11] = LVOID;
    modetab[12] = 1;
    modetab[13] = 2;

    // занесение в modetab описателя функции void* interpreter(void* n)
    modetab[14] = 9;
    modetab[15] = MFUNCTION;
    modetab[16] = LVOIDASTER;
    modetab[17] = 1;
    modetab[18] = LVOIDASTER;
    modetab[19] = startmode = 14;
    md = 19;
    keywordsnum = 0;
    lines[line = 1] = 1;
    charnum = 1;
    kw = 1;
    tc = 0;
}

typedef enum ruc_io_type {
    IO_TYPE_INPUT,
    IO_TYPE_OUTPUT,
} ruc_io_type;

static FILE **
io_type2file(ruc_io_type type)
{
    switch (type)
    {
        case IO_TYPE_INPUT:
            return &input;
        case IO_TYPE_OUTPUT:
            return &output;
        default:
            return NULL;
    }
}

static const char *
io_type2access_mask(ruc_io_type type)
{
    switch (type)
    {
        case IO_TYPE_OUTPUT:
            return "wt";
        case IO_TYPE_INPUT:
        default:
            return "r";
    }
}

void
attach_io(const char *path, ruc_io_type type)
{
    FILE *f = fopen(path, io_type2access_mask(type));
    FILE **target = io_type2file(type);

    if (f == NULL)
    {
        if (type == IO_TYPE_OUTPUT)
            printf(" ошибка открытия файла %s: %s\n", path, strerror(errno));
        else
            printf(" не найден файл %s\n", path);
        exit(1);
    }

    *target = f;
}

void
detach_io(ruc_io_type type)
{
    FILE **f = io_type2file(type);

    if (*f != NULL)
        fclose(*f);
    *f = NULL;
}


/* Занесение ключевых слов в reprtab */
void
read_keywords(const char *path)
{
    attach_io(path, IO_TYPE_INPUT);

    keywordsnum = 1;
    getnext();
    nextch();
    while (scan() != LEOF)   // чтение ключевых слов
        ;

    detach_io(IO_TYPE_INPUT);
}

/* Вывод таблиц и дерева */
void
output_tables_and_tree(const char *path)
{
    attach_io(path, IO_TYPE_OUTPUT);

    getnext();
    nextch();
    next = scan();

    ext_decl();                       //   генерация дерева

    lines[line + 1] = charnum;
    tablesandtree();
    detach_io(IO_TYPE_OUTPUT);
}

/* Генерация кодов */
void
output_codes(const char *path)
{
    attach_io(path, IO_TYPE_OUTPUT);
    codegen();
    tablesandcode();
    detach_io(IO_TYPE_OUTPUT);
}

/* Вывод таблиц в файл */
void
output_export(const char *path)
{
    int i;

    attach_io(path, IO_TYPE_OUTPUT);
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

    detach_io(IO_TYPE_OUTPUT);
}

int main(int argc, const char * argv[])
{
    init();
    read_keywords("keywords.txt");

    // Открытие исходного текста
    attach_io(argc > 1 ? argv[1] : name, IO_TYPE_INPUT);

    // Препроцессинг в файл macro.txt
    attach_io("macro.txt", IO_TYPE_OUTPUT);
    init_modetab();

    printf("\nИсходный текст:\n \n");
    preprocess_file();                //   макрогенерация

    detach_io(IO_TYPE_OUTPUT);
    detach_io(IO_TYPE_INPUT);

    attach_io("macro.txt", IO_TYPE_INPUT);
    output_tables_and_tree("tree.txt");
    output_codes("codes.txt");
    output_export("export.txt");

    detach_io(IO_TYPE_INPUT);

    return 0;
}

