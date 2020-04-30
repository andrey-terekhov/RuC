/*
 *	Copyright 2019 Andrey Terekhov
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

#include "defs.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <stdio.h>


#define COMPILER_TABLE_SIZE_DEFAULT	 (100)
#define COMPILER_TABLE_INCREMENT_MIN (100)

#define REPRTAB		(context->reprtab.table)
#define REPRTAB_POS (context->reprtab.pos)
#define REPRTAB_LEN (context->reprtab.len)


#ifdef __cplusplus
extern "C" {
#endif

/** A designated compiler table */
typedef struct compiler_table
{
	int *table; /** Actual table */
	int len;	/** Length of a useful part of table */
	int pos;	/** A position in a table */
	int size;	/** Total size of a table */
} compiler_table;

/** Определение глобальных переменных */
typedef struct compiler_context
{
	universal_scanner_options input_options;
	universal_printer_options output_options;
	universal_printer_options err_options;
	universal_printer_options miscout_options;

	double numdouble;
	int line;
	int mline;
	int charnum;
	int m_charnum;
	int cur;
	int next;
	int next1;
	int num;
	int hash;
	int keywordsnum;
	int wasstructdef;
	struct
	{
		int first;
		int second;
	} numr;
	int source[SOURCESIZE];
	int lines[LINESSIZE];
	int before_source[SOURCESIZE];
	int mlines[LINESSIZE];
	int m_conect_lines[LINESSIZE];
	int nextchar;
	int curchar;
	int func_def;
	int hashtab[256];
	int identab[MAXIDENTAB];
	int id;
	int modetab[MAXMODETAB];
	int md;
	int startmode;
	int stack[100];
	int stackop[100];
	int stackoperands[100];
	int stacklog[100];
	int sp;
	int sopnd;
	int aux;
	int lastid;
	int curid;
	int lg;
	int displ;
	int maxdispl;
	int maxdisplg;
	int type;
	int op;
	int inass;
	int firstdecl;
	int iniprocs[INIPROSIZE];
	int procd;
	int arrdim;
	int arrelemlen;
	int was_struct_with_arr;
	int usual;
	int instring;
	int inswitch;
	int inloop;
	int lexstr[MAXSTRINGL + 1];
	int tree[MAXTREESIZE];
	int tc;
	int mtree[MAXTREESIZE];
	int mtc;
	int mem[MAXMEMSIZE];
	int pc;
	int functions[FUNCSIZE];
	int funcnum;
	int functype;
	int kw;
	int blockflag;
	int entry;
	int wasmain;
	int wasret;
	int wasdefault;
	int notrobot;
	int prep_flag;
	int adcont;
	int adbreak;
	int adcase;
	int adandor;
	int switchreg;
	int predef[FUNCSIZE];
	int prdf;
	int emptyarrdef;
	int gotost[1000];
	int pgotost;
	int anst;
	int anstdispl;
	int ansttype;
	int leftansttype; // anst = VAL  - значение на стеке
	int g;
	int l;
	int x;
	int iniproc; // anst = ADDR - на стеке адрес значения
				 // anst = IDENT- значение в статике,
				 // в anstdisl смещение от l или g
				 // в ansttype всегда тип возвращаемого значения
				 // если значение указателя, адрес массива или строки
				 //лежит на верхушке стека, то это VAL, а не ADDR
	int bad_printf_placeholder;
	int onlystrings;

	/* Preprocessor flags */
	int macrotext[MAXREPRTAB];
	int mstring[50];
	int macrofunction[MAXREPRTAB];
	int functionident[MAXREPRTAB];
	int fchange[50];
	int fip;
	int mfp;
	int mfirstrp; // начало и конец макрослов в reprtab
	int mlastrp;
	int mp;
	int msp;
	int ifln;
	int mcl;
	int checkif;
	int flag_show_macro;
	int arg;

	compiler_table reprtab;
} compiler_context;


/**
 *	Initialize RuC context
 *
 *	@param	context	Uninitialized RuC context
 */
void compiler_context_init(compiler_context *context);

/**
 *	Deinitialize RuC ontext
 *
 *	@param	context	Initialized RuC context
 */
void compiler_context_deinit(compiler_context *context);

/**
 *	Attach input to a specific input/output pipe
 *
 *	@param	context	RuC context
 *	@param	ptr		Context-specific data, e.g. path to file or a pointer to data
 *	@param	type	IO type
 *	@param	source	Data source
 */
void compiler_context_attach_io(compiler_context *context, const char *ptr, ruc_io_type type, ruc_io_source source);

/**
 *	Detach file from a specific input/output pipe
 *
 *	@param	context	RuC context
 *	@param	type	IO type
 */
void compiler_context_detach_io(compiler_context *context, ruc_io_type type);

/**
 *	Initialize compiler table
 *
 *	@param	table	Target compiler table
 */
void compiler_table_init(compiler_table *table);

/**
 *	Ensure that specific offset is allocated in a table
 *
 *	@param	table	Target compiler table
 *	@param	pos		Target position
 *
 *	@return	Table size
 */
int compiler_table_ensure_allocated(compiler_table *table, int pos);

/**
 *	Expand compiler table
 *
 *	@param	table	Target compiler table
 *	@param	len		Requested length
 *
 *	@return	New size
 */
int compiler_table_expand(compiler_table *table, int len);

#ifdef __cplusplus
} /* extern "C" */
#endif
