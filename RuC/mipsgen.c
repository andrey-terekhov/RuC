//
//  mip16gen.c
//
//  Created by Andrey Terekhov on 16/12/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//
#include <stdlib.h>
#include "global_vars.h"

// запросы
#define BMEM   1     // in bdispl and breg address
#define BREG   2     // in breg destination
#define BF     3     // free
#define BC     4     // for conditions  BC > 0 BTRUE BC < 0 BFALSE

// ответы
#define AMEM  1      // in adispl and areg address
#define AREG  2      // in areg register number
#define CONST 3      // in num int const

int mbox,  bdispl, breg;
int manst, adispl, areg;
int labnum = 1;

// унарные операции LNOT, LOGNOT, -, ++, --, TIdenttoval(*), TIdenttoaddr(&)
// LNOT nor rd, rs, d0    LOGNOT slti rt, rs, 1   - sub rd, d0, rt
// *  lw rt, imm(rs) или сразу 0(areg)   & addi rt, areg, adispl или 0(areg)
// ++ lw rt, imm(rs) addi rt, rt, 1 sw rt, imm(rs)
extern void error(int err);
extern int szof(int);

int d0 = 0, stp = 29, t0 = 8;

char *mcodes[] =
{/* 0 */ "bltz", "", "j", "jal", "beq", "bne", "blez", "bgtz", "addi", "addiu",
/* 10 */ "slti", "sltiu", "andi", "ori", "xori", "lui", "", "", "", "",
/* 20 */ "", "", "", "", "", "", "", "", "mul", "",
/* 30 */ "", "", "", "", "","lw", "", "", "", "",
/* 40 */ "", "", "", "sw" "", "", "", "", "", "",
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
/* 30 */ "$fp", "$ra"
};

int tempregs[] =
{
    8, 9, 10, 11, 12, 13, 14, 15, 24, 25
};

#define too_many_sregs    1

void merror(int type)
{
    switch (type)
    {
        case too_many_sregs:
            printf("кончились регистры s0-s7\n");
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

void tocodeB(int op, int rt, int imm, int rs)  // lw rt, imm(rs)  sw rt, imm(rs)
{
    printf("\t%s %s, %i(%s)\n", mcodes[op], regs[rt], imm, regs[rs]);
}

void tocodeLUI(int op, int rt, int imm)  // lui rt, imm  sw rt
{
    printf("\t%s %s, %i\n", mcodes[op], regs[rt], imm);
}

void tocodeSLR(int op, int rd, int rt, int shamt)  // sll rd, rt, shamt    srl sra
{
    printf("\t%s %s, %s, %i\n", mcodes[op], regs[rd], regs[rt], shamt);
}

void tocodeJ(int op, char type[], int label)  // j label
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

void tocodeJEQ(int op, int rs, int rt, char type[], int label)  // beq rs, rt, label   bne
{
    printf("\t%s %s, %s, %s%i\n", mcodes[op], regs[rs], regs[rt], type, label);
}

void tocodeL(char type[], int label)
{
    printf("%s%i:\n", type, label);
}

int MExpr_gen(int t);

int toreg()
{
    int r;
    mbox = -BC;
    MExpr_gen(0);
    
    if (manst == AMEM)
        tocodeB(lw, r = t0, adispl, areg);
    else if (manst == AREG)
        r = areg;
    else  // CONST
        tocodeI(addi, r = t0, d0, num);
    return r;
}
/*
void finalop()
{
    int c;
    while ((c = tree[tc]) > 9000)
    {
        tc++;
        if (c != NOP)
        {
            if (c == ADLOGOR)
            {
                tocode(_DOUBLE);
                tocode(BNE0);
                tree[tree[tc++]] = pc++;
            }
            else if (c == ADLOGAND)
            {
                tocode(_DOUBLE);
                tocode(BE0);
                tree[tree[tc++]] = pc++;
            }
            else
            {
                tocode(c);
                if (c == LOGOR || c == LOGAND)
                    mem[tree[tc++]] = pc;
                else if (c == COPY00 || c == COPYST)
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
            }
        }
    }
    
}
*/

void mfinalop()
{
    int c;
    while ((c = tree[tc]) > 9000)
    {
        tc++;
    }
}
void MDeclid_gen();

int MExpr_gen(int incond)
{
    int flagprim = 1, eltype, wasstring = 0;
    while (flagprim)
    {
        switch (tree[tc++])
        {
            case TIdent:
                anstdispl = tree[tc++];
                break;
            case TIdenttoaddr:
                tocode(LA);
                tocode(anstdispl = tree[tc++]);
                break;
            case TIdenttoval:
                tocode(LOAD);
                tocode(tree[tc++]);
                break;
            case TIdenttovald:
                tocode(LOADD);
                tocode(tree[tc++]);
                break;
            case TAddrtoval:
                tocode(LAT);
                break;
            case TAddrtovald:
                tocode(LATD);
                break;
            case TConst:
                tocode(LI);
                tocode(tree[tc++]);
                break;
            case TConstd:
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
            case TDeclid:
                MDeclid_gen();
                break;
            case TBeginit:
            {
                int n = tree[tc++], i;
                tocode(BEGINIT);
                tocode(n);
                for (i=0; i<n; i++)
                    MExpr_gen(0);
            }
                break;
            case TSliceident:
                tocode(LOAD);        // параметры - смещение идента и тип элемента
                tocode(tree[tc++]);  // продолжение в след case
            case TSlice:             // параметр - тип элемента
                eltype = tree[tc++];
                MExpr_gen(0);
                tocode(SLICE);
                tocode(szof(eltype));
                if (eltype > 0 && modetab[eltype] == MARRAY)
                    tocode(LAT);
                break;
            case TSelect:
                tocode(SELECT);                // SELECT field_displ
                tocode(tree[tc++]);
                break;
            case TPrint:
                tocode(PRINT);
                tocode(tree[tc++]);  // type
                break;
            case TCall1:
            {
                int i, n = tree[tc++];
                tocode(CALL1);
                for (i=0; i < n; i++)
                    MExpr_gen(0);
            }
                break;
            case TCall2:
                tocode(CALL2);
                tocode(identab[tree[tc++]+3]);
                break;
                
            default:
                tc--;
        }
        
        mfinalop();
        
        if (tree[tc] == TCondexpr)
        {
            if (incond)
                return wasstring;
            else
            {
                int adelse, ad = 0;
                //int thenref = tree[++tc];
                //int elseref = tree[++tc];
                do
                {
                    tc += 3;
                    tocode(BE0);
                    adelse = pc++;
                    MExpr_gen(0);              // then
                    tocode(B);
                    mem[pc] = ad;
                    ad = pc;
                    mem[adelse] = ++pc;
                    MExpr_gen(1);              // else или cond
                }
                while  (tree[tc] == TCondexpr);
                while (ad)
                {
                    int r = mem[ad];
                    mem[ad] = pc;
                    ad = r;
                }
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

void mcompstmt_gen();

void MStmt_gen()
{
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
            tocodeJEQ(mbox<0 ? beq : bne, toreg(), d0, " else", labelse);
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
            tocodeJEQ(mbox<0 ? beq : bne, toreg(), d0, "endloop", adbreak);
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
            tocodeJEQ(mbox<0 ? bne : beq, toreg(), d0, "loop", labbeg);
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
            int oldbreak = adbreak, oldcase = adcase;
            adbreak = labnum++;
            adcase = labnum++;
            mbox = BREG;
            breg = getreg();
            MExpr_gen(0);
            MStmt_gen();
            freereg(breg);
            adcase = oldcase;
            adbreak = oldbreak;
        }
            break;
        case TCase:
        {
            int ocase =adcase;
            tocodeJEQ(bne, toreg(), breg, "case", adcase = labnum++);
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
        case TReturnvoid:
        {
            tocode(RETURNVOID);
        }
            break;
        case TReturnval:
        {
            int d = tree[tc++];
            MExpr_gen(0);
            tocode(RETURNVAL);
            tocode(d);
        }
            break;
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
        default:
        {
            tc--;
            Expr_gen(0);
        }
            break;
    }
}

void MDeclid_gen()
{
    int olddispl = tree[tc++], telem = tree[tc++], N = tree[tc++], element_len,
    all = tree[tc++], iniproc = tree[tc++], usual = tree[tc++], instruct = tree[tc++];
    // all - общее кол-во слов в структуре
    //  для массивов есть еще usual // == 0 с пустыми границами,
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
//                mbox = MASSN;
//                boxdispl = (displ - 3) * 4;
                MExpr_gen(0);
             /*   tocode(telem == LFLOAT ? ASSRV : ASSV);
                tocode(olddispl);  */
            }
        }
    }
    else                                // Обработка массива int a[N1]...[NN] =
    {/*
        tocode(DEFARR);                 // DEFARR N, d, displ, iniproc, usual     N1...NN уже лежат на стеке
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

void mcompstmt_gen()
{
    while (tree[tc] != TEnd)
    {
        switch (tree[tc])
        {
          /*  case TDeclarr:
            {
                int i, N;
                tc++;
                N = tree[tc++];
                for (i=0; i<N; i++)
                    MExpr_gen(0);
                break;
            }
        */
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
void mipsgen()
{
    int treesize = tc;
    tc = 0;
    if (wasmain == 0)
        error(no_main_in_program);
    tocodeJ(jal,"FUNC",identab[wasmain+3]);

    while (tc < treesize)
    {
        switch (tree[tc++])
        {
            case TEnd:
                break;
            case TFuncdef:
            {
                int identref = tree[tc++], maxdispl = tree[tc++];
                int fn = identab[identref+3];
                functions[fn]= pc;
                tocodeI(addi, stp, stp, -(maxdispl+8)*4);
                tc++;             // TBegin
                compstmt_gen();
            }
                break;
            /*
            case TDeclarr:
            {
                int i, N = tree[tc++];
                for (i=0; i<N; i++)
                    Expr_gen(0);
                break;
            }
            */
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
                printf("что-то не то\n");
                printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", tc, tree[tc-2], tree[tc-1]);
            }
        }
    }
}


