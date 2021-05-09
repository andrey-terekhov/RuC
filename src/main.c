/*
 *	Copyright 2015 Andrey Terekhov
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

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include "defs.h"


const char *name = "/Users/elias/test.c";


// Определение глобальных переменных

FILE *input, *output;
double numdouble;
float numf;
int line=0, mline = 0, charnum=1, m_charnum = 1, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef = 0;
 struct {int first; int second;} numr;
int source[SOURCESIZE], lines[LINESSIZE];
int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];
int nextchar, curchar, func_def;
int hashtab[256], reprtab[MAXREPRTAB], rp = 1, identab[MAXIDENTAB], id = 2,
    modetab[MAXMODETAB], md = 1, startmode = 1;
int stack[100], stackop[100], stackoperands[100], stacklog[100], ansttype,
    sp=0, sopnd=-1, aux=0, lastid, curid = 2, lg = -1, displ = 24, maxdispl,
    maxdisplg, type, op = 0, inass = 0, firstdecl;
int iniprocs[INIPROSIZE], procd = 1, arrdim, arrelemlen, was_struct_with_arr, usual;
int instring = 0, inswitch = 0, inloop = 0, lexstr[MAXSTRINGL+1];
int tree[MAXTREESIZE], tc=0, mtree[MAXTREESIZE], mtc=0,
    mem[MAXMEMSIZE], functions[FUNCSIZE], funcnum = 2, functype, kw = 0,
    blockflag = 1, entry, wasmain = 0, wasret, wasdefault, notrobot = 1, prep_flag = 0;
int adcont, adbreak, adcase, adandor, switchreg;
int predef[FUNCSIZE], prdf = -1, emptyarrdef;
int gotost[1000], pgotost, regis;
int anst, anstdispl, ansttype, leftansttype = -1;   

int bad_printf_placeholder = 0;

// optimization flags
int cycle_jump_reduce = 0;
int enable_ind_var = 1;
int cycle_condition_calculation = 0;
int delay_slot = 0;
int check_nested_for = 0;


extern void preprocess_file();

extern void tablesandtree();
extern void import();
extern int  getnext();
extern int  nextch();
extern int  scan();
extern void error(int ernum);
extern void mipsopt();
extern void optimize();
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

FILE *keywords(const char *const exec)
{
    char path[256];
    int last_slash = 0;

    for (int i = 0; exec[i] != '\0'; i++)
    {
        if (exec[i] == '\\' || exec[i] == '/')
        {
            last_slash = i;
        }

        path[i] = exec[i];
    }

    strcpy(&path[last_slash + 1], "keywords.txt");
    return fopen(path, "r");
}

int main(int argc, const char * argv[])
{
    int i;

	// включение вспомогательных оптимизирующих опций
	check_nested_for = cycle_condition_calculation || enable_ind_var;

    if (argc != 2){
        printf("Error: not enough argumnts\n");
    }
    else {
        name = argv[1];
    }
    for (i=0; i<256; i++)
        hashtab[i] = 0;
    
    // занесение ключевых слов в reprtab
    keywordsnum = 1;
    
    input = keywords(argv[0]);
    if (input == NULL)
    {
        printf(" не найден файл %s\n", "keywords.txt");
        exit(1);
    }
    getnext();
    nextch();
    while (scan() != LEOF)   // чтение ключевых слов
        ;
    fclose(input);           // закрытие файла ключевых слов
    
    if (argc < 2) {
        input = fopen(name, "r");          //   исходный текст
    } else {
        input = fopen(argv[1], "r");
    }
    output = fopen("macro.txt", "wt");

    if (input == NULL)
    {
        if (argc < 2) {
            printf(" не найден файл %s\n", name);
        } else {
            printf(" не найден файл %s\n", argv[1]);
        }
        
        exit(1);
    }
 
    modetab[1] = 0;
    modetab[2] = MSTRUCT;
    modetab[3] = 2;
    modetab[4] = 4;
    modetab[5] = modetab[7] = LINT;
    modetab[6] = toreprtab("numTh");
    modetab[8] = toreprtab("data");
    modetab[9] = 1;                // занесение в modetab описателя struct{int numTh; int inf;}
    modetab[10] = MFUNCTION;
    modetab[11] = LVOID;
    modetab[12] = 1;
    modetab[13] = 2;
    modetab[14] = 9;                  // занесение в modetab описателя  функции void t_msg_send(struct msg_info m)
    modetab[15] = MFUNCTION;
    modetab[16] = LVOIDASTER;
    modetab[17] = 1;
    modetab[18] = LVOIDASTER;
    modetab[19] = startmode = 14;     // занесение в modetab описателя  функции void* interpreter(void* n)
    md = 19;
    keywordsnum = 0;
    lines[line = 1] = 1;
    charnum = 1;
    kw = 1;
    tc = 0;

    printf("\nИсходный текст:\n \n");
    preprocess_file();                //   макрогенерация
    
    fclose(input);                    // исходный файл до макрогенерации
    fclose(output);                   // исходный файл после макрогенерации
    
    input  = fopen(/*"macro.txt"*/name, "r"); // исходный файл после макрогенерации

    
    if (input == NULL)
    {
        printf("файл %s не найден\n", name);
    }
    if(prep_flag == 1)
    {
        printf("\nТекст после препроцесора:\n \n");
    }

    output = fopen("tree.txt", "wt"); // файл с деревом до mipsopt
    
    getnext();
    nextch();
    next = scan();

    ext_decl();                       // генерация дерева

    lines[line+1] = charnum;
    tablesandtree();
    fclose(output);                   // файл с деревом до mipsopt
    
    mipsopt();
    
    for (i=0; i<mtc; i++)
        tree[i] = mtree[i];

    output = fopen("mtree.txt", "wt");
    tc = mtc;
    tablesandtree();
    fclose(output);                   // файл с деревом после mipsopt

//	optimize();

	for (i=0; i<mtc; i++)
		tree[i] = mtree[i];

	output = fopen("optimized.txt", "wt");
	tc = mtc;
	tablesandtree();
	fclose(output);                   // файл с деревом после оптимизаций

    output = fopen("mcode.s", "wt");
    
    printf("\t.file \"%s\"\n", name);
    fprintf(output, "\t.file 1 \"%s\"\n", name);
    
    fprintf(output, "\t.section .mdebug.abi32\n\t.previous\n\t.nan\tlegacy\n");
    fprintf(output, "\t.module fp=xx\n\t.module nooddspreg\n\t.abicalls\n");
    fprintf(output, "\t.option pic0\n\t.text\n\t.align 2\n");

    mipsgen();                       
    
    fclose(input);
    fclose(output);
    
   
    return 0;
}
