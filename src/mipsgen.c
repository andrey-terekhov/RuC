/*
 *	Copyright 2018 Andrey Terekhov
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

#include <stdlib.h>
#include <string.h>
#include "global_vars.h"


// запросы
#define BREG   1     // загрузить в breg
#define BREGF  2     // запрос на левый операнд,     могут ответить
    // CONST, тогда делать ничего не надо,           могут ответить
    // AMEM, тогда надо загрузить в breg,            могут ответить
    // AREG, если areg - сохраняемый регистр, то ничего не делать, иначе в breg
    // В качестве левого операнда могут использоваться только сохраняемые регистры,
    // в том числе и те, которые применяются для адресации, то есть при этом запросе
    // структура должна адресоваться от breg
#define BF     3     // свободный запрос на правый операнд
#define BCF    4     // goto на метку elselab по false
#define BCT    5     // goto на метку elselab по true
#define BV     6     // значение не нужно

// ответы
#define AREG  1      // in areg register number
#define AMEM  2      // in adispl and areg address
#define CONST 3      // in num int or char const

#define DISPL0 80


int mbox, breg, elselab;
int manst, adispl, areg, idp;
int labnum = 1, stringnum = 1, elselab, flagBC, identref, structdispl;
int log_real = 2;
int flag_jump_end_cycle = 0;
int flag_cond_cycle = 0; // 0 - ничего, 1 - посчитать условия и записать регистр, 2 - условие уже в регистре breg
int cond_cycle_end_manst = 0;
int cond_cycle_end_manst_left = 0;
int cond_cycle_end_left_reg = 0;
int delay_slot_inc = 1; // если ++ в цикле, то 1, если -- в цикле, то -1
int left_reg_cond = -1; // левый регистр в случае редукции индуцированных переменных
// унарные операции LNOT, LOGNOT, -, ++, --, TIdenttoval(*), TIdenttoaddr(&)
// LNOT nor rd, rs, d0    LOGNOT slti rt, rs, 1   - sub rd, d0, rt
// *  lw rt, displ(rs) или сразу 0(areg)   & addi rt, areg, adispl или сразу areg
// ++ lw rt, adispl(areg) addi rt, rt, 1 sw rt, adispl(areg)
extern void error(int err);
extern int szof(int);
extern int alignm(int);

int globinit = -8000, heap = 268435456; // 0x100010000 - нижняя граница динамической памяти
int indexcheck = 0;

int pc = 32, d0 = 0, at = 1, v0 = 2,  v1 = 3,  a0 = 4,   a1 = 5, a2 = 6, a3 = 7,
                            fv0 = 34, fv1 =35, fa0 = 42, fa1 = 43,
    t0 = 8, t1 = 9, t2 = 10, t9 = 25, s0 = 16, s7 = 23, gp = 28, stp = 29, fp = 30, ra =  31,
   ft0 = 36, ft1 = 37,                fs0 = 44;  // opnd = 43;

char *mcodes[] =
{/* 0 */ "bgez", "bltz", "j", "jal", "beq", "bne", "blez", "bgtz", "addi", "addiu",
/* 10 */ "slti", "sltiu", "andi", "ori", "xori", "lui", "mfhi", "", "mflo", "",
/* 20 */ "", "", "", "", "", "li", "div", "", "mul", "",
/* 30 */ "", "", "lb", "", "", "lw", "", "", "", "",
/* 40 */ "sb", "", "", "sw", "lhu", "sh", "", "", "", "",
/* 50 */ "", "", "", "", "", "", "", "", "", "",
/* 60 */ "sll", "", "srl", "sra", "sllv", "", "srlv", "srav", "jr", "jalr",
/* 70 */ "", "", "", "", "", "", "", "", "", "",
/* 80 */ "bge", "blt", "ble", "bgt", "", "", "", "", "", "",
/* 90 */ "", "", "add", "addu", "sub", "subu", "and", "or", "xor", "nor",
/*100 */ "", "", "slt", "sltu", "", "", "", "", "", "",
/*110 */ "add.s", "sub.s", "mul.s", "div.s", "abs.s", "neg.s", "", "", "", "",
/*120 */ "li.s", "li.d", "", "", "", "lwc1", "swc1", "mtc1", "bc1t", "bc1f",
/*130 */ "c.seq.s", "c.lt.s", "c.le.s", "cvt.s.w"
};

char *regs[] =
{/* 0 */ "$0",  "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1",
/* 10 */ "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3",
/* 20 */ "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp",
/* 30 */ "$fp", "$ra", "pc", "",
/* 34 */ "$f0", "$f2", "$f4", "$f6", "$f8", "$f10", "$f12", "$f14", "$f16", "$f18",
/* 44 */ "$f20", "$f22", "$f24", "$f26", "$f28", "$f30"
};

int savereg(int r)
{
    return  (r >= 16 && r <= 23) || (r >= 44 && r <= 49);
}
int tregs[] =
{
    8, 9, 10, 11, 12, 13, 14, 15, 24, 25
};

int sregs[] =
{
    16, 17, 18, 19, 20, 21, 22, 23, 30
};

int isregs(int r)
{
    return r >= 16 && r <= 23;
}

int isregsf(int r)
{
    return r >= 44 && r <= 49;
}

#define too_many_sregs    1
#define too_many_sregsf   2
#define too_many_params   3
#define dim_greater_5     4
#define wrong_index       5
#define memory_overflow   6
#define wrong_number_of_elems 7

struct ind_var
{
	int id;
	int is_static;
	int step;
	int reg;
};

struct ind_var ind_var_info[10];
int ind_var_number;

int cur_dyn_arr_info[5];	// информация текущего объявляемого массива
int is_dyn;					// является ли текущий объявляемый массив динамическим
int dyn_arr_info[10000][5]; // доступ к информации осуществляется по displ, хранятся смещение относительно gp, где в памяти хранится граница

void merror(int type)
{
    switch (type)
    {
        case too_many_sregs:
            printf("кончились регистры s0-s7\n");
            break;
        case too_many_sregsf:
            printf("кончились регистры fs0-fs5\n");
            break;
        case too_many_params:
            printf("пока количество параметров ограничено 4\n");
            break;
        case dim_greater_5:
            printf("в РуСи размерность массивов не может быть больше 5\n");
            break;
        case wrong_index:
            printf("индекс массива меньше 0 или больше N-1\n");
            break;
        case memory_overflow:
            printf("исчерпание памяти\n");
            break;
        case wrong_number_of_elems:
            printf("граница массива не может быть отрицательной\n");
            break;
            
        default:
            break;
    }
}

int nextreg = 16, nextregf = 44;

int getreg()
{
    if (nextreg == 24)
        merror(too_many_sregs);
    return nextreg++;
}

int getregf()
{
    if (nextregf == 50)
        merror(too_many_sregsf);
    return nextregf++;
}


void freereg(int r)
{
    nextreg--;
}

void freeregf(int r)
{
    nextregf--;
}

void tocodeR(int op, int rd, int rs, int rt)   // add rd, rs, rt
{
    printf("\t%s %s, %s, %s\n", mcodes[op], regs[rd], regs[rs], regs[rt]);
    fprintf(output, "\t%s %s, %s, %s\n", mcodes[op], regs[rd], regs[rs], regs[rt]);
}

void tocodeI(int op, int rt, int rs, int imm)  // addi rt, rs, imm   slti rt, rs, imm
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rt], regs[rs], imm);
    fprintf(output, "\t%s %s, %s, %i\n", mcodes[op], regs[rt], regs[rs], imm);
}

void tocodeB(int op, int rt, int displ, int rs)  // lw rt, displ(rs)  sw rt, displ(rs)
{
    printf("\t%s %s, %i(%s)\n", mcodes[op], regs[rt], displ, regs[rs]);
    fprintf(output, "\t%s %s, %i(%s)\n", mcodes[op], regs[rt], displ, regs[rs]);
}

void tocodeLI(int op, int rt, int imm)  // lui rt, imm,   li rt, imm(32)
{
    printf("\t%s %s, %i\n", mcodes[op], regs[rt], imm);
    fprintf(output, "\t%s %s, %i\n", mcodes[op], regs[rt], imm);
}

void tocodeLI_S(int op, int rt, double imm)  // li_s rt, imm
{
    printf("\t%s %s, %f\n", mcodes[op], regs[rt], imm);
    fprintf(output, "\t%s %s, %f\n", mcodes[op], regs[rt], imm);
}

void tocodemove(int rd, int rs)  // move rd, rs
{
    if (rd >= 34)
    {
        printf("\tmov.s %s, %s\n", regs[rd], regs[rs]);
        fprintf(output, "\tmov.s %s, %s\n", regs[rd], regs[rs]);
    }
    else
    {
        printf("\tmove %s, %s\n", regs[rd], regs[rs]);
        fprintf(output, "\tmove %s, %s\n", regs[rd], regs[rs]);
    }
}

void tocodeSLR(int op, int rd, int rt, int shamt)  // sll rd, rt, shamt    srl sra
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
    fprintf(output, "\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
}

void tocodeJ(int op, char type[], int label)  // j label   jal label
{
    if (label == -1)
    {
        printf("\t%s %s\n\tnop\n", mcodes[op], type);
        fprintf(output, "\t%s %s\n\tnop\n", mcodes[op], type);
    }
    else
    {
        printf("\t%s %s%i\n\tnop\n", mcodes[op], type, label);
        fprintf(output, "\t%s %s%i\n\tnop\n", mcodes[op], type, label);
    }
}

void tocodeJR(int op, int rs)   // jr rs    jalr rs
{
    printf("\t%s %s\n\tnop\n", mcodes[op], regs[rs]);
    fprintf(output, "\t%s %s\n\tnop\n", mcodes[op], regs[rs]);
}

void tocodeJC(int op, int rs, char type[], int label)  // bltz rs, label    bgez blez bgtz
{
    printf("\t%s %s, %s%i\n", mcodes[op], regs[rs], type, label);
    fprintf(output, "\t%s %s, %s%i\n", mcodes[op], regs[rs], type, label);
}

void tocodeJCimm(int op, int rs, int imm)  // bltz rs, imm    bgez blez bgtz
{
    printf("\t%s %s, %i\n", mcodes[op], regs[rs], imm);
    fprintf(output, "\t%s %s, %i\n", mcodes[op], regs[rs], imm);
}

void tocodeJEQ(int op, int rs, int rt, char type[], int label)  // beq rs, rt, label   bne
{
    printf("\t%s %s, %s, %s%i\n", mcodes[op], regs[rs], regs[rt], type, label);
    fprintf(output, "\t%s %s, %s, %s%i\n", mcodes[op], regs[rs], regs[rt], type, label);
}

void tocodeJEQimm(int op, int rs, int rt, int imm)  // beq rs, rt, imm   bne
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rs], regs[rt], imm);
    fprintf(output, "\t%s %s, %s, %i\n", mcodes[op], regs[rs], regs[rt], imm);
}

void tocodeL(char type[], int label)     // label
{
    if (label == -1)
    {
        printf("%s:\n", type);
        fprintf(output, "%s:\n", type);
    }
    else
    {
        printf("%s%i:\n", type, label);
        fprintf(output, "%s%i:\n", type, label);
    }
}

void tocodeD(int op, int rs, int rt)  // div rs, rt  частное в lo, остаток в hi
{
    printf("\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
    fprintf(output, "\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
}

void tocodeMF(int op, int rs)  // mfhi rs = lo, mflo
{
    printf("\t%s %s\n", mcodes[op], regs[rs]);
    fprintf(output, "\t%s %s\n", mcodes[op], regs[rs]);
}

void tocodeMOVE(int op, int rs, int rt)
{
    printf("\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
    fprintf(output, "\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
}

void tocodeCondF(int op, int rs, int rt)
{
    printf("\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
    fprintf(output, "\t%s %s, %s\n", mcodes[op], regs[rs], regs[rt]);
}

void tocodeCondFlagF(int op, char type[], int label) // это tocodeJ  ???
{
    printf("\t%s %s%i\n", mcodes[op], type, label);
    fprintf(output, "\t%s %s%i\n", mcodes[op], type, label);
}


void Struct_init_gen(int t, int ld, int lr);
void MExpr_gen();

void mdsp(int displ)
{                                   // displ пришло из extdecl
    // мои глоб переменные начинаются с globinit (сейчас - -8000
    // 5 слов с glodinit вправо занимает bounds
    // 5 слов с globinit-20 занимает stackC0, затем с globinit-40 идет stacki
    // со смещением globinit-60 сидит heap
    // со смещением globinit-64 сидит память для печати регистрового float
    // со смещением globinit-68 сидит память для печати регистрового int

    manst = AMEM;
    if (displ < 0)
    {
        areg = gp;
        adispl = globinit + 8 - displ;
    }
    else
    {
        areg = stp;
        adispl = displ + DISPL0;
    }
}

void MPrimary();

void MASSExpr(int c, int leftanst, int leftdispl, int leftreg)
{
    // слева переменная в регистре (leftanst и leftreg) или в памяти (leftanst, leftdispl, leftreg),  num, areg, masnst - правый операнд
    // справа константа (только для int) или регистр (AREG, areg), но не память
    
    
    // отдельно рассматриваем вещественные числа, CONST не м.б. операндом
    // чистые присваивания реализуются прямо в UNOP и в BINOP
    // не чистое присваивание (с операцией)
    if ((c > ASSR && c <= DIVASSR) || (c > ASSATR && c <= DIVASSATR))
    {
        int rez = mbox <= 2 ? breg : ft0;
        if (leftanst == AMEM)
        {
            tocodeB(lwc1, ft1, leftdispl, leftreg);   // исходное значение
        
            if (c == PLUSASSR || c == PLUSASSATR)
                tocodeR(add_s, rez, ft1, areg);
            else if (c == MINUSASSR || c == MINUSASSATR)
                tocodeR(sub_s, rez, ft1, areg);
            else if (c == MULTASSR || c == MULTASSATR)
                tocodeR(mul_s, rez, ft1, areg);
            else if (c == DIVASSR || c == DIVASSATR)
                tocodeR(div_s, rez, ft1, areg);
            
            tocodeB(swc1, areg = rez, leftdispl, leftreg);
        }
        else
        {
            // leftanst == AREG
            if (c == PLUSASSR || c == PLUSASSATR)
                tocodeR(add_s, leftreg, leftreg, areg);
            else if (c == MINUSASSR || c == MINUSASSATR)
                tocodeR(sub_s, leftreg, leftreg, areg);
            else if (c == MULTASSR || c == MULTASSATR)
                tocodeR(mul_s, leftreg, leftreg, areg);
            else if (c == DIVASSR || c == DIVASSATR)
                tocodeR(div_s, leftreg, leftreg, areg);
            
            if (mbox == BREG && breg != leftreg)
                tocodemove(breg, leftreg);
            areg = leftreg; // ответ есть и в breg, и в leftreg, leftreg лучше
        }
        manst = AREG;
    }
    else
    // здесь уже не вещественные числа
    {
        int flag = 0, opnd;
        int rez = leftanst == AREG ? leftreg : mbox <= 2 ? breg : t0;
        
        if( leftanst == AMEM)
            tocodeB( /*type == LCHAR ? lb : */ lw, opnd = t1, leftdispl, leftreg);
        else
            opnd = leftreg;
        
        if (manst == CONST)
        {
            if (c == SHLASS || c == SHLASSAT)
                tocodeSLR(sll, rez, opnd, num);
            else if (c == SHRASS || c == SHRASSAT)
                tocodeSLR(sra, rez, opnd, num);
            else if (c == ANDASS ||c == ANDASSAT)
                tocodeI(andi, rez, opnd, num);
            else if (c == ORASS || c == ORASSAT)
                tocodeI(ori, rez, opnd, num);
            else if (c == EXORASS || c == EXORASSAT)
                tocodeI(xori, rez, opnd, num);
            else if (c == PLUSASS || c == PLUSASSAT)
                tocodeI(addi, rez, opnd, num);
            else if (c == MINUSASS || c == MINUSASSAT)
                tocodeI(addi, rez, opnd, -num);
            else if (c == MULTASS ||c == MULTASSAT ||
                     c == DIVASS  || c == DIVASSAT ||
                     c == REMASS  || c == REMASSAT)
            {
                tocodeI(addi, t9, d0, num);
                flag = 1;
            }
        }
        
        if (manst == AREG || flag)
        {         // здесь второй операнд - это регистр areg или t9
            int aregt9 = flag == 1 ? t9 : areg;
            if (c == SHLASS || c == SHLASSAT)
                tocodeR(sllv, rez, opnd, areg);
            else if (c == SHRASS || c == SHRASSAT)
                tocodeSLR(srav, rez, opnd, areg);
            else if (c == ANDASS ||c == ANDASSAT)
                tocodeR(and, rez, opnd, areg);
            else if (c == ORASS || c == ORASSAT)
                tocodeR(or, rez, opnd, areg);
            else if (c == EXORASS || c == EXORASSAT)
                tocodeR(xor, rez, opnd, areg);
            else if (c == PLUSASS || c == PLUSASSAT)
                tocodeR(add, rez, opnd, areg);
            else if (c == MINUSASS || c == MINUSASSAT)
                tocodeR(sub, rez, opnd, areg);
            else if (c == MULTASS || c == MULTASSAT)
                tocodeR(mul, rez, opnd, aregt9);
            else if (c == DIVASS || c == DIVASSAT)
            {
                tocodeD(divc, opnd, aregt9);
                tocodeMF(mflo, rez);
            }
            else if (c == REMASS || c == REMASSAT)
            {
                tocodeD(divc, opnd, aregt9);
                tocodeMF(mfhi, rez);
            }
        }
 
        areg = rez;
    if (leftanst == AREG)
    {
        if (mbox <= 2 && leftreg != breg)
            tocodemove(areg = breg, leftreg);
    }
    else   //  if (leftanst == AMEM)
        tocodeB(/* type == LCHAR ? sb : */ sw, rez, leftdispl, leftreg);

    manst = AREG;
    }
}

int structcopy(int type, int ldi, int lreg, int rdi, int rreg)
{
    int n = modetab[type+2], i, t, al, strdi = 0;
    for (i = 0; i < n; i += 2)
    {
        t = modetab[type + 3 + i];
        al = alignm(t);
        ldi += strdi;
        rdi += strdi;
        if (ldi < 0)
            ldi += abs(ldi%al);
        else
            ldi = (ldi+al-1)/al*al;
        
        if (rdi < 0)
            rdi += abs(rdi%al);
        else
            rdi = (rdi+al-1)/al*al;

        if (modetab[t] == MSTRUCT)
            strdi = structcopy(t, ldi, lreg, rdi, rreg);
        else if (t == LINT || (t > 0 && modetab[t] == MPOINT))
        {
            tocodeB(lw, t0, rdi, rreg);
            tocodeB(sw, t0, ldi, lreg);
            strdi += 4;
        }
        else if (t == LCHAR)
        {
            tocodeB(lhu, t0, rdi, rreg);
            tocodeB(sh,  t0, ldi, lreg);
            strdi += 2;
        }
        else if (t == LFLOAT)
        {
            tocodeB(lwc1, ft0, rdi, rreg);
            tocodeB(swc1, ft0, ldi, lreg);
            strdi += 4;
        }
    }
    return strdi;
}

void MBin_operation(int c)      // бинарная операция (два вычислимых операнда)
{
    int oldbox = mbox, oldreg = breg, oldelselab = elselab;
    int leftanst, leftdispl, leftreg, leftnum, rightanst;
    int rez, lopnd, ropnd, flagreal = 2, flagreg = 0;
//    printf("bin form tc= %i mbox= %i manst= %i\n", tc, mbox, manst);
    switch (c)
    {
            case TArassn:
            case TArassni:
            case TArassnc:
        {
            int dim = tree[tc++];
            mbox = BREG;
            leftreg = breg = getreg();
            MExpr_gen();
            breg = a1;
            MExpr_gen();
            tocodemove(a0, leftreg);
            tocodeI(addi, a2, d0, dim);
            if (c == TArassn)
                tocodeJ(jal, "ARASSN", -1);
            else if (c == TArassni)
                    tocodeJ(jal, "ARASSNI", -1);
            else
                tocodeJ(jal, "ARASSNC", -1);
            freereg(leftreg);
            return;
        }

        case COPY00:
        {
            int leftdispl, leftreg;
            mdsp(tree[tc++]);
            leftdispl = adispl;
            leftreg = areg;
            mdsp(tree[tc++]);
            structcopy(tree[tc++], leftdispl, leftreg, adispl, areg);
            return;
        }
            
        case COPY01:
        {
            int type;
            mdsp(tree[tc++]);
            type = tree[tc++];
            tc += 2;           // IDENT
            structdispl = 0;
            Struct_init_gen(type, adispl, areg);
            mbox = oldbox;
            breg = oldreg;
            return;
        }
        case COPY10:
        {
            breg = getreg();
            int rdi, rreg, type;
            mdsp(tree[tc++]);
            rdi = adispl;
            rreg = areg;
            type = tree[tc++];
            mbox = BREGF;
            MExpr_gen();
            structcopy(type, adispl, areg, rdi, rreg);
            mbox = oldbox;
            breg = oldreg;
            freereg(breg);
            return;
        }

        case COPY11:
        {
            int type = tree[tc++];
            int oldbox = mbox, oldreg = breg;
            int flag = 0;
            int rez = mbox == BREGF ? breg : (flag = 1, getreg());
            int leftdispl, leftreg;
            mbox = BREGF;
            leftreg = breg = rez;
            MExpr_gen();
            leftdispl = adispl;
            structdispl = 0;
            Struct_init_gen(type, leftdispl, leftreg);
            mbox = oldbox;
            breg = oldreg;
            if (flag)
                freereg(rez);
            return;
        }
    }

    switch (c)
    {             // простые операции с присваиванием (левый операнд идент или выборка)
        case ASS: // но теперь IDENT тоже операнд, а у этих операций нет операнда
        case ASSR:
        case REMASS:
        case SHLASS:
        case SHRASS:
        case ANDASS:
        case EXORASS:
        case ORASS:
            
        case PLUSASS:
        case MINUSASS:
        case MULTASS:
        case DIVASS:
            
        case PLUSASSR:
        case MINUSASSR:
        case MULTASSR:
        case DIVASSR:
            
        {
            int idp = tree[tc+1], rareg, type;
            tc +=2;
            if (tree[tc-2] == TIdent) // здесь может быть только TIDENT или SELECT
            {
                if (identab[idp] < 0) // только идент может быть регистровой переменной
                {
                    rareg = breg = identab[idp+3];
                    if (c == ASS || c == ASSR)
                    {
                        mbox = BREG;
                        MExpr_gen();                       // правый операнд
                        mbox = oldbox, breg = oldreg, elselab = oldelselab;
                        if (mbox == BREG && breg != rareg)
                            tocodemove(breg, rareg);
                        areg = rareg;
                        manst = AREG;
                        return;
                    }
                    else     // другие операции с присваиванием
                    {
                        mbox = BF;
                        MExpr_gen();              // правый операнд
                        mbox = oldbox, breg = oldreg, elselab = oldelselab;
                        MASSExpr(c, AREG, 0, rareg);
                        return;
                    }
                }
                mdsp(identab[idp+3]);
            }
            else   // TSelect
            {
                mdsp(idp);
                type = tree[tc++];
            }
            leftdispl = adispl; leftreg = areg;
            if (c == ASS || c == ASSR)
            {
               if (mbox > 2)
                   mbox = BF;
               MExpr_gen();                       // правый операнд
               rareg = areg;
               mbox = oldbox, breg = oldreg, elselab = oldelselab;
               if (manst == CONST)
               {
                   rareg = mbox <= 2 ? breg : t0;
                   tocodeI(addi, rareg, d0, num);
               }
               tocodeB(c == ASSR ? swc1 : sw, rareg, leftdispl, leftreg);
               if (mbox == BREG /*&& manst == AREG*/ && breg != rareg)
                   tocodemove(breg, rareg);
               areg = rareg;
               manst = AREG;
               return;
            }
            else
            {
               mbox = BF;
               MExpr_gen();              // правый операнд, справа не может быть AMEM
               mbox = oldbox, breg = oldreg, elselab = oldelselab;
               MASSExpr(c, AMEM, leftdispl, leftreg);
               return;
            }
        }
    }
    switch (c)
    {
        case ASSAT:
        case ASSATR:
        {
            int rareg;
            breg = getreg();
            mbox = BREGF;
            MExpr_gen();           //  левый операнд (куда)
            leftdispl = adispl;
            leftreg = areg;
            mbox = BF;
            MExpr_gen();           // правый операнд (что)
            mbox = oldbox; breg = oldreg; elselab = oldelselab;
            rareg = areg;
            if (c == ASSAT)
            {
                if (manst == CONST)
                {
                    rareg = mbox <= 2 ? breg : t0;
                    tocodeI(addi, rareg, d0, num);
                }
                tocodeB(sw, rareg, leftdispl, leftreg);
            }
            else
                tocodeB(swc1, rareg, leftdispl, leftreg);

            if (mbox == BREG && breg != rareg)
                tocodemove(breg, rareg);

            manst = AREG;
            freereg(breg);
            return;
        }
            
        case REMASSAT:
        case SHLASSAT:
        case SHRASSAT:
        case ANDASSAT:
        case EXORASSAT:
        case ORASSAT:
            
        case PLUSASSAT:
        case MINUSASSAT:
        case MULTASSAT:
        case DIVASSAT:
            flagreal = 0;
            break;
            
        case PLUSASSATR:
        case MINUSASSATR:
        case MULTASSATR:
        case DIVASSATR:
           
            flagreal = 1;
    }

    if (flagreal < 2)
    {                         // последний кусок с присваиванием
        breg = getreg();
        mbox = BREGF;
        MExpr_gen();           //  левый операнд (куда)
        leftanst = manst;
        leftdispl = adispl;
        leftreg = areg;
        mbox = BF;
        MExpr_gen();           // правый операнд (что)
        mbox = oldbox; breg = oldreg; elselab = oldelselab;
        MASSExpr(c, leftanst, leftdispl, leftreg);
        freereg(breg);
        return;
    }

    flagBC = 1;
    if (mbox == BCF || mbox == BCT)
    {
        int thenlab = labnum++;
        if (c == LOGAND || c == LOGOR)
        {
            tc++;
            if (c == LOGAND)
            {
                if (mbox == BCF)
                    MExpr_gen();       // левое выражение
                else
                {
                    elselab = thenlab;
                    mbox = BCF;
                    MExpr_gen();
                    elselab = oldelselab;
                }
            }
            else  // LOGOR
            {
                if (mbox == BCF)
                {
                    elselab = thenlab;
                    mbox = BCT;
                    MExpr_gen();          // левое выражение
                    elselab = oldelselab;
                }
                else
                    MExpr_gen();
            }
            mbox = oldbox;
            if (tree[tc] == ADLOGAND || tree[tc] == ADLOGOR)
                tc += 2;

            MExpr_gen();           // правое выражение
        tocodeL("ELSE", thenlab);
            flagBC = 0;
            return;
        }
   
        switch (c)
        {
            case EQEQR:
            case NOTEQR:
            case LLTR:
            case LGTR:
            case LLER:
            case LGER:
                flagreal = 1;
                break;
            default:
                flagreal = 0;
        }
    
        rez = mbox <= BREGF ?  breg :
        (mbox = BREGF, flagreg = 1,
         breg = flagreal ? getregf() : getreg());
        lopnd = breg;
        if (flag_cond_cycle != 2)
        {
        	MExpr_gen();                                        // левый операнд
        	cond_cycle_end_manst_left = manst;
        }
        else
        {
        	manst = cond_cycle_end_manst_left;
        	cond_cycle_end_manst_left = 0;
        }
        if (manst == AREG)
        {
            lopnd = areg;
            if (mbox ==  BREGF)
                rez = areg;
        }
        if (flag_cond_cycle == 2 && manst == AREG)
        	lopnd = cond_cycle_end_left_reg;
        leftanst = manst;
        leftdispl = adispl;
        leftreg = areg;
        leftnum = num;
        if (left_reg_cond == 0)
        {
        	left_reg_cond = lopnd;
        }
        else if (left_reg_cond != -1)
        {
        	lopnd = left_reg_cond;
        	left_reg_cond = -1;
        }

        mbox = BF;
        if (flag_cond_cycle != 2)
        {
        	MExpr_gen();                                        // правый операнд
        	if (manst == AREG && cycle_condition_calculation)
        		tocodemove(oldreg, areg);
        	cond_cycle_end_manst = manst;
        }
        else
        {
        	manst = cond_cycle_end_manst;
        	cond_cycle_end_manst = 0;
        }

        if (flagreg)
        {
            if (flagreal)
                freeregf(lopnd);
            else
                freereg(lopnd);
        }
        rightanst = manst;
        ropnd = areg;      // правый операнд может быть только регистром или константой

        mbox = oldbox; breg = oldreg; elselab = oldelselab;
    
        switch (c)
        {
            case EQEQ:
            case NOTEQ:
                if (leftanst == CONST && manst == CONST)
                {
                    tocodeI(addi, t1, d0, leftnum == num ? 1 : 0);
                    tocodeJC( mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                if (leftanst == CONST && manst == AREG)
                    tocodeI(addi, lopnd, d0, leftnum);
                else if (leftanst == AREG && manst == CONST)
                    tocodeI(addi, lopnd, d0, num);
                
                // leftanst == AREG && anst == AREG
                if (c == EQEQ)
                    tocodeJEQ(mbox == BCF ? bne : beq, lopnd, ropnd, "ELSE", elselab);
                else
                    tocodeJEQ(mbox == BCF ? beq : bne, lopnd, ropnd, "ELSE", elselab);
                flagBC = 0;
                return;
                
            case EQEQR:
            case NOTEQR:
                if (c == EQEQR)
                {
                	tocodeCondF(c_seq_s, lopnd, ropnd);
                	tocodeCondFlagF(mbox == BCF ? bc1f : bc1t, "ELSE", elselab);
                }
                else
                {
                	tocodeCondF(c_seq_s, lopnd, ropnd);
                	tocodeCondFlagF(mbox ==BCF ? bc1t : bc1f, "ELSE", elselab);
                }
                flagBC = 0;
                return;
                
            case LLT:
            case LGT:
            case LLE:
            case LGE:
                if (leftanst == CONST && manst == CONST)
                {
                    tocodeI(addi, t1, d0,
                            c == LLT ? leftnum < num ? 1 : 0 :
                            c == LGT ? leftnum > num ? 1 : 0 :
                            c == LLE ? leftnum <= num ? 1 : 0 :
                                       leftnum >= num ? 1 : 0);
                    tocodeJC(mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                if (leftanst == CONST && manst == AREG)
                {
                	if (flag_cond_cycle == 0)
                		tocodeI(addi, t1, ropnd, -leftnum);
                	else if (flag_cond_cycle == 1)
                		tocodeI(addi, cond_cycle_end_left_reg = lopnd = breg, d0, num);
                	else
                		lopnd = breg;

                    if (flag_jump_end_cycle == 0)
                    {
						if (c == LLT)
						{
							if (flag_cond_cycle)
								tocodeJEQ(ble, ropnd, lopnd, "ELSE", elselab);
							else
								tocodeJC(mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
						}
						else if (c == LGT)
						{
							if (flag_cond_cycle)
								tocodeJEQ(bge, ropnd, lopnd, "ELSE", elselab);
							else
								tocodeJC(mbox == BCF ? bgez : bltz, t1, "ELSE", elselab);
						}
						else if (c == LLE)
						{
							if (flag_cond_cycle)
								tocodeJEQ(blt, ropnd, lopnd, "ELSE", elselab);
							else
								tocodeJC(mbox == BCF ? bltz : bgez, t1, "ELSE", elselab);
						}
						else
							if (flag_cond_cycle)
								tocodeJEQ(bgt, ropnd, lopnd, "ELSE", elselab);
							else
								tocodeJC(mbox == BCF ? bgtz : blez, t1, "ELSE", elselab);
                    }
                    else if (flag_jump_end_cycle == 1)
                    {
    					if (c == LLT)
    					{
							if (flag_cond_cycle)
								tocodeJEQ(bne, ropnd, lopnd, "BEGLOOP", adcont);
							else
								tocodeJC(bgtz, t1, "BEGLOOP", adcont);
    					}
						else if (c == LGT)
						{
							if (flag_cond_cycle)
								tocodeJEQ(bne, ropnd, lopnd, "BEGLOOP", adcont);
							else
								tocodeJC(bltz, t1, "BEGLOOP", adcont);
						}
						else if (c == LLE)
						{
							if (flag_cond_cycle)
								tocodeJEQ(bge, ropnd, lopnd, "BEGLOOP", adcont);
							else
								tocodeJC(bgez, t1, "BEGLOOP", adcont);
						}
						else
						{
							if (flag_cond_cycle)
								tocodeJEQ(ble, ropnd, lopnd, "BEGLOOP", adcont);
							else
								tocodeJC(blez, t1, "BEGLOOP", adcont);
						}
                    }
                    flagBC = 0;
                    return;
                }
                
                if (leftanst == AREG && manst == CONST)
                {
                	if (flag_cond_cycle == 0)
                		tocodeI(addi, t1, lopnd, -num);
                	else if (flag_cond_cycle == 1)
                		tocodeI(addi, ropnd = breg, d0, num), cond_cycle_end_left_reg = lopnd;
                	else
                		ropnd = breg;
                }
                else
                    // leftanst == AREG && anst == AREG
                	if (!flag_cond_cycle)
                		tocodeR(sub, t1, lopnd, ropnd);
                	else
                		ropnd = breg, cond_cycle_end_left_reg = lopnd;
                	
                if (flag_jump_end_cycle == 0)
                {
					if (c == LLT)
					{
						if (flag_cond_cycle)
							tocodeJEQ(bge, lopnd, ropnd, "ELSE", elselab);
						else
							tocodeJC(mbox == BCF ? bgez : bltz, t1, "ELSE", elselab);
					}
					else if (c == LGT)
					{
						if (flag_cond_cycle)
							tocodeJEQ(ble, lopnd, ropnd, "ELSE", elselab);
						else
							tocodeJC(mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
					}
					else if (c == LLE)
					{
						if (flag_cond_cycle)
							tocodeJEQ(bgt, lopnd, ropnd, "ELSE", elselab);
						else
							tocodeJC(mbox == BCF ? bgtz : blez, t1, "ELSE", elselab);
					}
					else
					{
						if (flag_cond_cycle)
							tocodeJEQ(blt, lopnd, ropnd, "ELSE", elselab);
						else
							tocodeJC(mbox == BCF ? bltz : bgez, t1, "ELSE", elselab);
					}
                }
                else if (flag_jump_end_cycle == 1)
                {
					if (c == LLT)
					{
						if (flag_cond_cycle == 2)
							tocodeJEQ(bne, lopnd, ropnd, "BEGLOOP", adcont);
						else
							tocodeJC(bltz, t1, "BEGLOOP", adcont);
					}
					else if (c == LGT)
					{
						if (flag_cond_cycle == 2)
							tocodeJEQ(bne, lopnd, ropnd, "BEGLOOP", adcont);
						else
							tocodeJC(bgtz, t1, "BEGLOOP", adcont);
					}
					else if (c == LLE)
					{
						if (flag_cond_cycle == 2)
							tocodeJEQ(ble, lopnd, ropnd, "BEGLOOP", adcont);
						else
							tocodeJC(blez, t1, "BEGLOOP", adcont);
					}
					else
					{
						if (flag_cond_cycle == 2)
							tocodeJEQ(bge, lopnd, ropnd, "BEGLOOP", adcont);
						else
							tocodeJC(bgez, t1, "BEGLOOP", adcont);
					}
                }
                flagBC = 0;
                return;
                
            case LLTR:
            case LGTR:
            case LLER:
            case LGER:
            	if (c == LLTR)
            	{
                    tocodeCondF(c_lt_s, lopnd, ropnd);
                    tocodeCondFlagF(mbox == BCF ? bc1f : bc1t, "ELSE", elselab);
                }
                else if (c == LGTR)
                {
                    tocodeCondF(c_le_s, lopnd, ropnd);
                    tocodeCondFlagF(mbox == BCF ? bc1t : bc1f, "ELSE", elselab);
                }
                else if (c == LLER)
                {
                    tocodeCondF(c_le_s, lopnd, ropnd);
                    tocodeCondFlagF(mbox == BCF ? bc1f : bc1t, "ELSE", elselab);
                }
                else
                {
                    tocodeCondF(c_lt_s, lopnd, ropnd);
                    tocodeCondFlagF(mbox == BCF ? bc1t : bc1f, "ELSE", elselab);
                }
                flagBC = 0;
                return;
        }
    }             // конец mbox == ВС
    
    switch (c)
    {
        case EQEQR:
        case NOTEQR:
        case LLTR:
        case LGTR:
        case LLER:
        case LGER:
        case LPLUSR:
        case LMINUSR:
        case LMULTR:
        case LDIVR:
            flagreal = 1;
            break;
        default:
            flagreal = 0;
    }

    rez = mbox <= BREGF ?  breg :
    (mbox = BREGF, flagreg = 1,
     breg = flagreal ? getregf() : getreg());
    
    if (c == LOGAND || c == LOGOR)
    {
        int oldbox = mbox, oldelselab = elselab;
        tc++;
        elselab = labnum++;
        if (c == LOGAND)
        {
            tocodeI(addi, rez, d0, 0);
            mbox = BCF;
            MExpr_gen();
            if (tree[tc] == ADLOGAND || tree[tc] == ADLOGOR)
                tc += 2;
            MExpr_gen();
            tocodeI(addi, rez, d0, 1);
        }
        else        //  if (c == LOGOR)
        {
            tocodeI(addi, rez, d0, 1);
            mbox = BCT;
            MExpr_gen();
            if (tree[tc] == ADLOGAND || tree[tc] == ADLOGOR)
                tc += 2;
            MExpr_gen();
            tocodeI(addi, rez, d0, 0);
        }
        tocodeL("ELSE", elselab);
        if (flagreg)
            freereg(rez);
        mbox = oldbox; elselab = oldelselab;
        manst = AREG;
        areg = rez;
        return;
    }
    
    lopnd = breg;
    MExpr_gen();                                        // левый операнд
    if (manst == AREG)
    {
        lopnd = areg;
        if (mbox ==  BREGF)
            rez = breg;
    }
    leftanst = manst;
    leftdispl = adispl;
    leftreg = areg;
    leftnum = num;

    mbox = BF;
    MExpr_gen();                                        // правый операнд
    
    if (flagreg)
    {
        if (flagreal)
            freeregf(lopnd);
        else
            freereg(lopnd);
    }
    rightanst = manst;
    ropnd = areg;      // правый операнд может быть только регистром или константой

    mbox = oldbox; breg = oldreg; elselab = oldelselab;

    manst = AREG;
    areg = rez;
    if (leftanst == CONST && rightanst == CONST)
    {
        switch (c)
        {
            case LREM:
                num = leftnum % num;
                break;
            case LSHL:
                num = leftnum << num;
                break;
            case LSHR:
                num = leftnum >> num;
                break;
            case LAND:
                num = leftnum & num;
                break;
            case LEXOR:
                num = leftnum ^ num;
                break;
            case LOR:
                num = leftnum | num;
                break;
            case LPLUS:
                num += leftnum;
                break;
            case LMINUS:
                num = leftnum - num;
                break;
            case LMULT:
                num *= leftnum;
                break;
            case LDIV:
                num = leftnum / num;
                break;
            case EQEQ:
                num = leftnum == num;
                break;
            case NOTEQ:
                num = leftnum != num;
                break;
            case LLT:
                num = leftnum < num;
                break;
            case LGT:
                num = leftnum > num;
                break;
            case LLE:
                num = leftnum <= num;
                break;
            case LGE:
                num = leftnum >= num;
                break;
        }
      
        if (abs(num) < 32768)
            if (mbox == BREG)
                tocodeI(addi, rez, d0, num);
            else
                manst = CONST;
        else
            tocodeLI(li, rez, num);
        return;
    }
    
    if (leftanst == CONST && rightanst == AREG)
    {
        switch (c)
        {
            case LREM:
                tocodeI(addi, t1, d0, leftnum);
                tocodeD(divc, t1, ropnd);
                tocodeMF(mfhi, rez);
                return;
            case LAND:
                tocodeI(andi, rez, ropnd, leftnum);
                return;
            case LEXOR:
                tocodeI(xori, rez, ropnd, leftnum);
                return;
            case LOR:
                tocodeI(ori, rez, ropnd, leftnum);
                return;
            case LPLUS:
                tocodeI(addi, rez, ropnd, leftnum);
                return;
            case LMULT:
                tocodeI(addi, t0, d0, leftnum);
                tocodeR(mul, rez, ropnd, t0);
                return;
            case LDIV:
                tocodeI(addi, t1, d0, leftnum);
                tocodeD(divc, t1, ropnd);
                tocodeMF(mflo, rez);
                return;
            case EQEQ:
                tocodeI(addi, t1, d0, leftnum);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(beq, ropnd, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case NOTEQ:
                tocodeI(addi, t1, d0, leftnum);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(bne, ropnd, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
                                       // некоммутативные команды
            case LSHL:
                tocodeI(addi, rez, d0, leftnum);
                tocodeSLR(sllv, rez, rez, ropnd);
                return;
            case LSHR:
                tocodeI(addi, rez, d0, leftnum);
                tocodeSLR(srav, rez, rez, ropnd);
                return;
            case LLT:
                tocodeI(slti, rez, ropnd, leftnum+1);
                tocodeI(xori, rez, rez, 1);
                return;
            case LGT:
                tocodeI(slti, rez, ropnd, leftnum);
                return;
            case LLE:
                tocodeI(slti, rez, ropnd, leftnum);
                tocodeI(xori, rez, rez, 1);
                return;
            case LGE:
                tocodeI(slti, rez, ropnd, leftnum+1);
                return;
            case LMINUS:
                tocodeI(addi, rez, ropnd, -leftnum);
                tocodeR(sub, rez, d0, rez);
                return;
        }
    }
    
    if (leftanst == AREG && rightanst == CONST)
    {
        switch (c)
        {
            case LREM:
                tocodeI(addi, t0, d0, num);
                tocodeD(divc, lopnd, t0);
                tocodeMF(mfhi, rez);
                return;
            case LSHL:
                tocodeSLR(sll, rez, lopnd, num);
                return;
            case LSHR:
                tocodeSLR(sra, rez, lopnd, num);
                return;
            case LAND:
                tocodeI(andi, rez, lopnd, num);
                return;
            case LEXOR:
                tocodeI(xori, rez, lopnd, num);
                return;
            case LOR:
                tocodeI(ori, rez, lopnd, num);
                return;
            case LPLUS:
                tocodeI(addi, rez, lopnd, num);
                return;
            case LMINUS:
                tocodeI(addi, rez, lopnd, -num);
                return;
            case LMULT:
                tocodeI(addi, t0, d0, num);
                tocodeR(mul, rez, lopnd, t0);
                return;
            case LDIV:
                tocodeI(addi, t0, d0, num);
                tocodeD(divc, lopnd, t0);
                tocodeMF(mflo, rez);
                return;
            case EQEQ:
                tocodeI(addi, t1, d0, num);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(beq, lopnd, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case NOTEQ:
                tocodeI(addi, t1, d0, num);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(bne, lopnd, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case LLT:
                tocodeI(slti, rez, lopnd, num);
                return;
            case LGT:
                tocodeI(slti, rez, lopnd, num+1);
                tocodeI(xori, rez, rez, 1);
                return;
            case LLE:
                tocodeI(slti, rez, lopnd, num+1);
                return;
            case LGE:
                tocodeI(slti, rez, lopnd, num);
                tocodeI(xori, rez, rez, 1);
                return;
        }
    }
    
    if (leftanst == AREG && rightanst == AREG)
    {
        switch (c)
        {
            case LREM:
                tocodeD(divc, lopnd, ropnd);
                tocodeMF(mfhi, rez);
                return;
            case LSHL:
                tocodeR(sllv, rez, lopnd, ropnd);
                return;
            case LSHR:
                tocodeR(srav, rez, lopnd, ropnd);
                return;
            case LAND:
                tocodeR(and, rez, lopnd, ropnd);
                return;
            case LEXOR:
                tocodeR(xor, rez, lopnd, ropnd);
                return;
            case LOR:
                tocodeR(or, rez, lopnd, ropnd);
                return;
            case LPLUS:
                tocodeR(add, rez, lopnd, ropnd);
                return;
            case LPLUSR:
                tocodeR(add_s, rez, lopnd, ropnd);
                return;
            case LMINUS:
                tocodeR(sub, rez, lopnd, ropnd);
                return;
            case LMINUSR:
                tocodeR(sub_s, rez, lopnd, ropnd);
                return;
            case LMULT:
                tocodeR(mul, rez, lopnd, ropnd);
                return;
            case LMULTR:
            	tocodeR(mul_s, rez, lopnd, ropnd);
                return;
            case LDIV:
                tocodeD(divc, lopnd, ropnd);
                tocodeMF(mflo, rez);
                return;
            case LDIVR:
            	tocodeR(div_s, rez, lopnd, ropnd);
                return;
                
            case EQEQ:
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(beq, lopnd, ropnd, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case EQEQR:
            	tocodeCondF(c_seq_s, lopnd, ropnd);
            	log_real = 1;
            	break;
            case NOTEQ:
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(bne, lopnd, ropnd, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case NOTEQR:
            	tocodeCondF(c_seq_s, lopnd, ropnd);
            	log_real = 0;
            	break;
            case LLT:
                tocodeR(slt, rez, lopnd, ropnd);
                return;
            case LLTR:
            	tocodeCondF(c_lt_s, lopnd, ropnd);
            	log_real = 1;
            	break;
            case LGT:
                tocodeR(slt, rez, ropnd, lopnd);
                return;
            case LGTR:
            	tocodeCondF(c_le_s, lopnd, ropnd);
            	log_real = 0;
            	break;
            case LLE:
                tocodeR(slt, rez, ropnd, lopnd);
                tocodeI(xori, rez, rez, 1);
                return;
            case LLER:
            	tocodeCondF(c_le_s, lopnd, ropnd);
            	log_real = 1;
            	break;
            case LGE:
                tocodeR(slt, rez, lopnd, ropnd);
                tocodeI(xori, rez, rez, 1);
                return;
            case LGER:
            	tocodeCondF(c_lt_s, lopnd, ropnd);
            	log_real = 0;
        }
        tocodeI(addi, rez, d0, 1);
        tocodeCondFlagF(log_real ? bc1t : bc1f, "ELSE", labnum);
        tocodeI(addi, rez, d0, 0);
    tocodeL("ELSE", labnum++);
    }
}

void BCend()
{
    if (mbox == BCF || mbox == BCT)
        tocodeJEQ(mbox == BCF ? beq : bne, areg, d0, "ELSE", elselab);
}

void MUnar_expr(int c)
{
    int rez, opnd, oldbox = mbox, oldreg = breg, oldelselab = elselab;
    switch (c)
    {
        case TDYNSelect:
        {
            int d = tree[tc++];
            MExpr_gen();
            manst =AMEM;
            adispl += d;
            return;
        }
        case POSTINC:
        case INC:
        case POSTDEC:
        case DEC:
            
        case POSTINCR:
        case INCR:
        case POSTDECR:
        case DECR:
        {
            int idp = tree[tc+1], rareg, rez, type;
            tc +=2;
            if (tree[tc-2] == TIdent) // здесь может быть только TIDENT или SELECT
            {
                if (identab[idp] < 0) // только идент может быть регистровой переменной
                {
                    rareg = breg = identab[idp+3];
                    if (c == INC || c == POSTINC)
                    {
                    	if (delay_slot && delay_slot_inc == 0)
                    	{
                    		delay_slot_inc = 1;
                    		return;
                    	}

                        if (c == POSTINC)
                            tocodemove(t0, rareg);
                        tocodeI(addi, rareg, rareg, 1);
                        manst = AREG;
                        areg = c == POSTINC ? t0 : rareg;
                        return;
                    }
                    else if (c == POSTINCR || c == INCR)
                    {
                        if (c == POSTINCR)
                            tocodemove(ft0, rareg);
                        tocodeLI_S(li_s, ft1, 1.000000);
                        tocodeR(add_s, rareg, rareg, ft1);
                        manst = AREG;
                        areg = c == POSTINCR ? ft0 : rareg;
                        return;
                    }
                    else if (c == DEC || c == POSTDEC)
                    {
                    	if (delay_slot && delay_slot_inc == 0)
                    	{
                    		delay_slot_inc = -1;
                    		return;
                    	}

                        if (c == POSTDEC)
                            tocodemove(t0, rareg);
                        tocodeI(addi, rareg, rareg, -1);
                        manst = AREG;
                        areg = c == POSTDEC ? t0 : rareg;
                        return;
                    }
                    else if (c == DECR || c == POSTDECR)
                    {
                        if (c == POSTDECR)
                            tocodemove(ft0, rareg);
                        tocodeLI_S(li_s, ft1, -1.000000);
                        tocodeR(add_s, rareg, rareg, ft1);
                        manst = AREG;
                        areg = c == POSTDECR ? ft0 : rareg;
                        return;
                    }
                }
                mdsp(identab[idp+3]);
            }
            else   // TSelect
            {
                mdsp(idp);
                type = tree[tc++];
            }
            
            rez = mbox <= BREGF ? breg : t0;
            if (c == INC || c == POSTINC)
            {
               tocodeB(lw, rez, adispl, areg);
               tocodeI(addi, t1, rez, 1);
               tocodeB(sw, t1, adispl, areg);
               manst = AREG;
               areg = c == INC ? t1 : rez;
               return;
            }
            else if (c == INCR || c == POSTINCR)
            {
                rez = mbox <= BREGF ? breg : ft0;
               tocodeB(lwc1, rez, adispl, areg);
               tocodeLI_S(li_s, ft1, 1.000000);
               tocodeR(add_s, ft1, rez, ft1);
               tocodeB(swc1, ft1, adispl, areg);
               manst = AREG;
               areg = c == INCR ? ft1 : rez;
               return;
            }
            else if (c == DEC || c == POSTDEC)
            {
                rez = mbox <= BREGF ? breg : t0;
               tocodeB(lw, rez, adispl, areg);
               tocodeI(addi, t1, rez, -1);
               tocodeB(sw, t1, adispl, areg);
               manst = AREG;
               areg = c == DEC ? t1 : rez;
               return;
            }
            else if (c == DECR || c == POSTDECR)
            {
                rez = mbox <= BREGF ? breg : ft0;
                tocodeB(lwc1, rez, adispl, areg);
                tocodeLI_S(li_s, ft1, -1.000000);
                tocodeR(add_s, ft1, rez, ft1);
                tocodeB(swc1, ft1, adispl, areg);
                manst = AREG;
                areg = c == DECR ? ft1 : rez;
                return;
            }
        }

        case POSTINCAT:
        case INCAT:
        case POSTDECAT:
        case DECAT:
            mbox = BF;
            MExpr_gen();                  // здесь точно будет manst == AMEM
            rez = mbox <= BREGF ? breg : t0;
            tocodeB(lw, rez, adispl, areg);
            tocodeI(addi, t1, rez, c == INCAT || c == POSTINCAT ? 1 : -1);
            tocodeB(sw, t1, adispl, areg);
            areg = c == INCAT || c == DECAT ? t1 : rez;
            return;
            
        case POSTINCATR:
        case INCATR:
        case POSTDECATR:
        case DECATR:
            {
                rez = mbox <= BREGF ? breg : ft0;
                tocodeB(lwc1, rez, adispl, areg);
                tocodeLI_S(li_s, ft1, c == INCAT || c == POSTINCAT ? 1.000000 : -1.000000);
                tocodeR(add_s, ft1, rez, ft1);
                tocodeB(swc1, ft1, adispl, areg);
                manst = AREG;
                areg = c == INCATR || c == DECATR ? ft1 : rez;
                return;
            }

        case TAddrtoval:
        case TAddrtovalc:
             mbox = BF;
             MExpr_gen();          // сейчас адрес в регистре areg
             rez = oldbox <= BREGF ? oldreg : t0;
             tocodeB(c == TAddrtoval ? lw : lhu, rez, 0, areg);
             manst = AREG;
             areg = rez;
             BCend();
             break;
              
          case TAddrtovalf:
             mbox = BF;
             MExpr_gen();            // сейчас адрес в регистре areg
             rez = oldbox <= BREGF ? oldreg : ft0;
             tocodeB(lwc1, rez, 0, areg);
             manst = AREG;
             areg = rez;
             break;
            
         case TPrint:
         {
             int t = tree[tc++], rez;
             mbox = BREG;
             rez = breg = t == LFLOAT ? fa0 : a0;
             MExpr_gen();
            if (t == LFLOAT)
            {
                tocodeB(swc1, rez, -8064, gp);
                tocodeI(addi, a0, gp, -8064);
            }
            else if (t == LINT || t == LCHAR)
            {
                tocodeB(t == LCHAR ? sh : sw, rez, -8068, gp);
                tocodeI(addi, a0, gp, -8068);
            }
             else if (t > 0 && modetab[t] == MSTRUCT)
                 tocodeI(addi, a0, areg, adispl);
             else //MARRAY
                 tocodeB(lw, a0, adispl, areg);
                 
			tocodeI(addi, a1, d0, t);
            tocodemove(a2, d0);
            tocodeI(addi, a3, d0, '\n');
            tocodeJ(jal, "auxprint", -1);
        }
            break;

		case TGet:
		{
			int t = tree[tc++], rez;
			mbox = BREG;
			rez = breg = a1;
			MExpr_gen();
			if (t > 0 && modetab[t] == MSTRUCT)
				tocodeI(addi, a1, areg, adispl);
			else if (t > 0 && modetab[t] == MARRAY)
				tocodeB(lw, a1, adispl, areg);
			
			if (t > 0 && modetab[t] == MPOINT)
				t = modetab[t+1];
			tocodeI(addi, a0, d0, t);
		    tocodeJ(jal, "auxget", -1);
	   }
		   break;

        case WIDEN:
        case WIDEN1:
            mbox = BF;
            MExpr_gen();
            rez = oldbox <= 2 ? oldreg : ft0;
            if (manst == CONST)
            {
                numdouble = num;
                tocodeLI_S(li_s, areg = rez, numdouble);
            }
            else
            {
                if (manst == AMEM)
                    tocodeB(lw, opnd = t0, adispl, areg);
                else         // manst == AREG
                    opnd = areg;
                tocodeMOVE(mtc1, opnd, rez);
                tocodeMOVE(cvt_s_w, areg = rez, rez);
            }
            manst = AREG;
            break;

        case LNOT:          // поразрядное отрицание:
        case LOGNOT:
        case UNMINUS:
        {
            int r = mbox == BREG ? breg : t0;
            int flag = 1;
            mbox = BF;
            MExpr_gen();
            if (manst == CONST)
            {
                if (c == LNOT)
                    flag = 0, num = ~num;
                else if (c == LOGNOT)
                    num = !num;
                else                 // UNMINUS
                    num = -num;
                
                if (flag)
                    tocodeI(addi, r, d0, num);
                else
                    tocodeLI(li, r, num);
            }
            else                     // AMEM или AREG
            {
                if (manst == AMEM)
                    tocodeB(lw, r, adispl, areg);
                
                if (c == LNOT)
                    tocodeR(nor, r, r, d0);
                else if (c == LOGNOT)
                    tocodeI(slti, r, r, 1);
                else                 // UNMINUS
                    tocodeR(sub, r, d0, r);
            }
            areg = r;
        }
        manst = AREG;
    }
    mbox = oldbox; breg = oldreg; elselab = oldelselab;
}

void MExpr_gen()
{
    int c = tree[tc++];
    if ((mbox == BCF || mbox == BCT) && c == LOGNOT)
        mbox = mbox == BCF ? BCT : BCF;
    if (c > 10000)
        MBin_operation(c -= 1000); // бинарная операция (два вычислимых операнда)
    else if (c > 9000)
        MUnar_expr(c);             // унарная операция  (один вычислимый операнд)
    else
       tc--, MPrimary();
    if (tree[tc] == NOP)
        tc++;
    if (tree[tc] == TExprend)
    {
        tc++;
    }
}

void MDeclid_gen();

void MStmt_gen();

void endslice(int t)
{
	int d = szof(t);
	if (d == 2)
		tocodeJ(jal, "SLICE2", -1); // v0 - адрес i-го элемента
	else if (d == 4)
		tocodeJ(jal, "SLICE4", -1); // v0 - адрес i-го элемента
	else
	{
		tocodeI(addi, a2, d0,  d);  // a2 - это шаг
		tocodeJ(jal, "SLICE", -1);  // v0 - адрес i-го элемента
	}
	manst = AMEM;
	adispl = 0;
	areg = v0;
}

void endslice0(int t, int r, int rez)
{
	int d = szof(t);
	if (d == 2)
		tocodeSLR(sll, t1, r, 1);
	else if (d == 4)
		tocodeSLR(sll, t1, r, 2);
	else
	{
		tocodeI(addi, t1, d0,  d);     // d - это шаг
		tocodeR(mul, t1, r, t1);
	}
	tocodeR(add, rez, rez, t1);        // C0 + i * d
	manst = AMEM;
	areg = r;
	adispl = 0;
}

void MPrimary()
{
    int oldbox = mbox, oldreg = breg, oldelselab = elselab, treecode;
    int rez, d, idp;
//    printf("MPrimary tc= %i tree[tc]= %i\n", tc, tree[tc]);
    switch (treecode =
			tree[tc++])
        {
            case TCondexpr:
            {
                int labend, curelse = elselab = labnum++;
                int oldbox = mbox;
                labend = labnum++;
                mbox = BCF;
                MExpr_gen();
                mbox = oldbox;
                MExpr_gen();
                tocodeJ(jump, "END", labend);
                tocodeL("ELSE", curelse);
                MExpr_gen();
                tocodeL("END", labend);
                areg = mbox == BF ? t0 : breg;
                mbox = AREG;
            }
                break;
            case TIdent:
                  idp = tree[tc++];
                manst = AMEM;
                mdsp(identab[idp+3]);
                if (mbox <= 2)
                    tocodeB(lw, breg, adispl, areg);
                break;
            case TIdenttoaddr:
                mdsp(identab[tree[tc++]+3]);
                rez = mbox <= BREGF ? breg : t0;
                tocodeI(addi, rez, areg, adispl);
                manst = AREG;
                areg = rez;
                break;
            case TIdenttoval:
            case TIdenttovalc:
            case TIdenttovalf:
            {
                int type;
                idp = tree[tc++];
                type = identab[idp+2];
                if (type > 0 && modetab[type] == MPOINT)
                    type = modetab[type+1];
                if (identab[idp] < 0)
                {
                    areg = identab[idp+3];
                    if (mbox == BREG && breg != areg)
                        tocodemove(breg, areg), areg = breg;
                    manst = AREG;
                    break;
                }
                mdsp(identab[idp+3]);
                rez = mbox <= BREGF ? breg : treecode ==TIdenttovalf ?ft0 : t0;
                tocodeB(treecode == TIdenttoval ? lw :
                        treecode == TIdenttovalf ? lwc1 : lhu, rez, adispl, areg);
                areg = rez;
//                  printf("idtoval adispl= %i areg= %i manst= %i\n", adispl, areg, manst);
                areg = rez;
                if (type > 0 && modetab[type] == MSTRUCT)
                {
                    manst = AMEM;
                    adispl = 0;
                }
                else
                    manst = AREG;
                BCend();
            }
                break;
                
            case TConst:
            case TConstc:
                manst = CONST;
                num = tree[tc++];
                if (abs(num) >= 32768)
                {
                    tocodeLI(li, areg = mbox <= 2 ? breg : t0, num);
                    manst = AREG;
                }
                else if (mbox == BREG)
                {
                    tocodeI(addi, areg = breg, d0, num);
                    manst = AREG;
                }
                if (mbox == BCF || mbox == BCT)
                {
                    if (manst == CONST)
                    {
                        if ((mbox == BCT && num != 0) || (mbox == BCF && num == 0))
                               tocodeJ(jump, "ELSE", elselab);
                    }
                    else
                        tocodeJEQ(mbox == BCF ? beq : bne, areg, d0, "ELSE", elselab);
                }
                break;
                
                case TConstf:
                    memcpy(&numf, &tree[tc++], sizeof(float));
                    if (mbox == BCF || mbox == BCT)
                    {
                        if ((mbox == BCT && numf != 0) ||
                            (mbox == BCF && numf == 0))
                               tocodeJ(jump, "ELSE", elselab);
                    }
                    else
                    {
                        manst = AREG;
                        areg = mbox <= 2 ? breg : ft0;
                        tocodeLI_S(li_s, areg, numf);
                    }
                    break;
            case TConstd:
            	areg = mbox <= 2 ? breg : ft0;
                memcpy(&numdouble, &tree[tc], sizeof(double));
                tc += 2;
                tocodeLI_S(li_d, areg, numdouble);
            	manst = AREG;
                break;
            case TDynArrBound:
            {
            	int displ = tree[tc++], dim = tree[tc++];
            	areg = mbox <= 2 ? breg : t1;
            	manst = AREG;
            	tocodeB(lw, areg, dyn_arr_info[displ][dim-1], gp);
            }
            	break;
            case TString:
            case TStringc:
            case TStringf:
            {
                int n = tree[tc++], i;
                printf("\t.rdata\n\t.align 2\n");
                fprintf(output, "\t.rdata\n\t.align 2\n");
                printf("\t.word %i\n", n);
                fprintf(output, "\t.word %i\n", n);
            tocodeL("STRING", stringnum);
                if (treecode == TStringc)
                {
                    for (i=0; i<3*n; i+=3)
                    {
                        int c;
                        tc++;      //TConstc
                        c = tree[tc++];
                        printf("\t.half %i\n", c);
                        fprintf(output, "\t.half %i\n", (tc++, c));
                    }
                }
                else if (treecode == TString)
                {
                    for (i=0; i<3*n; i+=3)
                    {
                        int c;
                        tc++;                // TConst
                        c = tree[tc++];      // TExprend
                        printf("\t.word %i\n", c);
                        fprintf(output, "\t.word %i\n", (tc++, c));
                    }
                }
                else        // TStringf
                {
                    for (i=0; i<3*n; i+=3)
                    {
                        int c;         // TConstf
                        tc++;
                        c = tree[tc++];
                        float f;
                        memcpy(&f, &c, sizeof(float));
                        printf("\t.float %f\n", f);
                        fprintf(output, "\t.float %f\n", (tc++, f));
                    }
                }
                printf("\t.text\n\t.align 2\n");
                fprintf(output, "\t.text\n\t.align 2\n");
                rez = mbox <= BREGF ? breg : t0;
                printf("\tlui $t1, %%hi(STRING%i)\n", stringnum);
                fprintf(output, "\tlui $t1, %%hi(STRING%i)\n", stringnum);
                printf("\taddiu %s, $t1, %%lo(STRING%i)\n", regs[rez], stringnum);
                fprintf(output, "\taddiu %s, $t1, %%lo(STRING%i)\n", regs[rez], stringnum++);
            }
                break;

            case TDeclid:
                MDeclid_gen();
                break;
            case TBeginit:
            {
                int n = tree[tc++], i;
                for (i=0; i<n; i++)
                    MExpr_gen();
            }
                break;
            case TIndVar:
            {
            	struct ind_var a;
            	a.id = tree[tc++];
            	ind_var_info[ind_var_number++] = a;
            }
            	break;
            case TSliceInd:
            {
            	// Должен быть поиск по id + нужно изменение индекса по шагу сделать
            	int id = tree[tc++];
            	for (int i = 0; i <= ind_var_number; i++)
            	{
            		if (ind_var_info[i].id == id)
            			areg = ind_var_info[i].reg;
            	}
            }
            	break;
            case TSliceident:
            {
                int olddispl, oldareg, oldbox = mbox, oldreg = breg, eltype;
                mdsp(tree[tc++]);                  // параметр - смещение
				eltype = tree[tc++];               // параметр - тип элемента
                olddispl = adispl; oldareg  = areg;
                if (indexcheck)
                {
					mbox = BREG;                   // a0 - это C0
					breg = a1;                     // a1 - это index
					MExpr_gen();
					tocodeB(lw, a0, olddispl, oldareg);
					endslice(eltype);
					
					if (tree[tc] == TSlice)
					{                                  // многомерная вырезка
						int r = getreg();
						while (tree[tc] == TSlice)
						{
							tc++;                      // сама TSlice
							eltype = tree[tc++];       // съели тип элемента
							tocodemove(r, v0);
							mbox = BREG;
							breg = a1;
							MExpr_gen();
							tocodeB(lw, a0, 0, r);
							endslice(eltype);
						}
						freereg(r);
					}
                    
                    mbox = oldbox;
                    breg = oldreg;
                    if (mbox <= BREGF)
                        tocodemove(areg = breg, v0);
                }
                else
                {
					int rez = getreg();
					int oldbox = mbox;
					mbox = BF;                       // rez - это C0
					MExpr_gen();
					tocodeB(lw, rez, olddispl, oldareg);
//					MExpr_gen(); // откуда вообще этот вызов?
					if (manst == CONST)
					{
						if (num > 32768)
							tocodeLI(li, areg = t0, num);
						else
							tocodeI(addi, areg = t0, d0, num);
					}
					endslice0(eltype, areg, rez);
					
					while (tree[tc] == TSlice)
					{                                  // многомерная вырезка
						tc++;                          // сама TSlice
						eltype = tree[tc++];           // съели тип элемента
						d = sizeof(eltype);
						tocodeB(lw, rez, 0, rez);
						mbox = BF;
						MExpr_gen();
						if (manst == CONST)
						{
							if (num > 32768)
								tocodeLI(li, areg = t0, num);
							else
								tocodeI(addi, areg = t0, d0, num);
						}
						endslice0(eltype, areg, rez);
					}
                    mbox = oldbox;
                    breg = oldreg;
                    if (mbox <= BREGF)
                        tocodemove(areg = breg, rez);
					else
						areg = rez;
					freereg(rez);
				}
				break;
            }
    
            case TSelect:
            {
                int type;
                mdsp(tree[tc++]);
                type = tree[tc++];
                if (type > 0 && modetab[type] == MSTRUCT)
                    manst = AMEM;
                else
                {
                    rez = mbox <= 2 ? breg : type == LFLOAT ? ft0 : t0;
                    tocodeB(type == LCHAR ? lhu : type == LFLOAT ? lwc1 : lw, rez, adispl, areg);
                    manst = AREG;
                    areg = rez;
                }
                break;
            }
            case TCall1:
            {
                int ftype = tree[tc++];
                int i, n = modetab[ftype+2];
                int a0i = a0, a0f = fa0;
                mbox = BREG;
                for (i=0; i < n; i++)
                {
                    int ptype = modetab[ftype+3+i];
                    if (ptype == LFLOAT || ptype == LDOUBLE)
                    {
                        breg = a0f++;
                        if (a0f == 44)
                            merror(too_many_params);
                    }
                    else
                    {     // параметры массивы и структуры передаются адресом
                        breg = a0i++;
                        if (a0i == 8)
                            merror(too_many_params);
                    }
                    MExpr_gen();
                }
                tc++;
//            case TCall2:
                tocodeJ(jal, "FUNC", identab[tree[tc++]+3]);
                ftype = modetab[ftype+1];
                manst = AREG;
                mbox = oldbox; breg = oldreg; elselab = oldelselab;
                if (ftype == LFLOAT || ftype == LDOUBLE)
                {
                    if (mbox <= BREGF)
                        tocodemove(areg = breg, fv0);
                    else
                        areg = fv0;
                }
                else
                {
                    if (mbox == BREG || (mbox == BREGF &&
                        (ftype > 0 && modetab[ftype] == MSTRUCT)))
                        tocodemove(areg = breg, v0);
                    else
                        areg = v0;
                    BCend();
                }
            }
                break;
                
            default:
                tc--;
        }
    mbox = oldbox; breg = oldreg; elselab = oldelselab;
}

void MDeclarr()
{
    int i, displ, N;
    N = tree[tc++];
    if (N > 5)
        merror(dim_greater_5);
    mbox = BREGF;
    breg = t0;
    displ = globinit;  // это bounds
    for (i=0; i<N; i++)
    {
        MExpr_gen();
        if (manst == CONST)
        {
        	is_dyn = 0;
            if (num < 0)
            {
                tocodeI(addi, a0, d0, wrong_number_of_elems);
                tocodeJ(jal, "ERROR", -1);
            }
            else if (num < 32768)
                tocodeI(addi, t0, d0, num);
            else
                tocodeLI(li, t0, num);
        }
        else
        {
        	// сохранение информации для оптимизации индуцированных переменных
        	cur_dyn_arr_info[i] = displ;
        	is_dyn = 1;
        	
            tocodeJC(bgez, t0, "DECLARR", labnum);
            tocodeI(addi, a0, d0, wrong_number_of_elems);
            tocodeJ(jal, "ERROR", -1);
        tocodeL("DECLARR", labnum++);
        }
        tocodeB(sw, t0, displ, gp);
        displ -= 4;
    }

}

void mcompstmt_gen()
{
    mbox = BV;
    while (tree[tc] != TEnd)
    {
        switch (tree[tc])
        {
			case 0:
				tc++;
				break;
            case NOP:
                tc++;
                break;
            case TIdent:
                tc += 2;
                break;
            case TDeclarr:
            {
                tc++;
                MDeclarr();
                break;
            }
            case TDeclid:
                tc++;
                MDeclid_gen();
                break;
                
            default:
                MStmt_gen();
        }
    }
    tc++;
}


void MStmt_gen()
{
    int r;
    mbox = BV;
    switch (tree[tc++])
    {
        case NOP:
            break;
/*
        case CREATEDIRECTC:
            tocode(CREATEDIRECTC);
            break;
            
        case EXITC:
            tocode(EXITC);
            break;
            
        case TStructbeg:
            tocode(B);
            tocode(0);
            iniprocs[tree[tc++]] = pc;
            break;
            
        case TStructend:
        {
            int numproc = tree[tree[tc++]+1];
            tocode(STOP);
            mem[iniprocs[numproc]-1] = pc;
        }
            break;
*/
        case TBegin:
            mcompstmt_gen();
            break;
            
        case TIf:
        {
            int elseref = tree[tc++], curelse = elselab = labnum++, labend;
            labend = labnum++;
            mbox = BCF;
            MExpr_gen();
            MStmt_gen();
            if (elseref)
            {
                tocodeJ(jump, "END", labend);
            tocodeL("ELSE", curelse);
                MStmt_gen();
            tocodeL("END", labend);
            }
            else
            tocodeL("ELSE", curelse);
        }
            break;
        case TWhile:
        {
            int oldbreak = adbreak, oldcont = adcont;
            adcont  = labnum++;
            elselab = adbreak = labnum++;
        tocodeL("CONT", adcont);
            mbox = BCF;
            MExpr_gen();
            MStmt_gen();
            tocodeJ(jump, "CONT", adcont);
        tocodeL("ELSE", adbreak);
        tocodeL("END", adbreak);
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TDo:
        {
            int oldbreak = adbreak, oldcont = adcont, labbeg = elselab = labnum++;
            adcont  = labnum++;
            adbreak = labnum++;
        tocodeL("LOOP", labbeg);
        tocodeL("ELSE", labbeg);
            MStmt_gen();
        tocodeL("CONT", adcont);
            mbox = BCT;
            MExpr_gen();
        tocodeL("END", adbreak);
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TFor:
        {
            int is_last_nested = 0;
			if (check_nested_for)
                is_last_nested = tree[tc++]; // Флаг вложенности
            int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++],
            stmtref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, incrtc, endtc;
            int cond_reg;
            mbox = BV;
            if (is_last_nested == 3)
            	left_reg_cond = 0;
            if (fromref)
                MExpr_gen();         // init
            int for_value_reg = areg;	 // for delay slot
            adbreak = elselab = labnum++;
            adcont  = labnum++;
            if (cycle_jump_reduce == 0)
            	tocodeL("BEGLOOP", adcont);
            if (condref)
            {
            	int oldbreg = breg;
                mbox = BCF;
                if (cycle_condition_calculation && is_last_nested)
                {
                	cond_reg = getreg();
                	flag_cond_cycle = 1;
                	breg = cond_reg;
                }
                MExpr_gen();         // cond
                if (enable_ind_var && is_last_nested)
                {
                	endtc = tc;
                	tc = stmtref;
                	while (tree[tc] == TIndVar)
                	{
						breg = getreg();
						MExpr_gen(); // TIndVar
						mbox = BF;
						MExpr_gen(); // Шаг;
						if (manst == CONST)
						{
							ind_var_info[ind_var_number - 1].is_static = 1;
							ind_var_info[ind_var_number - 1].step = num;
						}
						else
						{
							tocodemove(t0 + 7, areg);
							areg = t0 + 7;
							tocodeI(addi, areg, areg, 1);
							tocodeI(addi, t1, d0, 4);
							tocodeR(mul, areg, areg, t1);
							ind_var_info[ind_var_number - 1].is_static = 0;
							ind_var_info[ind_var_number - 1].step = areg;
						}
						mbox = BREG;
						MExpr_gen(); // TSliceident
						ind_var_info[ind_var_number - 1].reg = areg;
                	}
                	stmtref = tc;
                	tc = endtc;
                }
                if (cycle_condition_calculation && delay_slot && is_last_nested)
                {
                	endtc = tc;
                    tc = incrref;
                    delay_slot_inc = 0;
                    int cond = cond_reg;
                    if (is_last_nested != 3)
                    	MExpr_gen();         // incr
                    else
                    {
                        tocodemove(left_reg_cond, cond_reg);
                        if (ind_var_info[0].is_static)
                        {
							if (ind_var_info[0].step == 1)
								tocodeI(addi, t1, d0, ind_var_info[0].step * 4);
							else
								tocodeI(addi, t1, d0, -(ind_var_info[0].step + 1) * 4);
                        }
                        else
                        {
                        	tocodeI(addi, t1, d0, -4);
                        	tocodeR(mul, t1, t1, ind_var_info[0].step);
                        }
                    	tocodeR(mul, cond, cond, t1);
                    	tocodeR(add, cond, cond, ind_var_info[0].reg);
                    	left_reg_cond = ind_var_info[0].reg;
                    }
                    tc = endtc;

                    if (is_last_nested != 3)
                    {
						tocodeI(addi, for_value_reg, for_value_reg, -delay_slot_inc); // тут иногда 1
						tocodeI(addi, cond, cond, -delay_slot_inc); // тут иногда 1
                    }
                }
                flag_cond_cycle = 0;
                breg = oldbreg;
            }
            if (cycle_jump_reduce == 1)
            	tocodeL("BEGLOOP", adcont);
            if (cycle_condition_calculation && delay_slot && is_last_nested)
            {
            	endtc = tc;
                tc = incrref;
            tocodeL("CONT", adcont);
                MExpr_gen();         // incr
                tc = endtc;
            }
            if (incrref)
            {
                mbox = BV;
                incrtc = incrref;
                tc = stmtref;
                MStmt_gen();			// statement
                if (!(delay_slot && is_last_nested))
                {
					endtc = tc;
					tc = incrtc;
				tocodeL("CONT", adcont);
					MExpr_gen();         // incr
					tc = endtc;
                }
            }
            else
            {
                MStmt_gen();         // statement
            tocodeL("CONT", adcont);
            }
            if (cycle_jump_reduce == 0)
            	tocodeJ(jump, "BEGLOOP", adcont);
            if (cycle_jump_reduce == 1 && condref)
            {
            	int old_tc = tc, oldbreg = breg;
            	tc = condref;
                mbox = BCF;
                flag_jump_end_cycle = 1;
                if (cycle_condition_calculation && is_last_nested)
                {
                	flag_cond_cycle = 2;
                	breg = cond_reg;
                }
                // увеличение индуцированных переменных
                if (enable_ind_var)
                {
                	for (int i = 0; i < ind_var_number; i++)
                	{
                		if (ind_var_info[i].is_static)
                		{
							if (ind_var_info[i].step == 1)
								tocodeI(addi, ind_var_info[i].reg, ind_var_info[i].reg, ind_var_info[i].step * 4);
							else
								tocodeI(addi, ind_var_info[i].reg, ind_var_info[i].reg, -(ind_var_info[i].step + 1) * 4);
                		}
                		else
                		{
                			tocodeR(sub, ind_var_info[i].reg, ind_var_info[i].reg, ind_var_info[i].step);
                		}
                	}
                }
                MExpr_gen();         // cond
                flag_jump_end_cycle = 0;
                flag_cond_cycle = 0;
                tc = old_tc;
                if (cycle_condition_calculation && is_last_nested)
                	freereg(cond_reg);
            }
        	if (cycle_condition_calculation && delay_slot && is_last_nested && is_last_nested != 3)
        		tocodeI(addi, cond_reg, cond_reg, delay_slot_inc); // тут иногда -1
        tocodeL("end", adbreak);
        tocodeL("ELSE", adbreak);
        	if (ind_var_number != 0)
        	{
        		for (int i = 0; i < ind_var_number; i++)
        			freereg(ind_var_info[i].reg);
        		ind_var_number = 0;
        	}
			tc++; // Здесь был TForEnd

            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TGoto:
            tocodeJ(jump, "LABEL", abs(tree[tc++]));
            break;
        case TLabel:
        tocodeL("LABEL", tree[tc++]);
            break;
        case TSwitch:
        {
            int oldbreak = adbreak, oldcase = adcase, oldreg = switchreg;
            adcase = labnum++;
            adbreak = labnum++;
            mbox = BREG;
            switchreg = breg = getreg();
            MExpr_gen();
            MStmt_gen();
            tocodeL("END", adbreak);
            freereg(breg);
            adcase = oldcase;
            adbreak = oldbreak;
            switchreg = oldreg;
        }
            break;
        case TCase:
        {
            int ocase =adcase;
            mbox = BREG;
            breg = t0;
        tocodeL("CASE", ocase);
            MExpr_gen();
            tocodeJEQ(bne, t0, switchreg, "CASE", adcase = labnum++);
            MStmt_gen();
        }
            break;
        case TDefault:
        {
    tocodeL("CASE", adcase);
            MStmt_gen();
        }
            break;
        case TBreak:
            tocodeJ(jump, "END", adbreak);
            break;
        case TContinue:
            tocodeJ(jump, "CONT", adcont);
            break;
        case TReturnval:
            r = tree[tc++];
            mbox = BREG;
            breg = r == LFLOAT || r == LDOUBLE ? fv0 : v0;
            MExpr_gen();
        case TReturnvoid:
            tocodeJ(jump, "FUNCEND", identref);
            break;
           
        case TPrintid:
        {
            int i, t;
            i = tree[tc++];                // ссылка на identtab
            t = identab[i + 2];            // тип
            
            fprintf(output, "\t.rdata\n\t.align 2\n");
        tocodeL("STRING", stringnum);
            printf("\t.ascii \"");
            fprintf(output, "\t.ascii \"");

            while (tree[tc] != 0)
            {
                printf("%c", tree[tc]);
                fprintf(output, "%c", tree[tc++]);
            }
            tc++;
            printf("\\0\"\n\t.text\n\t.align 2\n");
            fprintf(output, "\\0\"\n\t.text\n\t.align 2\n");
            fprintf(output, "\tlui $t1, %%hi(STRING%i)\n", stringnum);
            fprintf(output, "\taddiu $a0, $t1, %%lo(STRING%i)\n", stringnum++);
            tocodemove(s7, stp);
            tocodeI(addi, stp, fp, -16);
            tocodeJ(jal, "printf", -1);
            tocodemove(stp, s7);
            
            if (identab[i] < 0)
            {
                int r = identab[i+3];
                if (t == LFLOAT)
                {
                    tocodeB(swc1, r, -8064, gp);
                    tocodeI(addi, a0, gp, -8064);
                }
                else   //  if (t == LINT || t == LCHAR)
                {
                    tocodeB(t == LCHAR ? sh : sw, r, -8068, gp);
                    tocodeI(addi, a0, gp, -8068);
                }
            }
            else
            {
                mdsp(identab[i + 3]);
                tocodeI(addi, a0, areg, adispl);
            }
            tocodeI(addi, a1, d0, t);
            if (t == LCHAR)
                tocodemove(a2, d0);
            else if (t > 0 && modetab[t] == MARRAY && modetab[t + 1] > 0)
                tocodeI(addi, a2, d0, '\n');
            else
                tocodeI(addi, a2, d0, ' ');
            tocodeI(addi, a3, d0, '\n');
            tocodeJ(jal, "auxprint", -1);
        }
            break;
			
		case TGetid:
		{
			int i, t;
			i = tree[tc++];                // ссылка на identtab
			t = identab[i + 2];            // тип
			
			mdsp(identab[i + 3]);
			tocodeI(addi, a0, d0, t);
			tocodeI(addi, a1, areg, adispl);
			tocodeJ(jal, "auxget", -1);
		}
			break;

        case TPrintf:
        {
            tc++;  // общий размер того, что надо вывести
            tc++;

            int n = tree[tc++], i, old_stringnum = stringnum, counter = 0;
            char var[100];
            printf("\t.rdata\n\t.align 2\n");
            fprintf(output, "\t.rdata\n\t.align 2\n");
        tocodeL("STRING", stringnum);
            printf("\t.ascii \"");
            fprintf(output, "\t.ascii \"");
            for (i=0; i<n; i++)
            {
                char c = tree[tc];
                if (c == '\n')
                {
                    tc++;
                    printf("%s", "\\n");
                    fprintf(output, "%s", "\\n");
                }
                else
                {
                    if (c == '%')
                    {
                        printf("%c", c);
                        fprintf(output, "%c", tree[tc++]);
                        i++;
                        c = tree[tc];
                        var[counter++] = c;
                        printf("%c", c);
                        fprintf(output, "%c", tree[tc++]);
                        if (i == n - 2 || i == n - 1)
                            continue;
                        printf("\\0\"\n");
                        fprintf(output, "\\0\"\n");
                        stringnum++;
                    tocodeL("STRING", stringnum);
                        printf("\t.ascii \"");
                        fprintf(output, "\t.ascii \"");

                    }
                    else
                    {
                        printf("%c", c);
                        fprintf(output, "%c", tree[tc++]);
                    }
                }
            }
            stringnum++;
            printf("\\0\"\n\t.text\n\t.align 2\n");
            fprintf(output, "\\0\"\n\t.text\n\t.align 2\n");
            
            for (i = 0; i < stringnum - old_stringnum; i++)
            {
                if (i < counter)
                {
                    mbox = BREG;
                    if (var[i] == 'f')
                    {
                        breg = ft0;
                        MExpr_gen();
                        fprintf(output, "\tcvt.d.s $f4,$f4\n\tmfc1\t$5,$f4\n\tmfhc1\t$6,$f4\n");
                    }
                    else
                    {
                        breg = a1;
                        MExpr_gen();
                    }
                }
                printf("\tlui $t1, %%hi(STRING%i)\n", old_stringnum + i);
                fprintf(output, "\tlui $t1, %%hi(STRING%i)\n", old_stringnum + i);
                printf("\taddiu %s, $t1, %%lo(STRING%i)\n", regs[a0], old_stringnum + i);
            fprintf(output, "\taddiu %s, $t1, %%lo(STRING%i)\n", regs[a0], old_stringnum + i);
                tocodemove(s7, stp);
                tocodeI(addi, stp, fp, -16);
                tocodeJ(jal, "printf", -1);
                tocodemove(stp, s7);
            }
        }
            break;
 /*
        case SETMOTOR:
            MExpr_gen();
            MExpr_gen();
            tocode(SETMOTORC);
            break;
 */
        default:
            tc--;
            mbox = BF;
            MExpr_gen();
            break;
    }
}

void Struct_init_gen(int t, int olddispl, int oldreg)  // в это время modetab[t] == MSTRUCT
{
    int i,n;
    if (tree[tc] == TStructinit)                       // был  {  }
    {
        t++;
        tc++;
        n = tree[tc++];
        for (i = 0; i < n; i++)
            Struct_init_gen(modetab[t+=2], olddispl, oldreg);
    }
    else
    {
        int al = alignm(t);
        structdispl = (structdispl+al-1)/al*al;
        if (modetab[t] == MSTRUCT)
        {
            mbox = BF;
            MExpr_gen();   // на этот запрос структура отвечает только AMEM
            structcopy(t, olddispl+structdispl, oldreg, adispl, areg);
        }
        else
        {
            int flagint = 1;
            mbox = BREG;
            breg = t == LFLOAT ? flagint = 0, ft0 : t0;
            if (t == LCHAR)
                flagint = 2;
            MExpr_gen();
            tocodeB(flagint == 0 ? swc1 : flagint == 1 ? sw : sh,
                    breg, olddispl+structdispl, oldreg);
        }
        structdispl += szof(t);
    }
}

void ARInitc(int N, int dim, char *str, int type)
{
    char si[80];
//    printf("dim= %i str %s\n", dim, str);
    if (dim < N)
    {
        int i, n = tree[tc+1];
        tc += 2;
        printf("\t.long %i\n", n);
        fprintf(output, "\t.long %i\n", n);
    tocodeL(str, -1);

        for (i=0; i<n; ++i)
        {
            sprintf(si, "%i", i);
            {
                char s[80];
                strcpy(s, str);
                char *ssi = strcat(s, si);
                printf("\t.long %s\n", ssi);
                fprintf(output, "\t.long %s\n", ssi);
            }
        }
        printf("\n");
        for (i=0; i<n; ++i)
        {
            sprintf(si, "%i", i);
            {
                char s[80];
                strcpy(s, str);
                char *ssi = strcat(s, si);
                ARInitc(N, dim+1, ssi, type);
            }
        }
    }
    else
    {
        int n = tree[tc+1], i;
        tc += 2;
        printf("\t.long %i\n", n);
        fprintf(output, "\t.long %i\n", n);
    tocodeL(str, -1);
        if (type == LCHAR)
        {
            for (i=0; i<3*n; i+=3)
            {
                int c;
                tc++;      //TConstc
                c = tree[tc++];
                printf("\t.half %i\n", c);
                fprintf(output, "\t.half %i\n", (tc++, c));
            }
        }
        else if (type == LINT)
        {
            for (i=0; i<3*n; i+=3)
            {
                int c;
                tc++;                // TConst
                c = tree[tc++];      // TExprend
                printf("\t.long %i\n", c);
                fprintf(output, "\t.long %i\n", (tc++, c));
            }
        }
        else        // LFLOAT
        {
            for (i=0; i<3*n; i+=3)
            {
                int c;         // TConstf
                tc++;
                c = tree[tc++];
                float f;
                memcpy(&f, &c, sizeof(float));
                printf("\t.float %f\n", f);
                fprintf(output, "\t.float %f\n", (tc++, f));
            }
        }
        printf("\n");
    }
}

void MDeclid_gen()
{
    int oldid = tree[tc++], telem = tree[tc++], N = tree[tc++], element_len,
    all = tree[tc++], iniproc = tree[tc++], usual = tree[tc++], instruct = tree[tc++];
    // all - общее кол-во слов в структуре
    //  для массивов есть еще usual
    // == 0 с пустыми границами,
    // == 1 без пустых границ,
    // all == 0 нет инициализатора,
    // all == 1 есть инициализатор
    // all == 2 есть инициализатор только из строк
    int olddispl, oldreg, ardispl = identab[oldid+3];
    mdsp(ardispl);
    olddispl = adispl;
    oldreg = areg;
    element_len = szof(telem);
    
    if (N == 0)                    // обычная переменная int a; или struct point p;
    {
        if (identab[oldid] < 0)    // регистровая переменная
        {
            if (telem == LINT || telem == LLONG || telem == LCHAR ||
                (telem > 0 && modetab[telem] == MPOINT))
                identab[oldid+3] = getreg();
            else if (telem == LFLOAT)        // LDOUBLE пока нет
                identab[oldid+3] = getregf();
        }
/*        if (iniproc)
        {
            tocode(STRUCTWITHARR);
            tocode(olddispl);
            tocode(iniprocs[iniproc]);
        } */
        if (all)                   // int a = или struct{} a =
        {
            if (telem > 0 && modetab[telem] == MSTRUCT)
            {
                structdispl = 0;
                Struct_init_gen(telem, olddispl, oldreg);
            }
            else
            {
//                int oldbox = mbox, oldreg = breg, oldelselab = elselab;
                int flagint = 1;
                mbox = BREG;
                if (identab[oldid] < 0)    // регистровая переменная
                {
                    breg = identab[oldid+3];
                    MExpr_gen();
                }
                else
                {
                    breg = telem == LFLOAT ? flagint = 0, ft0 : t0;
                    if (telem == LCHAR)
                        flagint = 2;
                    MExpr_gen();
                    tocodeB(flagint == 0 ? swc1 : flagint == 1 ? sw : sh,
                            breg, olddispl, oldreg);
                }
//                mbox = oldbox; breg = oldreg; elselab = oldelselab;
            }
        }
    }
    else if (usual == 2)
    {
        char s[80], si[80];
        s[0] = 'A';
        s[1] = 'R';
        s[2] = '\0';

        sprintf(si, "%i", labnum);
        strcat(s, si);
        printf("\t.rdata\n\t.align 2\n");
        fprintf(output, "\t.rdata\n\t.align 2\n");
        ARInitc(N, 1, s, telem);
        printf("\t.text\n");
        fprintf(output, "\t.text\n");
        
        printf("\tlui $t1, %%hi(AR%i)\n", labnum);
        fprintf(output, "\tlui $t1, %%hi(AR%i)\n", labnum);
        printf("\taddiu %s, $t1, %%lo(AR%i)\n", regs[t0], labnum);
        fprintf(output, "\taddiu %s, $t1, %%lo(AR%i)\n", regs[t0], labnum++);

        tocodeB(sw, t0, olddispl, oldreg);
    }
    else                                // Обработка массива int a[N1]...[NN] =
    {
    	if (is_dyn) // сохранение информации о динамическом массиве
    	{
    		for (int i = 0; i < 5; i++)
    			dyn_arr_info[identab[oldid + 3]][i] = cur_dyn_arr_info[i];
    	}
        tocodeI(addi, a0, d0, all == 0 ? N : N-1);
        tocodeI(addi, a1, d0, element_len);
        tocodeI(addi, a2, d0, ardispl);
        tocodeI(addi, a3, d0, all*4 + usual);
        tocodeJ(jal, "DEFARR", -1);
        
        if (all)                        // all == 1, если есть инициализация массива
        {
 /*           MExpr_gen();
            tocode(ARRINIT);        // ARRINIT N d all displ usual
            tocode(abs(N));
            tocode(element_len);
            tocode(olddispl);
            tocode(usual);             // == 0 с пустыми границами
*/                                       // == 1 без пустых границ и без иниц
        }
      }
}

void mipsgen()
{
    int treesize = tc;
 //   maxdisplg = (maxdisplg + 2) * 4;
    tc = 0;
    notrobot = 0;
    ind_var_number = 0;
    if (wasmain == 0)
        error(no_main_in_program);

    fprintf(output, "\n\t.globl\tmain\n");
    fprintf(output, "\t.ent\tmain\n\t.type\tmain, @function\nmain:\n");

    tocodemove(fp, stp);
    tocodeI(addi, fp, fp, -4);
    tocodeB(sw, ra, 0, fp);
    tocodeLI(li, t0, 268500992);      // это 0x10010000 - нижняя граница дин памяти
    tocodeB(sw, t0, globinit-60, gp); // это heap

    while (tc < treesize)
    {
        switch (tree[tc++])
        {
            case TEnd:
                break;
            case TFuncdef:
            {
                int i, n, ftype, id, a0i = a0, a0f = fa0;
                int displf = 12, displi = -4;
                identref =  tree[tc++];
                ftype = identab[identref+2];
                n = modetab[ftype+2];
                id = identref + 4;
                tocodeJ(jump, "NEXT", identref);
            tocodeL("FUNC", identab[identref+3]);
                maxdispl = (tree[tc++] + 7) / 8 * 8;
                
                tocodeI(addi, fp, fp, -maxdispl - DISPL0);
                tocodeB(sw, stp, 20, fp);
                tocodemove(stp, fp);
                tocodeB(sw, ra, 16, stp);
                printf("\n");
                fprintf(output, "\n");
                    
                for (i=0; i < n; i++)
                {
                    int ptype = modetab[ftype+3+i];
//                    identab[id] = -identab[id];
                    if (ptype == LFLOAT || ptype == LDOUBLE)
                    {
//                        identab[id+3] = a0f;
                        identab[id+3] = displf += 4;
                        tocodeB(swc1, a0f++, displf+DISPL0, stp);
                        if (a0f == 44)
                            merror(too_many_params);
                    }
                    else
                    {     // параметры массивы и структуры передаются адресом
//                        identab[id+3] = a0i;
                        identab[id+3] = displi += 4;
                        tocodeB(sw, a0i++, displi+DISPL0, stp);
                        if (a0i == 8)
                            merror(too_many_params);
                    }
                    id += 4;
                }
                tc++;             // TBegin
                mcompstmt_gen();
                printf("\n");
                fprintf(output, "\n");
            tocodeL("FUNCEND", identref);
                tocodeB(lw, ra, 16, stp);
                tocodeI(addi, fp, stp, maxdispl + DISPL0);
                tocodeB(lw, stp, 20, stp);
                tocodeJR(jr, ra);
            tocodeL("NEXT", identref);
            }
                break;
            case TDeclarr:
            {
                MDeclarr();
                break;
            }
            case TDeclid:
                MDeclid_gen();
                break;
                
            case NOP:
                break;
            /*
            case TStructbeg:
                tocode(B);
                tocode(0);
                iniprocs[tree[tc++]] = pc;
                break;
            
            case TStructend:
            {
                int numproc = tree[tree[tc++]+1];
                tocode(STOP);
                mem[iniprocs[numproc]-1] = pc;
            }
                break;
             */
                
            default:
            {
                printf("mips что-то не то\n");
                printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", tc, tree[tc-2], tree[tc-1]);
                exit(1);
            }
        }
    }
    tocodeJ(jal,"FUNC",identab[wasmain+3]);
    tocodeB(lw, ra, -4, stp);
    tocodeJR(jr, ra);
    
    fprintf(output, "\t.end\tmain\n\t.size\tmain, .-main\n");
    fprintf(output, "\t.rdata\n\t.align 2\n\t.word %i\nmodetab:\n\t.word 0\n", md);
    for (aux = 1; aux <= md; ++aux)
        fprintf(output, "\t.word %i\n", modetab[aux]);
}
