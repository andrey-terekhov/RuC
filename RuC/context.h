#ifndef RUC_CONTEXT_H
#define RUC_CONTEXT_H

#include <stdio.h>
#include "Defs.h"

#include "uniprinter.h"
#include "uniscanner.h"

// Определение глобальных переменных
typedef struct ruc_context
{
    const char *output_file; /** Output file */

    universal_scanner_options input_options;
    universal_printer_options output_options;
    universal_printer_options err_options;
    universal_printer_options miscout_options;

    double      numdouble;
    int         line;
    int         mline;
    int         charnum;
    int         m_charnum;
    int         cur;
    int         next;
    int         next1;
    int         num;
    int         hash;
    int         repr;
    int         keywordsnum;
    int         wasstructdef;
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
    int reprtab[MAXREPRTAB];
    int rp;
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
} ruc_context;

/**
 * Initialize RuC context
 *
 * @param context Uninitialized RuC context
 */
extern void ruc_context_init(ruc_context *context);

/**
 * Attach input to a specific input/output pipe
 *
 * @param context   RuC context
 * @param ptr       Context-specific data, e.g. path to file or a pointer to
 *                  data
 * @param type      IO type
 * @param source    Data source
 */
extern void ruc_context_attach_io(ruc_context *context,
                                  const char * ptr,
                                  ruc_io_type  type,
                                  ruc_io_source source);

/**
 * Detach file from a specific input/output pipe
 *
 * @param context   RuC context
 * @param type      IO type
 */
extern void ruc_context_detach_io(ruc_context *context, ruc_io_type type);

#endif
