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
//    printf("tc= %i tree[tc]= %i\n", tc, tree[tc]);
    return mtree[mtc++] = tree[tc++];
}

int munop(int t)
{
/*    return t == LNOT || t == LOGNOT || t == LAND || t == LMULT ||
    t == WIDEN || t == WIDEN1 || t == TAddrtoval || t == TAddrtovald ? 1
    : (t >= POSTINC && t <= DEC) || (t >= POSTINCV && t <= DECV) || t == TPrint ? 2
    : (t >= POSTINCAT && t <= DECAT) || (t >= POSTINCATV && t <= DECATV) || t == UNMINUS ? 1
    : (t >= POSTINCR && t <= DECR) || (t >= POSTINCRV && t <= DECRV) ? 2
    : (t >= POSTINCATR && t <= DECATR) || (t >= POSTINCATRV && t <= DECATRV) || t == UNMINUSR ? 1
    : 0;
}
*/
    switch (t)
    {
        case LNOT:
        case LOGNOT:
        case LAND:
        case WIDEN:
        case WIDEN1:
        case _DOUBLE:
        case TAddrtoval:
        case TAddrtovald:
        case POSTINCAT:
        case POSTDECAT:
        case INCAT:
        case DECAT:
        case POSTINCATV:
        case POSTDECATV:
        case INCATV:
        case DECATV:
        case UNMINUS:
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
            
        case POSTINC:
        case POSTDEC:
        case INC:
        case DEC:
        case POSTINCV:
        case POSTDECV:
        case INCV:
        case DECV:
        case TPrint:
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
        case LOGAND:
        case LOGOR:
            
        case EQEQ:
        case NOTEQ:
        case LLT:
        case LGT:
        case LLE:
        case LGE:
        case LPLUS:
        case LMINUS:
        case LMULT:    // у этих 10 команд есть такие же с плавающей зпт
        case LDIV:
            
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
            int fromref, condref, incrref;
            mcopy();
            fromref = mcopy();
            condref = mcopy();
            incrref = mcopy();
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
        case TPrintf:

        default:
            mexpr();
    }
}

void permute(int n1)
{
    int i, oldopnd = tree[tc+1];
//    printf("permute n1= %i tc= %i op= %i opnd=%i\n", n1, tc, op, opnd);
    for (i=tc+opnd-1; i>n1+opnd-1; i--)
        mtree[i] = mtree[i-opnd];
    mtree[n1] = op;
    if (opnd == 2)
        mtree[n1+1] = oldopnd;
    tc  += opnd;
    mtc += opnd;
}

int munarexpr()
{
    int i, n1, flag = 0;
    t = tree[tc];
    n1 = tc;
//  printf("munarexpr tc= %i t= %i\n", tc, t);
//    if (t < -343 || t > -300)
//        printf("\nсбой tree tc= %i tree[tc]= %i\n", tc, tree[tc]);
    if (t == TString)
    {
        mcopy();
        int nstr = mcopy();
        flag = n1;
        for (i=0; i<nstr; i++)
            mcopy();
    }
    else if (t == TSliceident || t == TSelect)
    {
        mcopy();
        mcopy();
        flag = n1;
        if (t == TSliceident)
        {
            mcopy();
            mcopy();
            mexpr();
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
        flag = n1;
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
        flag = n1;
        for (i=0; i<n; i++)
            mexpr();
    }
    else if (t == TStructinit)
    {
        mcopy();
        flag = n1;
        do
            mexpr();
        while (tree[tc] != TEndinit);
        mcopy();
    }
    else if (t == TIdent || t == TIdenttoval ||  t == TAddrtoval ||
             t == TIdenttovald || t == TIdenttoaddr || t == TConst)
    {
        mcopy();
        flag = n1;
        mcopy();
    }
    else if (t == TConstd || t == TSliceident)
    {
        mcopy();
        flag = n1;
        mcopy();
        mcopy();
    }
//    printf("munarexpr tc= %i tree[tc]= %i opnd= %i\n", tc, tree[tc], munop(tree[tc]));
    while ((op = tree[tc], opnd = munop(op)))
        permute(n1);
    if (tree[tc] == TExprend)
        mcopy();
    return flag;
}

void mexpr()
{
    stack[++sp] = munarexpr();

    while ((stack[++sp] = munarexpr()))
        ;
    --sp;
    while ((op = tree[tc], opnd = mbinop(op)))
    {
//    printf("mexpr1 tc= %i op= %i opnd= %i sp= %i stack[sp-1]= %i\n", tc, op, opnd, sp, stack[sp-1]);
        permute(stack[--sp]);
    }
    if (tree[tc] == TExprend)
        mcopy();
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
        mcopy();
        do
            mexpr();
        while (tree[tc] != TEndinit);
        mcopy();
    }
    else
        mexpr();
}
void mblock()
{
    int i, n, all;
    do
    {
        switch (t = tree[tc])
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
}

void mipsopt()
{
    int i;
    sp = -1;
    tc = 0;
    mtc = 0;
    mblock();
    for (i=0; i<mtc; i++)
        tree[i] = mtree[i];
    output = fopen("mtree.txt", "wt");
    tablesandtree();
    fclose(output);
    exit(0);
}
