#include <stdlib.h>
#include <stdio.h>
#include "Defs.h"
#include "global_vars.h"

// Определения глобальных переменных
/** Глобальный дескриптор для чтения файлов */
FILE *input;

/** Глобальный дескриптор для записи файлов */
FILE *output;

double numdouble;

int line = 0,
	charnum = 1,
	cur,
	next,
	next1,
	num,
	hash,
	repr,
	keywordsnum,
	wasstructdef = 0;
numr_obj numr;
int source[SOURCESIZE],
	lines[LINESSIZE];
int nextchar,
	curchar,
	func_def;
int hashtab[256],
	reprtab[MAXREPRTAB],
	rp = 1,
	identab[MAXIDENTAB],
	id = 2;
int modetab[MAXMODETAB],
	md = 1,
	startmode = 1;
int stack[100],
	stackop[100],
	stackoperands[100],
	stacklog[100],
	ansttype,
    sp = 0,
    sopnd = -1,
    aux = 0,
    lastid,
    curid = 2,
    lg = -1,
    displ = -3,
    maxdispl = 3,
    maxdisplg = 3,
    type,
    op = 0,
    inass = 0,
    firstdecl;
int iniprocs[INIPROSIZE],
	procd = 1,
	arrdim,
	arrelemlen,
	was_struct_with_arr;
int instring = 0,
	inswitch = 0,
	inloop = 0,
	lexstr[MAXSTRINGL + 1];
int tree[MAXTREESIZE],
	tc=0,
	mem[MAXMEMSIZE],
	pc=4,
	functions[FUNCSIZE],
	funcnum = 2,
	functype,
	kw = 0,
	blockflag = 1,
    entry,
    wasmain = 0,
    wasret,
    wasdefault,
    structdispl;
int notrobot = 1;
int adcont,
	adbreak,
	adcase,
	adandor;
int predef[FUNCSIZE],
	prdf = -1,
	emptyarrdef;
int gotost[1000],
	pgotost;

// anst = VAL  - значение на стеке
// anst = ADDR - на стеке адрес значения
// anst = IDENT- значение в статике, в anstdisl смещение от l или g
// если значение указателя, адрес массива или строки лежит на верхушке стека,
// то это VAL, а не ADDR
int anst,
	anstdispl,
	ansttype,				// в ansttype всегда тип возвращаемого значения
	leftansttype = -1;
int g,
	l,
	x,
	iniproc;

int bad_placeholder = 0;