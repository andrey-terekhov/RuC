//
//  mipsopt.c
//
//  Created by Andrey Terekhov on 21/11/18.
//  Copyright (c) 2018 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"
#include <stdlib.h>
extern void tablesandtree();

int t, op, opnd;

int mcopy()
{
 //   printf("tc= %i tree[tc]= %i\n", tc, tree[tc]);
    return mtree[mtc++] = tree[tc++];
}

int munop(int t)
{
    switch (t)
    {
        case LNOT:
        case LOGNOT:
        case POSTINCAT:
        case POSTDECAT:
        case INCAT:
        case DECAT:
        case POSTINCATV:
        case POSTDECATV:
        case INCATV:
        case DECATV:
        case UNMINUS:
         
        case TAddrtoval:
        case TAddrtovald:
        case WIDEN:
        case WIDEN1:

        case POSTINCATR:
        case POSTDECATR:
        case INCATR:
        case DECATR:
        case POSTINCATRV:
        case POSTDECATRV:
        case INCATRV:
        case DECATRV:
        case UNMINUSR:
            
            return 1;
            
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
            
        case ASSR:
        case PLUSASSR:
        case MINUSASSR:
        case MULTASSR:
        case DIVASSR:
            
        case ASSRV:
        case PLUSASSRV:
        case MINUSASSRV:
        case MULTASSRV:
        case DIVASSRV:

        case POSTINC:
        case POSTDEC:
        case INC:
        case DEC:
        case POSTINCV:
        case POSTDECV:
        case INCV:
        case DECV:
        case TPrint:
        case TPrintf:
        case POSTINCR:
        case POSTDECR:
        case INCR:
        case DECR:
        case POSTINCRV:
        case POSTDECRV:
        case INCRV:
        case DECRV:
            
            return 2;
            
        default:
            
            return 0;
    }
}

int mbinop(int t)
{
    switch (t)
    {
        case REMASSAT:
        case SHLASSAT:
        case SHRASSAT:
        case ANDASSAT:
        case EXORASSAT:
        case ORASSAT:
            
        case ASSAT:
        case PLUSASSAT:
        case MINUSASSAT:
        case MULTASSAT:
        case DIVASSAT:
            
        case REMASSATV:
        case SHLASSATV:
        case SHRASSATV:
        case ANDASSATV:
        case EXORASSATV:
        case ORASSATV:
            
        case ASSATV:
        case PLUSASSATV:
        case MINUSASSATV:
        case MULTASSATV:
        case DIVASSATV:

        case LREM:
        case LSHL:
        case LSHR:
        case LAND:
        case LEXOR:
        case LOR:
            
        case LPLUS:
        case LMINUS:
        case LMULT:
        case LDIV:
        case EQEQ:
        case NOTEQ:
        case LLT:
        case LGT:
        case LLE:
        case LGE:
            
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
            
        case ASSATR:
        case PLUSASSATR:
        case MINUSASSATR:
        case MULTASSATR:
        case DIVASSATR:
            
        case ASSATRV:
        case PLUSASSATRV:
        case MINUSASSATRV:
        case MULTASSATRV:
        case DIVASSATRV:
            op += 1000;
            return 1;
            
        case LOGAND:
        case LOGOR:
            op += 1000;
            return 2;
            
        default:
            return 0;
    }
}

void mexpr();

void mstatement();

void mblock();

void mstatement()
{
    t = tree[tc];
//  printf("stmt tc-1= %i tree[tc-1]= %i t= %i\n", tc-1, tree[tc-1], t);
    switch (t)
    {
        case TBegin:
            mcopy();
            mblock();
            break;
        case TIf:
        {
            int elseref;
            mcopy();
            elseref = mcopy();
            mexpr();
            mstatement();
            if (elseref)
                mstatement();
            break;
        }
        case TWhile:
        case TSwitch:
        case TCase:
            mcopy();
            mexpr();
            mstatement();
            break;
        case TDo:
            mcopy();
            mstatement();
            mexpr();
            break;
        case TFor:
        {
            int fromref, condref, incrref, statemref;
            mcopy();
            fromref = mcopy();
            condref = mcopy();
            incrref = mcopy();
            statemref = mcopy();
            if (fromref)
                mexpr();
            if (condref)
                mexpr();
            if (incrref)
                mexpr();
            mstatement();
            break;
        }
        case TLabel:
            mcopy();
            mcopy();
            mstatement();
            break;
        case TBreak:
        case TContinue:
        case TReturnvoid:
            mcopy();
            break;
        case TReturnval:
            mcopy();
            mcopy();
            mexpr();
            break;
        case TGoto:
        case TPrintid:
        case TGetid:
            mcopy();
            mcopy();
            break;

        default:
            mexpr();
    }
}

void permute(int n1)
{
    int i, oldopnd = tree[tc+1];
//    printf("permute sp= %i n1= %i tc= %i op= %i opnd=%i\n", sp, n1, tc, op, opnd);
    for (i=tc+opnd-1; i>n1+opnd-1; i--)
        mtree[i] = mtree[i-opnd];
    mtree[n1] = op;
    if (opnd == 2)
        mtree[n1+1] = oldopnd;
    tc  += opnd;
    mtc += opnd;
}

int operand()
{
    int i, n1, flag = 1;
    t = tree[tc];
    if (tree[tc] == NOP)
        mcopy();
    if (tree[tc] == TIdent || tree[tc] == ADLOGOR || tree[tc] == ADLOGAND)
    {
        mcopy();
        mcopy();
    }
    n1 = tc;
    t = tree[tc];
    if (t == TString)
    {
        mcopy();
        int nstr = mcopy();
        for (i=0; i<nstr; i++)
            mcopy();
    }
    else if (t == TSliceident || t == TSelect)
    {
        mcopy();
        mcopy();             // displ
        if (t == TSliceident)
        {
            mcopy();         // type
            mexpr();         // index
        }
        while (tree[tc] == TSlice || t == TSelect)
        {
            mcopy();
            if (t == TSlice)
            {
                mcopy();
                mexpr();
            }
        }
    }
    else if (t == TCall1)
    {
        int npar;
        mcopy();
        npar = mcopy();
        for (i=0; i<npar; i++)
            mexpr(0);
        mcopy();          // CALL2
        mcopy();
    }
    else if (t == TBeginit)
    {
        int n;
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TStructinit)
    {
        int i, n;
        mcopy();
        n =  mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TIdenttoval || t == TIdenttovald || t == TIdenttoaddr || t == TConst)
    {
        mcopy();
        mcopy();
    }
    else if (t == TConstd)
    {
        mcopy();
        mcopy();
        mcopy();
    }
    else
        flag = 0;

    return flag ? n1 : 0;
}

void mexpr()
{
    int wasopnd = 0;
    while (1)
    {
        while ( (stack[++sp] = operand() ) )
        {
            wasopnd = 1;
            printf("sp= %i stack[sp]= %i\n", sp, stack[sp]);
        }
        sp--;
        if (tree[tc] == NOP)
            mcopy();
        op = tree[tc];
        if (op == TExprend)
        {
            mcopy();
            if (wasopnd)
                --sp;
            break;
        }
        else if ( (opnd = munop(op)) )
                if (wasopnd)
                    if (op == WIDEN1)
                        permute(stack[sp-1]);
                    else if (op == TPrintf)
                        permute(stack[sp -= tree[tc+1]]);
                    else
                        permute(stack[sp]);
                else
                    mcopy(), mcopy();
        else if ( (opnd = mbinop(op)) )
                permute(stack[--sp]);
        
        else if ((op = tree[tc]) == TCondexpr)
        {
//            printf("Cond tc= %i sp= %i\n", tc, sp);
            opnd = 1;
            permute(stack[sp]);
            mexpr();
            mexpr();
        }
        else
            break;
    }
}
    
void init()
{
    int i, n;
    t = tree[tc];
    if (t == TBeginit)
    {
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TStructinit)
    {
        int i, n;
        mcopy();
        n = mcopy();
        for (i=0; i<n; i++)
            mexpr();
    }
    else
        mexpr();
}
void mblock()
{
    int i, n, all;
    do
    {
        switch (tree[tc])
        {
            case TFuncdef:
            {
                mcopy();     // TFucdef
                mcopy();
                mcopy();
                mcopy();     // TBegin
                mblock();
                break;
            }
                
            case TDeclarr:
            {
                mcopy();     // TDeclarr
                n= mcopy();
                for (i=0; i<n; i++)
                    mexpr();
                break;
            }
            case TDeclid:
            {
                mcopy();    // TDeclid
                mcopy();    // displ
                mcopy();    // type_elem
                mcopy();    // N
                all = mcopy();
                mcopy();    // iniproc
                mcopy();    // usual
                mcopy();    // instruct
                if (all)
                    init();
                break;
            }
            case NOP:
                mcopy();     // NOP
                break;
            case TStructbeg:
            case TStructend:
                mcopy();
                break;
                
            default:
                mstatement();
        }
    }
    while (tree[tc] != TEnd);
    mcopy();
}

void mipsopt()
{
    sp = -1;
    tc = 0;
    mtc = 0;
    mblock();
}
