//
//  mip16gen.c
//
//  Created by Andrey Terekhov on 16/12/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"

// запросы
#define BREG   1     // загрузить в breg
#define BREGF  2     // запрос на левый операнд, могут ответить
    // CONST, тогда делать ничего не надо, могут ответить
    // AMEM, тогда надо загрузить в breg, могут ответить
    // AREG, если areg - сохраняемый регистр, то ничего не делать, иначе в breg
    // В качестве левого операнда могут использоваться только сохраняемые регистры,
    // в том числе и те, которые применяются для адресации
#define BF     3     // свободный запрос на правый операнд
#define BCF    4     // goto на метку elselab по false
#define BCT    5     // goto на метку elselab по true
#define BV     6     // значение не нужно

// ответы
#define AREG  1      // in areg register number
#define AMEM  2      // in adispl and areg address
#define CONST 3      // in num int const

int mbox, breg, elselab;
int manst, adispl, areg;
int labnum = 1, stringnum = 1, elselab, flagBC, identref;
int log_real = 2;
// унарные операции LNOT, LOGNOT, -, ++, --, TIdenttoval(*), TIdenttoaddr(&)
// LNOT nor rd, rs, d0    LOGNOT slti rt, rs, 1   - sub rd, d0, rt
// *  lw rt, displ(rs) или сразу 0(areg)   & addi rt, areg, adispl или сразу areg
// ++ lw rt, adispl(areg) addi rt, rt, 1 sw rt, adispl(areg)
extern void error(int err);
extern int szof(int);

int pc = 32, d0 = 0, at = 1, v0 = 2,  v1 = 3,  a0 = 4,   a1 = 5, a2 = 6, a3 = 7,
                            fv0 = 34, fv1 =35, fa0 = 42, fa1 = 43,
    t0 = 8, t1 = 9, t2 = 10, t9 = 25, s0 = 16, gp = 28, stp = 29, fp = 30, ra =  31,
   ft0 = 36,ft1 = 37,                fs0 = 44, opnd = 43;

char *mcodes[] =
{/* 0 */ "bgez", "bltz", "j", "jal", "beq", "bne", "blez", "bgtz", "addi", "addiu",
/* 10 */ "slti", "sltiu", "andi", "ori", "xori", "lui", "mfhi", "", "mflo", "",
/* 20 */ "", "", "", "", "", "li", "div", "", "mul", "",
/* 30 */ "", "", "", "", "", "lw", "", "", "", "",
/* 40 */ "", "", "", "sw", "", "", "", "", "", "",
/* 50 */ "", "", "", "", "", "", "", "", "", "",
/* 60 */ "sll", "", "srl", "sra", "sllv", "", "srlv", "srav", "jr", "jalr",
/* 70 */ "", "", "", "", "", "", "", "", "", "",
/* 80 */ "", "", "", "", "", "", "", "", "", "",
/* 90 */ "", "", "add", "addu", "sub", "subu", "and", "or", "xor", "nor",
/*100 */ "", "", "slt", "sltu", "", "", "", "", "", "",
/*110 */ "add.s", "sub.s", "mul.s", "div.s", "abs.s", "neg.s", "", "", "", "",
/*120 */ "li.s", "", "", "", "", "lwc1", "swc1", "mtc1.s", "bc1t", "bc1f",
/*130 */ "c.seq.s", "c.lt.s", "c.le.s", "cvt.s.w"
};

char *regs[] =
{/* 0 */ "$0",  "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1",
/* 10 */ "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3",
/* 20 */ "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp",
/* 30 */ "$fp", "$ra", "pc", "",
/* 34 */ "$fv0", "$fv1", "$ft0", "$ft1", "$ft2", "$ft3", "$ft4", "$ft5", "$fa0", "$fa1",
/* 44 */ "$fs0", "$fs1", "$fs2", "$fs3", "$fs4", "$fs5"
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

            
        default:
            break;
    }
}
/*
int szof(int mode)
{
    if (mode > 0)
    {
        int el = modetab[mode], len = 0, i;
        if (el == MARRAY || el == MPOINT)
            return 4;
        else             //  MSTRUCT
        {
            for (i=0; i < modetab[mode+2]; i+=2)
                len += szof(modetab[mode+i+3]);
            return len;
        }
    }
    else if (mode == LCHAR)
        return 1;
    else if (mode == LLONG || mode == LDOUBLE)
        return 8;
    else
        return 4;        // LINT и LDOUBLE
}
*/
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
    printf("\tmove %s, %s\n", regs[rd], regs[rs]);
    fprintf(output, "\tmove %s, %s\n", regs[rd], regs[rs]);
}

void tocodeSLR(int op, int rd, int rt, int shamt)  // sll rd, rt, shamt    srl sra
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
    fprintf(output, "\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
}

void tocodeJ(int op, char type[], int label)  // j label   jal label
{
    printf("\t%s %s%i\n", mcodes[op], type, label);
    fprintf(output, "\t%s %s%i\n", mcodes[op], type, label);
}

void tocodeJR(int op, int rs)   // jr rs    jalr rs
{
    printf("\t%s %s\n", mcodes[op], regs[rs]);
    fprintf(output, "\t%s %s\n", mcodes[op], regs[rs]);
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
    printf("%s%i:\n", type, label);
    fprintf(output, "%s%i:\n", type, label);
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



void MExpr_gen();

void mdsp(int displ)
{
    manst = AMEM;
    if (displ < 0)
    {
        areg = gp;
        adispl = -(32768 + 4 * (displ - 3));  // в глоб данных первые 5 слов bounds + слово
    }                                         // сохранения возврата
    else
    {
        areg = fp;
        adispl = 4 * (displ - 3);
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
    if ((c > ASSR && c <= DIVASSR) || (c > ASSATR && c <= DIVASSATR)
    		|| (c > ASSRV && c <= DIVASSRV) || (c > ASSATRV && c <= DIVASSATRV))
    {
        int rez = mbox <= 2 ? breg : ft0;
        if (leftanst == AMEM)
        {
            tocodeB(lwc1, ft1, leftdispl, leftreg);   // исходное значение
        
            if (c == PLUSASSR || c == PLUSASSATR || c == PLUSASSRV || c == PLUSASSATRV)
                tocodeR(add_s, rez, ft1, areg);
            else if (c == MINUSASSR || c == MINUSASSATR || c == MINUSASSRV ||
                     c == MINUSASSATRV)
                tocodeR(sub_s, rez, ft1, areg);
            else if (c == MULTASSR || c == MULTASSATR || c == MULTASSRV ||
                     c == MULTASSATRV)
                tocodeR(mul_s, rez, ft1, areg);
            else if (c == DIVASSR || c == DIVASSATR || c == DIVASSRV || c == DIVASSATRV)
                tocodeR(div_s, rez, ft1, areg);
            
            tocodeB(swc1, areg = rez, leftdispl, leftreg);
        }
        else
        {
            // leftanst == AREG
            if (c == PLUSASSR || c == PLUSASSATR || c == PLUSASSRV || c == PLUSASSATRV)
                tocodeR(add_s, leftreg, leftreg, areg);
            else if (c == MINUSASSR || c == MINUSASSATR || c == MINUSASSRV ||
                     c == MINUSASSATRV)
                tocodeR(sub_s, leftreg, leftreg, areg);
            else if (c == MULTASSR || c == MULTASSATR || c == MULTASSRV ||
                     c == MULTASSATRV)
                tocodeR(mul_s, leftreg, leftreg, areg);
            else if (c == DIVASSR || c == DIVASSATR || c == DIVASSRV || c == DIVASSATRV)
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
            tocodeB(lw, opnd = t1, leftdispl, leftreg);
        else
            opnd = leftreg;
        
        if (manst == CONST)
        {
            if (c == SHLASS || c == SHLASSV || c == SHLASSAT || c == SHLASSATV)
                tocodeSLR(sll, rez, opnd, num);
            else if (c == SHRASS || c == SHRASSV || c == SHRASSAT || c == SHRASSATV)
                tocodeSLR(sra, rez, opnd, num);
            else if (c == ANDASS || c == ANDASSV ||c == ANDASSAT || c == ANDASSATV)
                tocodeI(andi, rez, opnd, num);
            else if (c == ORASS || c == ORASSV || c == ORASSAT || c == ORASSATV)
                tocodeI(ori, rez, opnd, num);
            else if (c == EXORASS|| c == EXORASSV || c == EXORASSAT || c == EXORASSATV)
                tocodeI(xori, rez, opnd, num);
            else if (c == PLUSASS|| c == PLUSASSV || c == PLUSASSAT || c == PLUSASSATV)
                tocodeI(addi, rez, opnd, num);
            else if (c == MINUSASS|| c == MINUSASSV || c == MINUSASSAT || c == MINUSASSATV)
                tocodeI(addi, rez, opnd, -num);
            else if (c == MULTASS ||c == MULTASSV ||c == MULTASSAT ||c == MULTASSATV ||
                     c == DIVASS || c == DIVASSV || c == DIVASSAT || c == DIVASSATV ||
                     c == REMASS || c == REMASSV || c == REMASSAT || c == REMASSATV)
            {
                tocodeI(addi, t9, d0, num);
                flag = 1;
            }
        }
        
        if (manst == AREG || flag)
        {         // здесь второй операнд - это регистр areg или t9
            int aregt9 = flag == 1 ? t9 : areg;
            if (c == SHLASS || c == SHLASSV || c == SHLASSAT || c == SHLASSATV)
                tocodeR(sllv, rez, opnd, areg);
            else if (c == SHRASS || c == SHRASSV || c == SHRASSAT || c == SHRASSATV)
                tocodeSLR(srav, rez, opnd, areg);
            else if (c == ANDASS || c == ANDASSV ||c == ANDASSAT || c == ANDASSATV)
                tocodeR(and, rez, opnd, areg);
            else if (c == ORASS || c == ORASSV || c == ORASSAT || c == ORASSATV)
                tocodeR(or, rez, opnd, areg);
            else if (c == EXORASS|| c == EXORASSV || c == EXORASSAT || c == EXORASSATV)
                tocodeR(xor, rez, opnd, areg);
            else if (c == PLUSASS|| c == PLUSASSV || c == PLUSASSAT || c == PLUSASSATV)
                tocodeR(add, rez, opnd, areg);
            else if (c == MINUSASS|| c == MINUSASSV || c == MINUSASSAT || c == MINUSASSATV)
                tocodeR(sub, rez, opnd, areg);
            else if (c == MULTASS|| c == MULTASSV || c == MULTASSAT || c == MULTASSATV)
                tocodeR(mul, rez, opnd, aregt9);
            else if (c == DIVASS || c == DIVASSV || c == DIVASSAT || c == DIVASSATV)
            {
                tocodeD(divc, opnd, aregt9);
                tocodeMF(mflo, rez);
            }
            else if (c == REMASS || c == REMASSV || c == REMASSAT || c == REMASSATV)
            {
                tocodeD(divc, opnd, aregt9);
                tocodeMF(mfhi, rez);
            }
        }
 
        areg = rez;
    if (leftanst == AREG && mbox <= 2 && leftreg != breg)
        tocodemove(areg = breg, leftreg);
    else   //  if (leftanst == AMEM)
        tocodeB(sw, rez, leftdispl, leftreg);

    manst = AREG;
    }
}

void MBin_operation(int c)      // бинарная операция (два вычислимых операнда)
{
    int oldbox = mbox, oldreg = breg, oldelselab = elselab;
    int leftanst, leftdispl, leftreg, leftnum, rightanst;
    int rez, lopnd, ropnd, flagreal = 2, flagreg = 0;
//    printf("bin form tc= %i mbox= %i manst= %i\n", tc, mbox, manst);
    switch (c)
    {
        case ASSAT:
        case ASSATV:
        case ASSATR:
        case ASSATRV:
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
            if (c == ASSAT || c == ASSATV)
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
            
        case REMASSATV:
        case SHLASSATV:
        case SHRASSATV:
        case ANDASSATV:
        case EXORASSATV:
        case ORASSATV:
            
        case PLUSASSAT:
        case MINUSASSAT:
        case MULTASSAT:
        case DIVASSAT:
            
        case PLUSASSATV:
        case MINUSASSATV:
        case MULTASSATV:
        case DIVASSATV:
            flagreal = 0;
            break;
            
        case PLUSASSATR:
        case MINUSASSATR:
        case MULTASSATR:
        case DIVASSATR:
           
        case PLUSASSATRV:
        case MINUSASSATRV:
        case MULTASSATRV:
        case DIVASSATRV:
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
        MExpr_gen();                                        // левый операнд
        if (manst == AREG)
        {
            lopnd = areg;
            if (mbox ==  BREGF)
                rez = areg;
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
                    tocodeI(addi, t1, ropnd, -leftnum);
                    if (c == LLT)
                        tocodeJC(mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
                    else if (c == LGT)
                        tocodeJC(mbox == BCF ? bgez : bltz, t1, "ELSE", elselab);
                    else if (c == LLE)
                        tocodeJC(mbox == BCF ? bltz : bgez, t1, "ELSE", elselab);
                    else
                        tocodeJC(mbox == BCF ? bgtz : blez, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                
                if (leftanst == AREG && manst == CONST)
                    tocodeI(addi, t1, lopnd, -num);
                else
                    // leftanst == AREG && anst == AREG
                    tocodeR(sub, t1, lopnd, ropnd);

                    if (c == LLT)
                        tocodeJC(mbox == BCF ? bgez : bltz, t1, "ELSE", elselab);
                    else if (c == LGT)
                        tocodeJC(mbox == BCF ? blez : bgtz, t1, "ELSE", elselab);
                    else if (c == LLE)
                        tocodeJC(mbox == BCF ? bgtz : blez, t1, "ELSE", elselab);
                    else
                        tocodeJC(mbox == BCF ? bltz : bgez, t1, "ELSE", elselab);
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
            rez = areg;
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
    int idf, rez, opnd, oldbox = mbox, oldreg = breg, oldelselab = elselab;
    switch (c)
    {
        case ASS:
        case ASSV:
        {
            int ranst, rareg;
            int oldbox = mbox, oldreg = breg, oldelselab = elselab;
            idf = tree[tc++];
            
            if (identab[idf] < 0)
            {
                mbox = BREG;
                rareg = breg = identab[idf + 3];
                MExpr_gen();                       // правый операнд
                mbox = oldbox, breg = oldreg, elselab = oldelselab;
                areg = rareg;
                if (mbox == BREG && breg != rareg)
                    tocodemove(breg, rareg);
            }
            else
            {
                mbox = BF;
                MExpr_gen();                       // правый операнд
                ranst = manst, rareg = areg;
                mdsp(identab[idf + 3]);            // левый операнд
                mbox = oldbox, breg = oldreg, elselab = oldelselab;
                if (ranst == CONST)
                {
                    rareg = mbox <= 2 ? breg : t0;
                    tocodeI(addi, rareg, d0, num);
                }
                tocodeB(sw, rareg, adispl, areg);
                if (mbox == BREG && ranst == AREG && breg != rareg)
                    tocodemove(breg, rareg);
            }
            areg = rareg;
            manst = AREG;
            return;
        }
        case ASSR:
        case ASSRV:
        {
            int rareg;
            int oldbox = mbox, oldreg = breg, oldelselab = elselab;
            idf = tree[tc++];
               
               if (identab[idf] < 0)
               {
                   mbox = BREG;
                   rareg = breg = identab[idf + 3];
                   MExpr_gen();                       // правый операнд
                   mbox = oldbox, breg = oldreg, elselab = oldelselab;
               }
               else
               {
                   MExpr_gen();                       // правый операнд
                   rareg = areg;
                   mdsp(identab[idf + 3]);            // левый операнд
                   mbox = oldbox, breg = oldreg, elselab = oldelselab;
                   
                   tocodeB(swc1, rareg, adispl, areg);
               }
            if (mbox == BREG && breg != rareg)
                tocodemove(breg, rareg);
            manst = AREG;
            areg = rareg;
            return;
        }
 
        case REMASS:
        case SHLASS:
        case SHRASS:
        case ANDASS:
        case EXORASS:
        case ORASS:
            
        case REMASSV:
        case SHLASSV:
        case SHRASSV:
        case ANDASSV:
        case EXORASSV:
        case ORASSV:
            
        case PLUSASS:
        case MINUSASS:
        case MULTASS:
        case DIVASS:
            
        case PLUSASSV:
        case MINUSASSV:
        case MULTASSV:
        case DIVASSV:

        case PLUSASSR:
        case MINUSASSR:
        case MULTASSR:
        case DIVASSR:

        case PLUSASSRV:
        case MINUSASSRV:
        case MULTASSRV:
        case DIVASSRV:
        {
            int leftreg, ranst, rareg;
            int oldbox = mbox, oldreg = breg, oldelselab = elselab;
            idf = tree[tc++];
            
            if (identab[idf] < 0)
            {
                mbox = BF;
                MExpr_gen();              // правый операнд
                mbox = oldbox, breg = oldreg, elselab = oldelselab;
                MASSExpr(c, AREG, 0, identab[idf + 3]);
            }
            else
            {
                mbox = BF;
                MExpr_gen();              // правый операнд
                ranst = manst, rareg = areg;
                mdsp(identab[idf + 3]);   // левый операнд
                manst = ranst;
                leftreg = areg;           // справа не может быть AMEM
                areg = rareg;
                mbox = oldbox, breg = oldreg, elselab = oldelselab;
                MASSExpr(c, AMEM, adispl, leftreg);
            }
            return;
        }
        case POSTINC:
        case INC:
        case POSTINCV:
        case INCV:
            manst = AREG;
            idf = tree[tc++];
            if (identab[idf] < 0)
            {
                opnd = identab[idf+3];
                if (c == POSTINC || c == POSTINCV)
                    tocodemove(t0, opnd);
                tocodeI(addi, opnd, opnd, 1);
            }
            else
            {
                mdsp(identab[idf+3]);
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, opnd = t1, t0, 1);
                tocodeB(sw, t1, adispl, areg);
            }
            areg = c == INC || c == INCV ? opnd : t0;
            break;
        case POSTDEC:
        case DEC:
        case POSTDECV:
        case DECV:
            manst = AREG;
            idf = tree[tc++];
            if (identab[idf] < 0)
            {
                opnd = identab[idf+3];
                if (c == POSTDEC || c == POSTDECV)
                    tocodemove(t0, opnd);
                tocodeI(addi, opnd, opnd, -1);
            }
            else
            {
                mdsp(identab[idf+3]);
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, opnd = t1, t0, -1);
                tocodeB(sw, t1, adispl, areg);
            }
            areg = c == DEC || c == DECV ? opnd : t0;
            break;
        case POSTINCAT:
        case INCAT:
        case POSTINCATV:
        case INCATV:
            mbox = BF;
            MExpr_gen();                  // здесь точно будет manst == AMEM
            tocodeB(lw, t0, adispl, areg);
            tocodeI(addi, t1, t0, 1);
            tocodeB(sw, t1, adispl, areg);
            areg = c == INCAT || c == INCATV ? t1 : t0;
            break;
        case POSTDECAT:
        case DECAT:
        case POSTDECATV:
        case DECATV:
            mbox = BF;
            MExpr_gen();                  // здесь точно будет manst == AMEM
            tocodeB(lw, t0, adispl, areg);
            tocodeI(addi, t1, t0, -1);
            tocodeB(sw, t1, adispl, areg);
            areg = c == DEC || c == DECV ? t1 : t0;
            break;
            
        case TAddrtoval:
             mbox = BF;
             MExpr_gen();
                      // сейчас адрес в регистре areg
             rez = oldbox <= BREGF ? oldreg : t0;
             tocodeB(lw, rez, 0, areg);
             manst = AREG;
             areg = rez;
             BCend();
             break;
              
          case TAddrtovald:
             mbox = BF;
             MExpr_gen();
                      // сейчас адрес в регистре areg
              rez = oldbox <= BREGF ? oldreg : ft0;
              tocodeB(lwc1, rez, 0, areg);
              manst = AREG;
              areg = rez;
              break;
            
         case TPrint:
         {
             int type = tree[tc++];
             mbox = BREG;
             breg = type == LFLOAT || type == LDOUBLE ? fa0 : a0;
             MExpr_gen();
             tocodeI(addi, a1, d0, type);
             tocodeJ(jal, "PRINT", 1);
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
                tocodeMOVE(cvt_s_w, areg = rez, opnd);
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
    if (c == TIdent)
        tc++, c = tree[tc++];
    
    if (c > 10000)
        MBin_operation(c -= 1000); // бинарная операция (два вычислимых операнда)
    else if (c > 9000 || c == TAddrtoval || c == TAddrtovald || c == TPrint)
        MUnar_expr(c);             // унарная операция  (один вычислимый операнд)
    else
       tc--, MPrimary();
    if (tree[tc] == NOP)
        tc++;
    if (tree[tc] == TExprend)
        tc++;
}

/*                else if (c == COPY00 || c == COPYST)
                {
                    tocode(tree[tc++]);   // d1
                    tocode(tree[tc++]);   // d2
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY01)
                {
                    tocode(tree[tc++]);   // d1
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY10 || c == COPY10V)
                {
                    tocode(tree[tc++]);   // d2
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY11 || c == COPY11V)
                    tocode(tree[tc++]);   // длина
                else if (c == COPY0ST)
                {
                    tocode(tree[tc++]);   // d1
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY1ST)
                    tocode(tree[tc++]);   // длина
                
                else if (c == COPY0STASS)
                {
                    tocode(tree[tc++]);   // d1
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY1STASS || c== COPY1STASSV)
                    tocode(tree[tc++]);   // длина
                
                else if((c >= REMASS && c <= DIVASS) || (c >= REMASSV && c <= DIVASSV) ||
                        (c >= ASSR && c <= DIVASSR)  || (c >= ASSRV && c <= DIVASSRV) ||
                        (c >= POSTINC && c <= DEC)   || (c >= POSTINCV && c <= DECV) ||
                        (c >= POSTINCR && c <= DECR) || (c >= POSTINCRV && c <= DECRV))
                {
                    tocode(tree[tc++]);
                }
 */

void MDeclid_gen();

void MStmt_gen();

void MPrimary()
{
    int oldbox = mbox, oldreg = breg, oldelselab = elselab;
    int eltype, rez, d;
//    printf("MPrimary tc= %i tree[tc]= %i\n", tc, tree[tc]);
    if (tree[tc] == TIdent)
        tc += 2;
    switch (tree[tc++])
        {
            case TCondexpr:
            {
                int labend, curelse = elselab = labnum++;
                int oldbox = mbox, oldreg = breg, oldelselab = elselab;
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
//            case TIdent:
//                tc++;          // рудимент, возможно, нужно для выборки поля
//                break;
            case TIdenttoaddr:
                mdsp(tree[tc++]);
                rez = mbox <= BREGF ? breg : t0;
                tocodeI(addi, rez, areg, adispl);
                manst = AREG;
                areg = rez;
                break;
            case TIdenttoval:
            {
                int idp = tree[tc++];
                if (identab[idp] < 0)
                {
                    areg = identab[idp+3];
                    if (mbox == BREG && breg != areg)
                        tocodemove(breg, areg), areg = breg;
                }
                else
                {
                    mdsp(identab[idp+3]);
                    rez = mbox <= BREGF ? breg : t0;
                    tocodeB(lw, rez, adispl, areg);
                    areg = rez;
//                  printf("idtoval adispl= %i areg= %i manst= %i\n", adispl, areg, manst);
                }
                manst = AREG;
                BCend();
            }
                break;
            case TIdenttovald:
            {
                int idp = tree[tc++];
                if (identab[idp] < 0)
                {
                    areg = identab[idp+3];
                    if (mbox == BREG && breg != areg)
                        tocodemove(breg, areg), areg = breg;
                }
                else
                {
                    mdsp(identab[idp+3]);
                    rez = mbox <= BREGF ? breg : ft0;
                    tocodeB(lwc1, rez, adispl, areg);
                    areg = rez;
                }
                manst = AREG;
                break;
            }
            case TConst:
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
            case TConstd:
            	areg = mbox <= 2 ? breg : ft0;
                memcpy(&numdouble, &tree[tc], sizeof(double));
                tc += 2;
                tocodeLI_S(li_s, areg, numdouble);
            	manst = AREG;
                break;
            case TString:
            {
                int n = tree[tc++], i;
                printf("\t.rdata\n\t.align 2\n");
                fprintf(output, "\t.rdata\n\t.align 2\n");
                printf("\t.word %i\n", n);
                fprintf(output, "\t.word %i\n", n);
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
                        printf("%c", c);
                        fprintf(output, "%c", tree[tc++]);
                    }
                }
                printf("\\0\"\n\t.text\n");
                fprintf(output, "\\0\"\n\t.text\n");
                rez = mbox <= BREGF ? breg : t0;
                printf("\tlui $t1, %%hi(STRING%i)\n", stringnum);
                fprintf(output, "\tlui $t1, %%hi(STRING%i)\n", stringnum);
                printf("\taddui %s, $t1, %%lo(STRING%i)\n", regs[rez], stringnum);
                fprintf(output, "\taddui %s, $t1, %%lo(STRING%i)\n", regs[rez], stringnum);
            }
                break;

            case TDeclid:
                MDeclid_gen();
                break;
/*
            case TBeginit:
            {
                int n = tree[tc++], i;
                tocode(BEGINIT);
                tocode(n);
                for (i=0; i<n; i++)
                    MExpr_gen();
            }
                break;
*/
            case TSliceident:
                mdsp(tree[tc++]);              // параметр - смещение
                tocodeB(lw, a0, adispl, areg); // продолжение в след case
            case TSlice:                       // параметр - тип элемента
            {
                int oldbox = mbox, oldreg = breg;
                eltype = tree[tc++];
                mbox = BREG;                       // a0 - это C0
                breg = a1;                         // a1 - это index
                MExpr_gen();
                d = szof(eltype);
                if (eltype == LCHAR)
                    tocodeJ(jal, "SLICE1", 0);     // v0 - адрес i-го элемента
                 else if (eltype > 0)
                 {
                    tocodeI(addi, a2, d0, 4*d);    // a2 - это шаг
                    tocodeJ(jal, "SLICE", 0);      // v0 - адрес i-го элемента
                 }
                 else
                    tocodeJ(jal, "SLICE4", 0);     // v0 - адрес i-го элемента
 
                if (eltype > 0 && modetab[eltype] == MARRAY)
                    tocodeB(lw, v0, 0, v0);
                mbox = oldbox;
                breg = oldreg;
            }
                manst = AMEM;
                adispl = 0;
                if (mbox <= BREGF)
                    tocodemove(areg = breg, v0);
                else
                    areg = v0;
                break;
            case TSelect:
//                tocode(SELECT);                // SELECT field_displ
//                tocode(tree[tc++]);
                break;
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
                    if (mbox <= BREGF)
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
 //   tc++;
    N = tree[tc++];
    if (N > 5)
        merror(dim_greater_5);
    mbox = BREG;
    breg = t0;
    displ = -32764;
    for (i=0; i<N; i++)
    {
        MExpr_gen();
        tocodeB(sw, t0, displ, gp);
        displ += 4;
    }

}

void mcompstmt_gen()
{
    mbox = BV;
    while (tree[tc] != TEnd)
    {
        switch (tree[tc])
        {
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
            int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++],
            stmtref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, incrtc, endtc;
            mbox = BV;
            if (fromref)
                MExpr_gen();         // init
            adbreak = elselab = labnum++;
            adcont  = labnum++;
        tocodeL("BEGLOOP", adcont);
            if (condref)
            {
                mbox = BCF;
                MExpr_gen();         // cond
            }
            if (incrref)
            {
                mbox = BV;
                incrtc = incrref;
                tc = stmtref;
                MStmt_gen();         // statement
                endtc = tc;
                tc = incrtc;
            tocodeL("CONT", adcont);
                MExpr_gen();         // incr
                tc = endtc;
            }
            else
            {
                MStmt_gen();         // statement
            tocodeL("CONT", adcont);
            }
            tocodeJ(jump, "BEGLOOP", adcont);
        tocodeL("end", adbreak);
        tocodeL("ELSE", adbreak);

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
            adbreak = labnum++;
            adcase = labnum++;
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
            mbox = BF;
            MExpr_gen();
            tocodeJEQ(bne, t0, switchreg, "CASE", adcase = labnum++);
    tocodeL("CASE", ocase);
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
/*
        case TPrintid:
        {
            tocode(PRINTID);
            tocode(tree[tc++]);  // ссылка в identtab
        }
            break;
 */
        case TPrintf:
        {
 //           tocode(PRINTF);
            tc++;  // общий размер того, что надо вывести
            mbox = BREG;
            breg = a1;
            MExpr_gen();
            breg = a0;
            MExpr_gen();
            printf("\tlw $t9, %%call16(printf)($gp)\n");
            fprintf(output, "\tlw $t9, %%call16(printf)($gp)\n");
            tocodeJR(jalr, t9);
        }
            break;
/*
        case TGetid:
        {
            tocode(GETID);
            tocode(tree[tc++]);  // ссылка в identtab
        }
            break;
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
    int olddispl = identab[oldid+3];
    element_len = szof(telem);
    
    if (N == 0)                    // обычная переменная int a; или struct point p;
    {
        if (identab[oldid] < 0)    // регистровая переменная
        {
            if (telem == LINT || telem == LLONG)
                identab[oldid+3] = getreg();
            else if (telem == LFLOAT || telem == LDOUBLE)
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
            {/*
                do
                    Expr_gen(0);
                while (tree[tc] != TEndinit);
                tc++;
                tocode(COPY0STASS);
                tocode(olddispl);
                tocode(all);       // общее кол-во слов   */
            }
            else
            {
                int oldbox = mbox, oldreg = breg, oldelselab = elselab;
                int flagfloat = 0;
                mbox = BREG;
                if (identab[oldid] < 0)    // регистровая переменная
                {
                    breg = identab[oldid+3];
                    MExpr_gen();
                }
                else
                {
                breg = telem == LFLOAT || telem == LDOUBLE ? flagfloat = 1, ft0 : t0;
                    MExpr_gen();
                    mdsp(olddispl);
                    if (flagfloat)
                        tocodeB(swc1, ft0, adispl, areg);
                    else
                        tocodeB(sw, t0, adispl, areg);
                }
                mbox = oldbox; breg = oldreg; elselab = oldelselab;

             /*   tocode(telem == LFLOAT ? ASSRV : ASSV);
                tocode(olddispl);  */
            }
        }
    }
    else                                // Обработка массива int a[N1]...[NN] =
    {
        // DEFARR N, d, displ, iniproc, usual     N1...NN уже лежат в глоб данных
        mdsp(olddispl);
        tocodeB(lw, t0, -32764, gp);    // NN
        tocodeSLR(sll, t1, t0, 2);      // NN*4
        tocodeR(sub, stp, stp, t1);     // захват памяти под NN элементов
        tocodeB(sw, stp, adispl, areg); // C0, то есть адрес нулевого элемента
        tocodeI(addi, stp, stp, -4);
        tocodeB(sw, t0, 0, stp);        // NN
        /*
        tocode(all == 0 ? N : abs(N)-1);
        tocode(element_len);
        tocode(olddispl);
        tocode(iniprocs[iniproc]);
        tocode(usual);
        tocode(all);
        tocode(instruct);
        
        if (all)                        // all == 1, если есть инициализация массива
        {
            Expr_gen(0);
            tocode(ARRINIT);        // ARRINIT N d all displ usual
            tocode(abs(N));
            tocode(element_len);
            tocode(olddispl);
            tocode(usual);             // == 0 с пустыми границами
                                       // == 1 без пустых границ и без иниц
        }
      */
      }
}

void mipsgen()
{
    int treesize = tc;
    maxdisplg = (maxdisplg + 2) * 4;
    tc = 0;
    notrobot = 0;
    if (wasmain == 0)
        error(no_main_in_program);

tocodeL("START", 0);
    tocodeB(sw, stp, -32768, gp);
    tocodeB(sw, ra, 0, stp);
    printf("\n");
    fprintf(output, "\n");

//    tocodeI(addi, stp, stp, -4);
//    tocodemove(s8, stp);
 
    while (tc < treesize)
    {
        switch (tree[tc++])
        {
            case TEnd:
                break;
            case TFuncdef:
            {
                int i, n, ftype, id, a0i = a0, a0f = fa0;
                identref =  tree[tc++];
                ftype = identab[identref+2];
                n = modetab[md+2];
                id = identref + 4;
                    for (i=0; i < n; i++)
                    {
                        int ptype = modetab[ftype+3+i];
                        identab[id] = -identab[id];
                        if (ptype == LFLOAT || ptype == LDOUBLE)
                        {
                            identab[id] = a0f++;
                            if (a0f == 44)
                                merror(too_many_params);
                        }
                        else
                        {     // параметры массивы и структуры передаются адресом
                            identab[id] = a0i++;
                            if (a0i == 8)
                                merror(too_many_params);
                        }
                    id += 4;
                }
                tocodeJ(jump, "NEXT", identref);
            tocodeL("FUNC", identab[identref+3]);
                maxdispl = (tree[tc++] - 3) * 4;
                tocodeI(addi, stp, stp, -maxdispl - 56);
                tocodeB(sw, fp, 0, stp);
                tocodemove(fp, stp);
                tocodeB(sw, ra, 4, fp);
                printf("\n");
                fprintf(output, "\n");
                
                tc++;             // TBegin
                mcompstmt_gen();
                printf("\n");
                fprintf(output, "\n");
            tocodeL("FUNCEND", identref);
                tocodeB(lw, ra, 4, fp);
                tocodeI(addi, stp, fp, maxdispl + 56);
                tocodeB(lw, fp, 0, fp);
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
    tocodeB(lw, stp, -32768, gp);
    tocodeB(lw, ra, 0, stp);
    tocodeJR(jr, ra);

tocodeL("SLICE", 0);               // a0 = C0,  a1 = index, a2 = d
    tocodeB(lw, t0, -4, a0);       // t0 = N
    tocodeJCimm(bltz, a1, 3);      // if (index < 0 err
    tocodeR(sub, t1, a1, t0);
    tocodeJCimm(bltz, t1, 2);      // if (index - N < 0) ok
    tocodeR(add, a0, d0, t0);     // a0 = N, a1 = index
    tocodeJ(jal, "ERR", 0);
    tocodeR(mul, t0, a1, a2);      // t0 = index * d
    tocodeR(add, v0, t0, a0);      // v0 = C0 + t0
    tocodeJR(jr, ra);              // v0 - адрес i-го элемента
    
tocodeL("SLICE1", 0);              // a0 = C0,  a1 = index, d == 1
    tocodeB(lw, t0, -4, a0);       // t0 = N
    tocodeJCimm(bltz, a1, 3);      // if (index < 0 err
    tocodeR(sub, t1, a1, t0);
    tocodeJCimm(bltz, t1, 2);      // if (index - N < 0) ok
    tocodeR(add, a0, d0, t0);     // a0 = N, a1 = index
    tocodeJ(jal, "ERR", 0);
    tocodeR(add, v0, a1, a0);      // v0 = C0 + index
    tocodeJR(jr, ra);              // v0 - адрес i-го элемента
    
tocodeL("SLICE4", 0);               // a0 = C0,  a1 = index, d == 4
    tocodeB(lw, t0, -4, a0);       // t0 = N
    tocodeJCimm(bltz, a1, 3);      // if (index < 0 err
    tocodeR(sub, t1, a1, t0);
    tocodeJCimm(bltz, t1, 2);      // if (index - N < 0) ok
    tocodeR(add, a0, d0, t0);     // a0 = N, a1 = index
    tocodeJ(jal, "ERR", 0);
    tocodeSLR(sll, t0, a1, 2);     // t0 = index * 4
    tocodeR(add, v0, t0, a0);      // v0 = C0 + t0
    tocodeJR(jr, ra);              // v0 - адрес i-го элемента


tocodeL("ERR", 0);
    merror(wrong_index);
    printf("\n");
    fprintf(output, "\n");

}


