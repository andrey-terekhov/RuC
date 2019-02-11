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
#define BF     2     // free, в adispl и areg ответ, если breg > 0, то используется этот
                     // регистр, иначе один из несохраняемых (чаще всего t0)
#define BC     3     // for conditions  BC > 0 BTRUE BC < 0 BFALSE

// ответы
#define AMEM  1      // in adispl and areg address
#define AREG  2      // in areg register number
#define CONST 3      // in num int const

int mbox,  bdispl, breg;
int manst, adispl, areg;
int labnum = 1;

// унарные операции LNOT, LOGNOT, -, ++, --, TIdenttoval(*), TIdenttoaddr(&)
// LNOT nor rd, rs, d0    LOGNOT slti rt, rs, 1   - sub rd, d0, rt
// *  lw rt, displ(rs) или сразу 0(areg)   & addi rt, areg, adispl или сразу areg
// ++ lw rt, adispl(areg) addi rt, rt, 1 sw rt, adispl(areg)
extern void error(int err);
extern int szof(int);

int d0 = 0, at = 1, v0 = 2, v1 = 3, a0 = 4, a1 = 5, a2 = 6, a3 = 7,
    t0 = 8, t1 = 9, t2 = 10, t9 = 25, s0 = 16, gp = 28, stp = 29, s8 = 30, ra =  31;

char *mcodes[] =
{/* 0 */ "bltz", "", "j", "jal", "beq", "bne", "blez", "bgtz", "addi", "addiu",
/* 10 */ "slti", "sltiu", "andi", "ori", "xori", "lui", "", "", "", "",
/* 20 */ "", "", "", "", "", "", "", "", "mul", "",
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
/* 30 */ "$s8", "$ra"
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

            
        default:
            break;
    }
}

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
}

void tocodeI(int op, int rt, int rs, int imm)  // addi rt, rs, imm   slti rt, rs, imm
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rt], regs[rs], imm);
}

void tocodeB(int op, int rt, int displ, int rs)  // lw rt, displ(rs)  sw rt, displ(rs)
{
    printf("\t%s %s, %i(%s)\n", mcodes[op], regs[rt], displ, regs[rs]);
}

void tocodeLI(int op, int rt, int imm)  // lui rt, imm,   li rt, imm(32)
{
    printf("\t%s %s, %i\n", mcodes[op], regs[rt], imm);
}

void tocodeSLR(int op, int rd, int rt, int shamt)  // sll rd, rt, shamt    srl sra
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
}

void tocodeJ(int op, char type[], int label)  // j label   jal label
{
    printf("\t%s %s%i\n", mcodes[op], type, label);
}

void tocodeJR(int op, int rs)   // jr rs    jalr rs
{
    printf("\t%s %s\n", mcodes[op], regs[rs]);
}

void tocodeJC(int op, int rs, char type[], int label)  // bltz rs, label    bgez blez bgtz
{
    printf("\t%s %s, %s%i\n", mcodes[op], regs[rs], type, label);
}

void tocodeJCimm(int op, int rs, int imm)  // bltz rs, imm    bgez blez bgtz
{
    printf("\t%s %s, %i\n", mcodes[op], regs[rs], imm);
}

void tocodeJEQ(int op, int rs, int rt, char type[], int label)  // beq rs, rt, label   bne
{
    printf("\t%s %s, %s, %s%i\n", mcodes[op], regs[rs], regs[rt], type, label);
}

void tocodeJEQimm(int op, int rs, int rt, int imm)  // beq rs, rt, imm   bne
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rs], regs[rt], imm);
}


void tocodeL(char type[], int label)     // label
{
    printf("%s%i:\n", type, label);
}

void MExpr_gen(int br);

int consttoreg()      // сейчас manst равен CONST
{
    int r = mbox == BREG ? breg : t0;
    if (abs(num) < 32768)
        tocodeI(addi, r, d0, num);
    else
        tocodeLI(li, r, num);
    return r;
}

int toreg()
{
    int r = mbox == BREG ? breg : t0;
    if (manst == AMEM)
        tocodeB(lw, r, adispl, areg);
    else if (manst == AREG)
    {
        r = areg;
        if (mbox == BREG && breg != areg)
           tocodeR(addi, r = breg, d0, areg);
    }
    else  // CONST
        r = consttoreg();
    manst = AREG;
    return areg = r;
}

int condtoreg()
{
    mbox = -BC;
    MExpr_gen(0);
    return toreg();
}

void mdsp(int displ)
{
    manst = AMEM;
    if (displ < 0)
    {
        areg = gp;
        adispl = -(32768 + 4 * (displ - 2));  // в глоб данных первые 5 слов bounds
    }
    else
    {
        areg = stp;
        adispl = 4 * (displ - 3);
    }
}

void MPrimary();

void MBin_operation(int c)
{
    int br = getreg(), leftanst, leftdispl, leftreg, leftnum, oldbreg = breg;
    mbox = BF;
    breg = br;
    MExpr_gen(br);
    leftanst = manst;
    leftdispl = adispl;
    leftreg = areg;
    leftnum = num;
    MExpr_gen(t0);
    if (leftanst == CONST && manst == CONST)
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
            case LDIV:
                num = leftnum / num;
        
                break;
                
            default:
                break;
        }
    }
    else if (leftanst == CONST && manst == AREG)
    {
        int r;
        r = leftanst;
        leftanst = manst;
        manst = r;
        r = leftdispl;
        leftdispl = adispl;
        manst = r;
        r = leftreg;
        leftreg = areg;
        areg = r;
        r = leftnum;
        leftnum = num;
        num = r;

    }
    if (leftanst == AREG && manst == CONST)
    {
        switch (c)
        {
            case LREM:
            case LSHL:
                tocodeI(sll, oldbreg, areg, num);
                break;
            case LSHR:
                tocodeI(sra, oldbreg, areg, num);
                break;
            case LAND:
                tocodeI(andi, oldbreg, areg, num);
                break;
            case LEXOR:
                tocodeI(xori, oldbreg, areg, num);
                break;
            case LOR:
                tocodeI(ori, oldbreg, areg, num);
                break;
            case LPLUS:
                tocodeI(addi, oldbreg, areg, num);
                break;
            case LMINUS:
                tocodeI(addi, oldbreg, areg, -num);
                break;
            case LMULT:
                break;
            case LDIV:
                break;
        }
    }
    if (leftanst == AREG && manst == AREG)
    {
        switch (c)
        {
            case LREM:
            case LSHL:
                tocodeR(sllv, oldbreg, leftreg, areg);
                break;
            case LSHR:
                tocodeR(srav, oldbreg, leftreg, areg);
                break;
            case LAND:
                tocodeR(and, oldbreg, leftreg, areg);
                break;
            case LEXOR:
                tocodeR(xor, oldbreg, leftreg, areg);
                break;
            case LOR:
                tocodeR(or, oldbreg, leftreg, areg);
                break;
            case LPLUS:
                tocodeR(add, oldbreg, leftreg, areg);
                break;
            case LMINUS:
                tocodeR(sub, oldbreg, leftreg, areg);
                break;
            case LMULT:
                tocodeR(mul, oldbreg, leftreg, areg);
                break;
            case LDIV:
                break;
        }
    }
}

void MExpr_gen(int br)
{
    int c = tree[tc];
    printf("exprgen tc= %i c= %i\n", tc, c);
    if (c < 9000)
        MPrimary();
    else if (c > 10000)
        MBin_operation(c -= 1000); // бинарная операция
    else                           // унарная  операция
    {
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
                int ad, ar, flag = 1;
                mdsp(tree[tc++]);
                ad = adispl;
                ar = areg;
                mbox = BF;
                breg = t0;
                MExpr_gen(t0);
                
                if (c == ASS || c == ASSV)
                {
                    if (manst == CONST)
                    {
                        if (abs(num) < 32768)
                            tocodeI(addi, t0, d0, num);
                        else
                            tocodeLI(li, t0, num);
                    }
                }
                else
                {
                    tocodeB(lw, t0, ad, ar);
                    if (manst == CONST)
                    {
                        if (abs(num) < 32768)
                        {
                            if (c == SHLASS || c == SHLASSV)
                                tocodeSLR(sll, t0, t0, num);
                            else if (c == SHRASS || c == SHRASSV)
                                tocodeSLR(sra, t0, t0, num);
                            else if (c == ANDASS || c == ANDASSV)
                                tocodeI(andi, t0, t0, num);
                            else if (c == ORASS || c == ORASSV)
                                tocodeI(ori, t0, t0, num);
                            else if (c == EXORASS|| c == EXORASSV)
                                tocodeI(xori, t0, t0, num);
                            else if (c == PLUSASS|| c == PLUSASSV)
                                tocodeI(addi, t0, t0, num);
                            else if (c == MINUSASS|| c == MINUSASSV)
                                tocodeI(addi, t0, t0, -num);
                            flag = 0;
                            if (c == MULTASS || c == MULTASSV)
                            {
                                tocodeI(addi, t1, d0, num);
                                flag = 1;
                            }
                        }
                        else
                            tocodeLI(li, t1, num);
                    }
                }
                if (flag)
                {
                    if (c == SHLASS || c == SHLASSV)
                        tocodeR(sllv, t0, t0, t1);
                    else if (c == SHRASS || c == SHRASSV)
                        tocodeSLR(srav, t0, t0, t1);
                    else if (c == ANDASS || c == ANDASSV)
                        tocodeR(and, t0, t0, t1);
                    else if (c == ORASS || c == ORASSV)
                        tocodeR(or, t0, t0, t1);
                    else if (c == EXORASS|| c == EXORASSV)
                        tocodeR(xor, t0, t0, t1);
                    else if (c == PLUSASS|| c == PLUSASSV)
                        tocodeR(addi, t0, t0, t1);
                    else if (c == MINUSASS|| c == MINUSASSV)
                        tocodeR(sub, t0, t0, t1);
                    else if (c == MULTASS|| c == MULTASSV)
                        tocodeR(mul, t0, t0, t1);
                }
                tocodeB(sw, t0, ad, ar);
            }
                break;
                
            case POSTINC:
            case INC:
            case POSTINCV:
            case INCV:
            {
                mdsp(tree[tc++]);
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, t1, t0, 1);
                tocodeB(sw, t1, adispl, areg);
                manst = AREG;
                areg = c == INC || c == INCV ? t1 : t0;
            }
                break;
            case POSTDEC:
            case DEC:
            case POSTDECV:
            case DECV:
            {
                mdsp(tree[tc++]);
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, t1, t0, -1);
                tocodeB(sw, t1, adispl, areg);
                manst = AREG;
                areg = c == INC || c == INCV ? t1 : t0;

            }
                break;
            case POSTINCAT:
            case INCAT:
            case POSTINCATV:
            case INCATV:
            {
                mbox = BF;
                breg = t0;
                MExpr_gen(t0);   // здесь точно будет manst == AMEM
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, t1, t0, 1);
                tocodeB(sw, t1, adispl, areg);
                manst = AREG;
                areg = c == INC || c == INCV ? t1 : t0;
            }
                break;
            case POSTDECAT:
            case DECAT:
            case POSTDECATV:
            case DECATV:
            {
                mbox = BF;
                breg = t0;
                MExpr_gen(t0);   // здесь точно будет manst == AMEM
                tocodeB(lw, t0, adispl, areg);
                tocodeI(addi, t1, t0, -1);
                tocodeB(sw, t1, adispl, areg);
                manst = AREG;
                areg = c == INC || c == INCV ? t1 : t0;
            }
                break;
            case LNOT:  // поразрядное отрицание:
            case LOGNOT:
            case UNMINUS:

            case LAND:
            case LOR:
            case LEXOR:
            case LOGAND:
            case LOGOR:
            case LREM:
            case LSHL:
            case LSHR:
                
            case EQEQ:
            case NOTEQ:
            case LLT:
            case LGT:
            case LLE:
            case LGE:
            case LPLUS:
            case LMINUS:
            case LMULT:
            case LDIV:
                
            default:
                break;
        }
    }
}

void mfinalop()
{
    int c;
    while ((c = tree[tc]) > 9000)
    {
        tc++;
        if (c != NOP)
        {
            if (c == ADLOGOR)
            {
                tocodeJC(bgtz, t0, "log", tree[tree[tc++]] = labnum++);
            }
            else if (c == ADLOGAND)
            {
                tocodeJC(blez, t0, "log", tree[tree[tc++]] = labnum++);
            }
            else
            {
                tocode(c);
                if (c == LOGOR || c == LOGAND)
                    tocodeL("log", tree[tc++]);
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
            }
        }
    }
}

void MDeclid_gen();

void MStmt_gen();

void MPrimary()
{
    int flagprim = 1, eltype, r, d;
    while (flagprim)
    {
    printf("MPrimary tc= %i tree[tc]= %i\n", tc, tree[tc]);
    switch (tree[tc++])
        {
            case TCondexpr:
            {
                int labelse = labnum++, labend;
                labend = labnum++;
                tocodeJEQ(beq, condtoreg(), d0, " else", labelse);
                MExpr_gen(0);
                tocodeJ(jump, "end", labend);
                tocodeL("else", labelse);
                MExpr_gen(1);
                tocodeL("end", labend);
            }
            case TIdent:
                mdsp(tree[tc++]);   // рудимент, возможно, нужно для выборки поля
                break;
            case TIdenttoaddr:
                mdsp(tree[tc++]);
                r = mbox == BREG ? breg : t0;
                manst = AREG;
                tocodeI(addi, r, areg, adispl);
                areg = r;
                break;
            case TIdenttoval:
                mdsp(tree[tc++]);
                r = mbox == BREG ? breg : t0;
                manst = AREG;
                tocodeB(lw, r, adispl, areg);
                areg = r;
                break;
                
/*            case TIdenttovald:
                tocode(LOADD);
                tocode(tree[tc++]);
                break;
 */
            case TAddrtoval:       // сейчас адрес в регистре areg
                r = mbox == BREG ? breg : t0;
                manst = AREG;
                tocodeB(lw, r, 0, areg);
                areg = r;
                break;
                
/*            case TAddrtovald:
                tocode(LATD);
                break;
 */
            case TConst:
                manst = CONST;
                num = tree[tc++];
                if (abs(num) >= 32768)
                {
                    r = mbox == BREG ? breg : t0;
                    tocodeLI(li, r, num);
                    manst = AREG;
                    areg = r;
                }
                break;
                
/*            case TConstd:
                tocode(LID);
                tocode(tree[tc++]);
                tocode(tree[tc++]);
                break;
 
            case TString:
            {
                int n = tree[tc++], res,i;
                tocode(LI);
                tocode(res = pc+4);
                tocode(B);
                pc += 2;
                for (i=0; i<n; i++)
                    tocode(tree[tc++]);
                mem[res-1] = n;
                mem[res-2] = pc;
                wasstring = 1;
            }
                break;
*/
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
                    MExpr_gen(0);
            }
                break;
*/
            case TSliceident:
                mdsp(tree[tc++]);              // параметры - смещение идента и тип элемента
                tocodeB(lw, a0, adispl, areg); // продолжение в след case
            case TSlice:                       // параметр - тип элемента
                eltype = tree[tc++];
                mbox = BREG;                   // a0 - C0
                breg = a1;                     // a1 - index
                MExpr_gen(0);
                d = szof(eltype);
                tocodeI(addi, a2, d0, d);      // a2 - d
                tocodeJ(jal, "SLICE", 1);      // t0 - адрес i-го элемента
 
                if (eltype > 0 && modetab[eltype] == MARRAY)
                    tocodeB(lw, t0, 0, t0);
                break;
            case TSelect:
                tocode(SELECT);                // SELECT field_displ
                tocode(tree[tc++]);
                break;
            case TPrint:
                tocodeI(addi, a0, d0, tree[tc++]);
                tocodeJ(jal, "PRINT", 1);
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
                    MExpr_gen(breg);
                }
            }
                break;
            case TCall2:
                tocodeJ(jal, "FUNC", identab[tree[tc++]+3]);
                break;
            case NOP:
                tc++;
                flagprim = 0;
            default:
                tc--;
        }
    }
}
/*
        mfinalop();
        
        if (tree[tc] == TCondexpr)
        {
            if (incond)
                return wasstring;
            else
            {
                tc++;
                do
                {
                    int labelse = labnum++, labend;
                    labend = labnum++;
                    tocodeJEQ(beq, condtoreg(), d0, " else", labelse);
                    MExpr_gen(0);
                    tocodeJ(jump, "end", labend);
                tocodeL("else", labelse);
                    MExpr_gen(1);
                tocodeL("end", labend);
                }
                while  (tree[tc] == TCondexpr);
            }
            
            mfinalop();
        }
        if (tree[tc] == TExprend)
        {
            tc++;
            flagprim = 0;
        }
    }
    return wasstring;
}
*/
void MStmt_gen();

void mcompstmt_gen()
{
    while (tree[tc] != TEnd)
    {
        switch (tree[tc])
        {
            case TDeclarr:
            {
                int i, N;
                tc++;
                N = tree[tc++];
                mbox = BREG;
                breg = t0;
                bdispl = -32768;
                for (i=0; i<N; i++)
                {
                    MExpr_gen(0);
                    tocodeB(sw, t0, bdispl, gp);
                    bdispl += 4;
                }

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
    int r, oldmbox = mbox, oldbdispl = bdispl, oldbreg = breg;
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
            int elseref = tree[tc++], labelse = labnum++, labend;
            labend = labnum++;
            tocodeJEQ(mbox<0 ? beq : bne, condtoreg(), d0, " else", labelse);
            MStmt_gen();
            if (elseref)
            {
                tocodeJ(jump, "end", labend);
            tocodeL("else", labelse);
                MStmt_gen();
            }
            else
                tocodeL("else", labelse);
                tocodeL("end", labend);
        }
            break;
        case TWhile:
        {
            int oldbreak = adbreak, oldcont = adcont;
            adcont  = labnum++;
            adbreak = labnum++;
            tc++;
        tocodeL("cont", adcont);
            tocodeJEQ(mbox<0 ? beq : bne, condtoreg(), d0, "endloop", adbreak);
            MStmt_gen();
            tocodeJ(jump, "cont", adcont);
        tocodeL("end", adbreak);
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TDo:
        {
            int oldbreak = adbreak, oldcont = adcont, labbeg = labnum++;
            adcont  = labnum++;
            adbreak = labnum++;
        tocodeL("loop", labbeg);
            MStmt_gen();
        tocodeL("cont", adcont);
            tocodeJEQ(mbox<0 ? bne : beq, condtoreg(), d0, "loop", labbeg);
        tocodeL("end", adbreak);
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TFor:
        {
            int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++], stmtref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, incrtc, endtc;
            int r = getreg();
            if (fromref)
            {
                mbox = BREG;
                breg = r;
                MExpr_gen(0);         // init
            }
            adbreak = labnum++;
            adcont  = labnum++;
            tocodeL("cont", adcont);
            if (condref)
            {
                mbox = BREG;
                breg = t0;
                MExpr_gen(0);         // cond
            }
            incrtc = tc;
            tc = stmtref;
            MStmt_gen();             //  ???? был 0
            if (incrref)
            {
                endtc = tc;
                tc = incrtc;
                MExpr_gen(0);         // incr
                tc = endtc;
            }
            tocodeJ(jump, "cont", adcont);
        tocodeL("end", adbreak);
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TGoto:
            tocodeJ(jump, "label", abs(tree[tc++]));
            break;
        case TLabel:
        tocodeL("label", tree[tc++]);
            break;
        case TSwitch:
        {
            int oldbreak = adbreak, oldcase = adcase, oldreg = switchreg;
            adbreak = labnum++;
            adcase = labnum++;
            mbox = BREG;
            switchreg = breg = getreg();
            MExpr_gen(breg);
            MStmt_gen();
            freereg(breg);
            adcase = oldcase;
            adbreak = oldbreak;
            switchreg = oldreg;
        }
            break;
        case TCase:
        {
            int ocase =adcase;
            MExpr_gen(0);
            tocodeJEQ(bne, condtoreg(), switchreg, "case", adcase = labnum++);
    tocodeL("case", ocase);
            MStmt_gen();
        }
            break;
        case TDefault:
        {
    tocodeL("case", adcase);
            MStmt_gen();
        }
            break;
            
        case TBreak:
            tocodeJ(jump, "end", adbreak);
            break;
        case TContinue:
            tocodeJ(jump, "cont", adcont);
            break;
        case TReturnval:
            r = tree[tc++];
            mbox = BREG;
            breg = v0;
            MExpr_gen(breg);
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
            MExpr_gen(0);
            MExpr_gen(0);
            tocode(SETMOTORC);
            break;
 */
        default:
        {
            tc--;
            MExpr_gen(t0);
        }
            break;
    }
    mbox = oldmbox;
    bdispl = oldbdispl;
    breg = oldbreg;
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
                MExpr_gen(t0);
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
        tocodeI(addi, s8, s8, -4);
        tocodeI(addi, t1, s8, -4);      // C0, то есть адрес нулевого элемента
        tocodeB(lw, t0, -32768, gp);    // NN
        tocodeB(sw, t0, 0, s8);
        tocodeSLR(sll, t0, t0, 2);      // NN*4
        tocodeR(sub, s8, s8, t0);       // захват памяти под NN элементов
        mdsp(olddispl);
        tocodeB(sw, t1, adispl, areg);
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
    tocodeI(addi, stp, stp, -4);
    tocodeB(sw, ra, 0, stp);
    tocodeJ(jal,"FUNC",identab[wasmain+3]);
    tocodeB(lw, ra, 0, stp);
    tocodeI(addi, stp, stp, 4);
    tocodeJR(jr, ra);

tocodeL("SLICE", 1);               // a0 - C0,  a1 - index, a2 - d
    tocodeB(lw, t0, -4, a0);       // t0 - N
    tocodeJEQimm(beq, a1, d0, 5);
    tocodeJCimm(blez, a1, 2);
    tocodeR(slt, t1, a1, t0);      // t1 = index < N ? 1 : 0
    tocodeJCimm(blez, t1, 2);
    tocodeR(addi, a0, d0, t0);
    tocodeJ(jal, "FUNCERR", 0);
    tocodeR(mul, t0, a1, a2);      // t0 = index * d
    tocodeR(add, t0, t0, a0);      // t0 = s0 + t0
    tocodeJR(jr, ra);              // t0 - адрес i-го элемента
    printf("\n");


    while (tc < treesize)
    {
        switch (tree[tc++])
        {
            case TEnd:
                break;
            case TFuncdef:
            {
                int identref =  tree[tc++];
            tocodeL("FUNC", identab[identref+3]);
                maxdispl = (tree[tc++] - 3) * 4;
                tocodeI(addi, stp, stp, -maxdispl - 56);
                tocodeR(addi, s8, d0, stp);
                tocodeB(sw, ra, maxdispl + 36, stp);
                printf("\n");
                tc++;             // TBegin
                mcompstmt_gen();
                printf("\n");

            }
                break;
            case TDeclarr:
            {
                int i, N = tree[tc++];
                mbox = BREG;
                breg = t0;
                bdispl = -32768;
                for (i=0; i<N; i++)
                {
                    MExpr_gen(t0);
                    tocodeB(sw, t0, bdispl, gp);
                    bdispl += 4;
                }
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
}


