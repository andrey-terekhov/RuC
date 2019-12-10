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

#define _CRT_SECURE_NO_WARNINGS

#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "defs.h"


// Определение глобальных переменных

struct
{
	int first;
	int second;
} numr;

FILE *input, *output;

double numdouble;
int line = 0, mline = 0, charnum = 1, m_charnum = 1, cur, next, next1, num, hash, repr, keywordsnum, wasstructdef = 0;

int source[SOURCESIZE], lines[LINESSIZE];
int before_source[SOURCESIZE], mlines[LINESSIZE], m_conect_lines[LINESSIZE];
int nextchar, curchar, func_def;
int hashtab[256], reprtab[MAXREPRTAB], rp = 1, identab[MAXIDENTAB], id = 2, modetab[MAXMODETAB], md = 1, startmode = 1;
int stack[100], stackop[100], stackoperands[100], stacklog[100], ansttype,
	sp = 0, sopnd = -1, aux = 0, lastid, curid = 2, lg = -1, displ = -3, maxdispl = 3, maxdisplg = 3, type, op = 0,
	inass = 0, firstdecl;
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
 *	anst = IDENT- значение в статике, в anstdisl смещение от l или g
 *	в ansttype всегда тип возвращаемого значения
 *	если значение указателя, адрес массива или строки лежит на верхушке стека, то это VAL, а не ADDR
 */
int anst, anstdispl, ansttype, leftansttype = -1;
int g, l, x, iniproc;

int bad_printf_placeholder = 0;

extern void preprocess_file();

extern void tablesandcode();
extern void tablesandtree();
extern void import();
extern int getnext();
extern int nextch();
extern int scan();
extern void error(int ernum);
extern void codegen();
extern void mipsopt();
extern void mipsgen();
extern void ext_decl();

int toreprtab(char str[])
{
	int i;
	int oldrepr = rp;
	hash = 0;
	rp += 2;

	for (i = 0; str[i] != 0; i++)
	{
		hash += str[i];
		reprtab[rp++] = str[i];
	}
	hash &= 255;
	reprtab[rp++] = 0;
	reprtab[oldrepr] = hashtab[hash];
	reprtab[oldrepr + 1] = 1;
	return hashtab[hash] = oldrepr;
}

void compile(const char *code)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		hashtab[i] = 0;
	}

	// занесение ключевых слов в reprtab
	keywordsnum = 1;

	input = fopen("keywords.txt", "r");
	if (input == NULL)
	{
		printf(" не найден файл %s\n", "keywords.txt");
		exit(1);
	}
	getnext();
	nextch();
	while (scan() != LEOF); // чтение ключевых слов
	fclose(input);

	input = fopen(code, "r"); // исходный текст
	output = fopen("macro.txt", "wt");

	if (input == NULL)
	{
		printf(" не найден файл %s\n", code);
		exit(1);
	}
	modetab[1] = 0;
	modetab[2] = MSTRUCT;
	modetab[3] = 2;
	modetab[4] = 4;
	modetab[5] = modetab[7] = LINT;
	modetab[6] = toreprtab("numTh");
	modetab[8] = toreprtab("data");
	modetab[9] = 1; // занесение в modetab описателя struct{int numTh; int inf;}
	modetab[10] = MFUNCTION;
	modetab[11] = LVOID;
	modetab[12] = 1;
	modetab[13] = 2;
	modetab[14] = 9; // занесение в modetab описателя функции void t_msg_send(struct msg_info m)
	modetab[15] = MFUNCTION;
	modetab[16] = LVOIDASTER;
	modetab[17] = 1;
	modetab[18] = LVOIDASTER;
	modetab[19] = startmode = 14; // занесение в modetab описателя функции void* interpreter(void* n)
	md = 19;
	keywordsnum = 0;
	lines[line = 1] = 1;
	charnum = 1;
	kw = 1;
	tc = 0;

	printf("\nИсходный текст:\n \n");
	preprocess_file(); // макрогенерация

	fclose(output);
	fclose(input);

	input = fopen("macro.txt", "r");

	if (input == NULL)
	{
		printf(" файл %s не найден\n", "macro.txt");
	}
	if (prep_flag == 1)
	{
		printf("\nТекст после препроцесора:\n \n");
	}

	output = fopen("tree.txt", "wt");

	getnext();
	nextch();
	next = scan();

	ext_decl(); // генерация дерева

	lines[line + 1] = charnum;
	tablesandtree();
	fclose(output);
	output = fopen("codes.txt", "wt");

	codegen(); // генерация кода

	tablesandcode();

	fclose(input);
	fclose(output);

	output = fopen("export.txt", "wt");
	fprintf(output, "%i %i %i %i %i %i %i\n", pc, funcnum, id, rp, md, maxdisplg, wasmain);

	for (i = 0; i < pc; i++)
		fprintf(output, "%i ", mem[i]);
	fprintf(output, "\n");

	for (i = 0; i < funcnum; i++)
		fprintf(output, "%i ", functions[i]);
	fprintf(output, "\n");

	for (i = 0; i < id; i++)
		fprintf(output, "%i ", identab[i]);
	fprintf(output, "\n");

	for (i = 0; i < rp; i++)
		fprintf(output, "%i ", reprtab[i]);

	for (i = 0; i < md; i++)
		fprintf(output, "%i ", modetab[i]);
	fprintf(output, "\n");

	fclose(output);
}
