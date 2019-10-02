//
//  mip16gen.c
//
//  Created by Andrey Terekhov on 16/12/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//
#include <stdlib.h>
#include "global_vars.h"

// запросы
#define BREG   1     // in breg destination
#define BREGF  2     // регистровый запрос на левый операнд, но могут ответить CONST
#define BF     3     // свободный запрос на правый операнд
#define BC     4     // for conditions  BC > 0 on TRUE, BC < 0 on FALSE
                     // goto на метку elselab

// ответы
#define AMEM  1      // in adispl and areg address
#define AREG  2      // in areg register number
#define CONST 3      // in num int const

int mbox, breg, elselab;
int manst, adispl, areg;
int labnum = 1, stringnum = 1, elselab, flagBC;

// унарные операции LNOT, LOGNOT, -, ++, --, TIdenttoval(*), TIdenttoaddr(&)
// LNOT nor rd, rs, d0    LOGNOT slti rt, rs, 1   - sub rd, d0, rt
// *  lw rt, displ(rs) или сразу 0(areg)   & addi rt, areg, adispl или сразу areg
// ++ lw rt, adispl(areg) addi rt, rt, 1 sw rt, adispl(areg)
extern void error(int err);
extern int szof(int);

int pc = 32, d0 = 0, at = 1, v0 = 2, v1 = 3, a0 = 4, a1 = 5, a2 = 6, a3 = 7,
    t0 = 8, t1 = 9, t2 = 10, t9 = 25, s0 = 16, gp = 28, stp = 29, s8 = 30, ra =  31;

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
/*100 */ "", "", "slt", "sltu"
};

char *regs[] =
{/* 0 */ "$0",  "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1",
/* 10 */ "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3",
/* 20 */ "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp",
/* 30 */ "$s8", "$ra", "pc"
};

int tregs[] =
{
    8, 9, 10, 11, 12, 13, 14, 15, 24, 25
};

int sregs[] =
{
    16, 17, 18, 19, 20, 21, 22, 23, 30
};

#define too_many_sregs    1
#define too_many_params   2
#define dim_greater_5     3

void merror(int type)
{
    switch (type)
    {
        case too_many_sregs:
            printf("кончились регистры s0-s7\n");
            break;
        case too_many_params:
            printf("пока количество параметров ограничено 4\n");
            break;
        case dim_greater_5:
            printf("в РуСи размерность массивов не может быть больше 5\n");
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
int nextreg = 16;

int getreg()
{
    if (nextreg == 24)
        merror(too_many_sregs);
    return nextreg++;
}

void freereg(int r)
{
    nextreg--;
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
        areg = stp;
        adispl = 4 * (displ - 3);
    }
}

void MPrimary();

void MASSExpr(int c, int rez, int leftanst)
{
    int flag = 0, ropnd;
    ropnd = leftanst == AREG ? areg  : t0;
    
    if (c == ASS || c == ASSV || c == ASSAT || c == ASSATV)
    {
        if (manst == CONST)   // здесь уже точно num < 32768
            tocodeI(addi, ropnd, d0, num);
        
        if (leftanst != AREG)
            tocodeB(sw, ropnd, adispl, areg);
        manst = AREG;
        areg = ropnd;
    }
    else
    {
        int opnd = leftanst == AREG ? areg :(tocodeB(lw, t1, adispl, areg), t1);
        
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
            else if (c == MULTASS || c == MULTASSV || c == MULTASSAT || c == MULTASSATV ||
                     c == DIVASS || c == DIVASSV || c == DIVASSAT || c == DIVASSATV ||
                     c == REMASS || c == REMASSV || c == REMASSAT || c == REMASSATV)
            {
                tocodeI(addi, ropnd, d0, num);
                flag = 1;
            }
        }
        
        if (manst == AREG || flag)
        {                          // здесь второй операнд - это регистр ropnd
            if (c == SHLASS || c == SHLASSV || c == SHLASSAT || c == SHLASSATV)
                tocodeR(sllv, rez, opnd, ropnd);
            else if (c == SHRASS || c == SHRASSV || c == SHRASSAT || c == SHRASSATV)
                tocodeSLR(srav, rez, opnd, ropnd);
            else if (c == ANDASS || c == ANDASSV ||c == ANDASSAT || c == ANDASSATV)
                tocodeR(and, rez, opnd, ropnd);
            else if (c == ORASS || c == ORASSV || c == ORASSAT || c == ORASSATV)
                tocodeR(or, rez, opnd, ropnd);
            else if (c == EXORASS|| c == EXORASSV || c == EXORASSAT || c == EXORASSATV)
                tocodeR(xor, rez, opnd, ropnd);
            else if (c == PLUSASS|| c == PLUSASSV || c == EXORASSAT || c == EXORASSATV)
                tocodeR(add, rez, opnd, ropnd);
            else if (c == MINUSASS|| c == MINUSASSV || c == MINUSASSAT || c == MINUSASSATV)
                tocodeR(sub, rez, opnd, ropnd);
            else if (c == MULTASS|| c == MULTASSV || c == MULTASSAT || c == MULTASSATV)
                tocodeR(mul, rez, opnd, ropnd);
            else if (c == DIVASS || c == DIVASSV || c == DIVASSAT || c == DIVASSATV)
            {
                tocodeD(divc, opnd, ropnd);
                tocodeMF(mflo, rez);
            }
            else if (c == REMASS || c == REMASSV || c == REMASSAT || c == REMASSATV)
            {
                tocodeD(divc, opnd, ropnd);
                tocodeMF(mfhi, rez);
            }
            
            if (leftanst != AREG)
                tocodeB(sw, rez, adispl, areg);
        }
    }
    manst = AREG;
    areg = rez;
}

void MBin_operation(int c)      // бинарная операция (два вычислимых операнда)
{
    int obox = mbox, oldreg = breg, oldelselab = elselab;
    int leftanst, leftdispl, leftareg, leftreg, leftnum, rightanst;
    int rez, ropnd, labend = labnum++;
//    printf("bin form tc= %i mbox= %i manst= %i\n", tc, mbox, manst);
    flagBC = 1;
    if (c == LOGAND || c == LOGOR)
    {
        tc++;
        if (abs(mbox) == BC)
        {
            mbox = BREGF;
            rez = leftreg = breg = getreg();
            MExpr_gen();
            if (tree[tc] == ADLOGAND || tree[tc] == ADLOGOR)
                tc += 2;
            
            if (manst == CONST)
                tocodeI(addi, rez, d0, num == 0 ? 0 : 1);
            // manst == AREG || manst == CONST
            if (c == LOGAND)
                tocodeJC(blez, rez, "ELSE", obox > 0 ? labend : elselab);
            else
                tocodeJC(bgtz, rez, "ELSE", obox > 0 ? elselab : labend);
            
            //            flagBC = 0;
            mbox = BF;
            MExpr_gen();
            if (manst == CONST)
                tocodeI(addi, t1, d0, num == 0 ? 0 : 1);
            // manst == AREG || manst == CONST
            tocodeJC(obox > 0 ? bgtz : blez, areg, "ELSE", elselab);
        tocodeL("ELSE", labend);
            mbox = obox; breg = oldreg; elselab = oldelselab;
            freereg(rez);
            flagBC = 0;
            return;
        }
    }
    
    rez = mbox == BREG || mbox == BREGF ?  breg : t0;
    mbox = BREGF, leftreg = breg = getreg();
    MExpr_gen();                               // левый операнд
    leftanst = manst;
    leftdispl =  adispl;
    leftareg = areg;
    leftnum = num;

    if (tree[tc] == ADLOGAND || tree[tc] == ADLOGOR)
        tc += 2;
    
    if (c == LOGAND || c == LOGOR)
    {
        if (leftanst == CONST)
            tocodeI(addi, rez, d0, leftnum == 0 ? 0 : 1);
        
        tocodeI(addi, rez == t0 ? t1 : rez, d0, c == LOGAND ? 0 : 1);
        tocodeJC(c == LOGAND ? blez : bgtz, leftreg, "ELSE", labend);
    }
    
    mbox = BF;
    MExpr_gen();                               // правый операнд
    rightanst = manst;
    ropnd = areg;
    
    if (c == LOGAND || c == LOGOR)
    {
        if (manst == CONST)
            tocodeI(addi, ropnd = t0, d0, num == 0 ? 0 : 1);
        
        tocodeJC(c == LOGAND ? blez : bgtz, ropnd, "ELSE", labend);
        tocodeI(addi, areg = rez == t0 ? t1 : rez, d0, c == LOGAND ? 1 : 0);
    tocodeL("ELSE", labend);
        flagBC = 0;
        freereg(leftreg);
        mbox = obox; breg = oldreg; elselab = oldelselab;
        return;
    }

    areg = rez;
    freereg(leftreg);
    mbox = obox; breg = oldreg; elselab = oldelselab;
    
    if (abs(mbox) == BC)
    {
        switch (c)
        {
            case EQEQ:
            case NOTEQ:
                if (leftanst == CONST && manst == CONST)
                {
                    tocodeI(addi, t1, d0, leftnum == num ? 1 : 0);
                    tocodeJC(obox < 0 ? blez : bgtz, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                if (leftanst == CONST && manst == AREG)
                    tocodeI(addi, leftreg, d0, leftnum);
                else if (leftanst == AREG && manst == CONST)
                    tocodeI(addi, leftreg, d0, num);
                // leftanst == AREG && anst == AREG
                if (c == EQEQ)
                    tocodeJEQ(obox < 0 ? bne : beq, leftreg, ropnd, "ELSE", elselab);
                else
                    tocodeJEQ(obox > 0 ? bne : beq, leftreg, ropnd, "ELSE", elselab);
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
                    tocodeJC(obox < 0 ? blez : bgtz, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                if (leftanst == CONST && manst == AREG)
                {
                    tocodeI(addi, t1, ropnd, -leftnum);
                    if (c == LLT)
                        tocodeJC(obox < 0 ? blez : bgtz, t1, "ELSE", elselab);
                    else if (c == LGT)
                        tocodeJC(obox < 0 ? bgez : bltz, t1, "ELSE", elselab);
                    else if (c == LLE)
                        tocodeJC(obox < 0 ? bltz : bgez, t1, "ELSE", elselab);
                    else
                        tocodeJC(obox < 0 ? bgtz : blez, t1, "ELSE", elselab);
                    flagBC = 0;
                    return;
                }
                if (leftanst == AREG && manst == CONST)
                    tocodeI(addi, t1, leftreg, -num);
                else                       // leftanst == AREG && anst == AREG
                    tocodeR(sub, t1, leftreg, ropnd);

                    if (c == LLT)
                        tocodeJC(obox < 0 ? bgez : bltz, t1, "ELSE", elselab);
                    else if (c == LGT)
                        tocodeJC(obox < 0 ? blez : bgtz, t1, "ELSE", elselab);
                    else if (c == LLE)
                        tocodeJC(obox < 0 ? bgtz : blez, t1, "ELSE", elselab);
                    else
                        tocodeJC(obox < 0 ? bltz : bgez, t1, "ELSE", elselab);
                flagBC = 0;
                return;
        }
    }

    
    switch (c)
    {
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
            
        case ASSAT:
        case PLUSASSAT:
        case MINUSASSAT:
        case MULTASSAT:
        case DIVASSAT:
            
        case ASSATV:
        case PLUSASSATV:
        case MINUSASSATV:
        case MULTASSATV:
        case DIVASSATV:
        {
            displ = leftdispl;
            areg  = leftareg;
            MASSExpr(c, mbox == BREG || mbox == BREGF ? breg : t0, leftanst);
            return;
        }
    }

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
            if (obox == BREG)
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
        }
    }
    
    if (leftanst == AREG && rightanst == CONST)
    {
        switch (c)
        {
            case LREM:
                tocodeI(addi, t0, d0, num);
                tocodeD(divc, leftreg, t0);
                tocodeMF(mfhi, rez);
                return;
            case LSHL:
                tocodeSLR(sll, rez, leftreg, num);
                return;
            case LSHR:
                tocodeSLR(sra, rez, leftreg, num);
                return;
            case LAND:
                tocodeI(andi, rez, leftreg, num);
                return;
            case LEXOR:
                tocodeI(xori, rez, leftreg, num);
                return;
            case LOR:
                tocodeI(ori, rez, leftreg, num);
                return;
            case LPLUS:
                tocodeI(addi, rez, leftreg, num);
                return;
            case LMINUS:
                tocodeI(addi, rez, leftreg, -num);
                return;
            case LMULT:
                tocodeI(addi, t0, d0, num);
                tocodeR(mul, rez, leftreg, t0);
                return;
            case LDIV:
                tocodeI(addi, t0, d0, num);
                tocodeD(divc, leftreg, t0);
                tocodeMF(mflo, rez);
                return;
            case EQEQ:
                tocodeI(addi, t1, d0, num);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(beq, leftreg, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case NOTEQ:
                tocodeI(addi, t1, d0, num);
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(bne, leftreg, t1, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case LLT:
                tocodeI(slti, rez, leftreg, num);
                return;
            case LGT:
                tocodeI(slti, rez, leftreg, num+1);
                tocodeI(xori, rez, rez, 1);
                return;
            case LLE:
                tocodeI(slti, rez, leftreg, num+1);
                return;
            case LGE:
                tocodeI(slti, rez, leftreg, num);
                tocodeI(xori, rez, rez, 1);
                return;
        }
    }
    
    if (leftanst == AREG && rightanst == AREG)
    {
        switch (c)
        {
            case LREM:
                tocodeD(divc, leftreg, ropnd);
                tocodeMF(mfhi, rez);
                return;
            case LSHL:
                tocodeR(sllv, rez, leftreg, ropnd);
                return;
            case LSHR:
                tocodeR(srav, rez, leftreg, ropnd);
                return;
            case LAND:
                tocodeR(and, rez, leftreg, ropnd);
                return;
            case LEXOR:
                tocodeR(xor, rez, leftreg, ropnd);
                return;
            case LOR:
                tocodeR(or, rez, leftreg, ropnd);
                return;
            case LPLUS:
                tocodeR(add, rez, leftreg, ropnd);
                return;
            case LMINUS:
                tocodeR(sub, rez, leftreg, ropnd);
                return;
            case LMULT:
                tocodeR(mul, rez, leftreg, ropnd);
                return;
            case LDIV:
                tocodeD(divc, leftreg, ropnd);
                tocodeMF(mflo, rez);
                return;
                
            case EQEQ:
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(beq, leftreg, ropnd, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case NOTEQ:
                tocodeI(addi, rez, d0, 1);
                tocodeJEQimm(bne, leftreg, ropnd, 1);
                tocodeI(addi, rez, d0, 0);
                return;
            case LLT:
                tocodeR(slt, rez, leftreg, ropnd);
                return;
            case LGT:
                tocodeR(slt, rez, ropnd, leftreg);
                return;
            case LLE:
                tocodeR(slt, rez, ropnd, leftreg);
                tocodeI(xori, rez, rez, 1);
                return;
            case LGE:
                tocodeR(slt, rez, leftreg, ropnd);
                tocodeI(xori, rez, rez, 1);
                return;
        }
    }
}


void MUnar_expr(int c)
{
    int idf, opnd;
    switch (c)
    {
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
            
        case ASS:
        case PLUSASS:
        case MINUSASS:
        case MULTASS:
        case DIVASS:
            
        case ASSV:
        case PLUSASSV:
        case MINUSASSV:
        case MULTASSV:
        case DIVASSV:
        {
            idf = tree[tc++];
            MExpr_gen();
            if (identab[idf] < 0)
            {
                manst = AREG;
                MASSExpr(c, areg = identab[idf + 3], manst);
            }
            else
            {
                mdsp(identab[idf + 3]);
                MASSExpr(c, mbox == BREG || mbox == BREGF ? breg : t0, manst);
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
        case LNOT:          // поразрядное отрицание:
        case LOGNOT:
        case UNMINUS:
        {
            int r = mbox == BREG ? breg : t0;
            mbox = BF;
            MExpr_gen();
            if (manst == CONST)
            {
                if (c == LNOT)
                    num = ~num;
                else if (c == LOGNOT)
                    num = !num;
                else                 // UNMINUS
                    num = -num;
                
                tocodeI(addi, r, d0, num);
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
}

void MExpr_gen()
{
    int c = tree[tc++];
    if (abs(mbox) == BC && c == LOGNOT)
        mbox = -mbox;
    if (c == TIdent)
        tc++, c = tree[tc++];
    
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
        if (abs(mbox) == BC && flagBC)
            tocodeJEQ(mbox < 0 ? beq : bne, areg, d0, "ELSE", elselab);
    }
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
                int labend, curelse = elselab = labnum++, oldbox = mbox;
                labend = labnum++;
                mbox = -BC;
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
                tc++;          // рудимент, возможно, нужно для выборки поля
                break;
            case TIdenttoaddr:
                mdsp(tree[tc++]);
                rez = mbox == BREG || mbox == BREGF ? breg : t0;
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
                    rez = mbox == BREG || mbox == BREGF ? breg : t0;
                    tocodeB(lw, rez, adispl, areg);
                    areg = rez;
//                  printf("idtoval adispl= %i areg= %i manst= %i\n", adispl, areg, manst);
                }
                manst = AREG;
            }
                break;
                
/*            case TIdenttovald:
                tocode(LOADD);
                tocode(tree[tc++]);
                break;
 */
            case TAddrtoval:       // сейчас адрес в регистре areg
                rez = mbox == BREG || mbox == BREGF ? breg : t0;
                tocodeB(lw, rez, 0, areg);
                manst = areg;
                areg = rez;
                break;
                
/*            case TAddrtovald:
                tocode(LATD);
                break;
 */
            case TConst:
                manst = CONST;
                num = tree[tc++];
                if (mbox != BF || abs(num) >= 32768)
                {
                    rez = mbox == BREG || mbox == BREGF ? breg : t0;
                    if (abs(num) >= 32768)
                        tocodeLI(li, rez, num);
                    else
                        tocodeI(addi, rez, d0, num);
                    manst = AREG;
                    areg = rez;
                }
                break;
                
/*            case TConstd:
                tocode(LID);
                tocode(tree[tc++]);
                tocode(tree[tc++]);
                break;
*/
            case TString:
            {
                int n = tree[tc++], i,
                rez = mbox == BREG || mbox == BREGF ? breg : t0;
                tocodeJ(jump, "STRING", stringnum);
                printf("%i\n", n);
                fprintf(output, "%i\n", n);
                tocodeR(add, rez, d0, pc);
                printf("%c", '\"');
                fprintf(output, "%c", '\"');
                for (i=0; i<n; i++)
                {
                    printf("%c", tree[tc]);
                    fprintf(output, "%c", tree[tc++]);
                }
                printf("%c\n", '\"');
                fprintf(output, "%c\n", '\"');
                tocodeL("STRING", stringnum++);
//                wasstring = 1;
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
                eltype = tree[tc++];
                mbox = BREG;                   // a0 - это C0
                breg = a1;                     // a1 - это index
                MExpr_gen();
                d = szof(eltype);
                if (eltype == LCHAR)
                    tocodeI(addi, a2, d0, d);  // a2 - это шаг
                else
                    tocodeI(addi, a2, d0, 4*d);// a2 - это шаг
                tocodeJ(jal, "SLICE", 1);      // v0 - адрес i-го элемента
 
                if (eltype > 0 && modetab[eltype] == MARRAY)
                    tocodeB(lw, v0, 0, v0);
                manst = AMEM;
                adispl = 0;
                areg = v0;
                break;
            case TSelect:
//                tocode(SELECT);                // SELECT field_displ
//                tocode(tree[tc++]);
                break;
            case TPrint:
            {
                int type = tree[tc++];
                mbox = BREG;
                breg = a0;
                MExpr_gen();
                tocodeI(addi, a1, d0, type);
                tocodeJ(jal, "PRINT", 1);
            }
                break;
            case TCall1:
            {
                int i, n = tree[tc++];
                if (n > 4)
                    merror(too_many_params);
                mbox = BREG;
                for (i=0; i < n; i++)
                {
                    breg = a0 + i;
                    MExpr_gen();
                }
            }
                break;
            case TCall2:
                tocodeJ(jal, "FUNC", identab[tree[tc++]+3]);
                mbox = AREG;
                areg = v0;
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
    mbox = BF;
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
            mbox = -BC;
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
            mbox = -BC;
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
            mbox = BC;
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
            int r = getreg();
            int idfrom = identab[fromref], idfrom3 = identab[fromref+3];
            if (fromref)
            {
                identab[fromref] *= -1;
                identab[fromref+3] = r;
                MExpr_gen();         // init
            }
            adbreak = elselab = labnum++;
            adcont  = labnum++;
            tocodeL("CONT", adcont);
            if (condref)
            {
                mbox = -BC;
                MExpr_gen();         // cond
            }
            if (incrref)
            {
                incrtc = incrref;
                tc = stmtref;
                MStmt_gen();         // statement
                endtc = tc;
                tc = incrtc;
                MExpr_gen();         // incr
                tc = endtc;
            }
            else
                MStmt_gen();         // statement
            tocodeJ(jump, "CONT", adcont);
        tocodeL("end", adbreak);
        tocodeL("ELSE", adbreak);

            adbreak = oldbreak;
            adcont = oldcont;
            if (fromref)
                tocodeB(sw, r, idfrom3, stp);
            identab[fromref] = idfrom;
            identab[fromref+3] = idfrom3;
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
            breg = v0;
            MExpr_gen();
        case TReturnvoid:
            printf("\n");
            tocodeB(lw, ra, maxdispl + 36, stp);
            tocodeI(addi, stp, stp, maxdispl + 56);
            tocodeJR(jr, ra);
            break;
/*
        case TPrintid:
        {
            tocode(PRINTID);
            tocode(tree[tc++]);  // ссылка в identtab
        }
            break;
        case TPrintf:
        {
            tocode(PRINTF);
            tocode(tree[tc++]);  // общий размер того, что надо вывести
        }
            break;
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
    int olddispl = tree[tc++], telem = tree[tc++], N = tree[tc++], element_len,
    all = tree[tc++], iniproc = tree[tc++], usual = tree[tc++], instruct = tree[tc++];
    // all - общее кол-во слов в структуре
    //  для массивов есть еще usual
    // == 0 с пустыми границами,
    // == 1 без пустых границ,
    // all == 0 нет инициализатора,
    // all == 1 есть инициализатор
    // all == 2 есть инициализатор только из строк
    element_len = szof(telem);
    
    if (N == 0)                    // обычная переменная int a; или struct point p;
    {
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
                mbox = BREG;
                breg = t0;
                MExpr_gen();
                mdsp(olddispl);
                tocodeB(sw, t0, adispl, areg);

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
                int identref =  tree[tc++];
                tocodeJ(jump, "FUNCEND", identref);
            tocodeL("FUNC", identab[identref+3]);
                maxdispl = (tree[tc++] - 3) * 4;
                tocodeI(addi, stp, stp, -maxdispl - 56);
                tocodemove(s8, stp);
                tocodeB(sw, ra, maxdispl + 36, stp);
                printf("\n");
                fprintf(output, "\n");
                
                tc++;             // TBegin
                mcompstmt_gen();
            tocodeL("FUNCEND", identref);
                printf("\n");
                fprintf(output, "\n");
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

tocodeL("SLICE", 1);               // a0 = C0,  a1 = index, a2 = d
    tocodeB(lw, t0, -4, a0);       // t0 = N
    tocodeJCimm(bltz, a1, 3);      // if (index < 0 err
    tocodeR(sub, t1, a1, t0);
    tocodeJCimm(bltz, t1, 2);      // if (index - N < 0) ok
    tocodeR(addi, a0, d0, t0);     // a0 = N, a1 = index
    tocodeJ(jal, "ERR", 0);
    tocodeR(mul, t0, a1, a2);      // t0 = index * d
    tocodeR(add, v0, t0, a0);      // v0 = C0 + t0
    tocodeJR(jr, ra);              // v0 - адрес i-го элемента
tocodeL("ERR", 0);
    printf("\n");
    fprintf(output, "\n");

}


