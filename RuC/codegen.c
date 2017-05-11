//
//  codegen.c
//
//  Created by Andrey Terekhov on 3/10/15.
//  Copyright (c) 2015 Andrey Terekhov. All rights reserved.
//
#include "global_vars.h"

extern void error(int err);
extern int szof(int);

int curth = 0;

void tocode(int c)
{
//    printf("tocode tc=%i pc %i) %i\n", tc, pc, c);
    mem[pc++] = c;
}

void adbreakend()
{
    while (adbreak)
    {
        int r = mem[adbreak];
        mem[adbreak] = pc;
        adbreak = r;
    }
}

void adcontbeg(int ad)
{
    while (adcont != ad)
    {
        int r = mem[adcont];
        mem[adcont] = ad;
        adcont = r;
    }
}

void adcontend()
{
    while (adcont != 0)
    {
        int r = mem[adcont];
        mem[adcont] = pc;
        adcont = r;
    }
}

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
                else if (c == COPY00)
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
                else if (c == COPY10)
                {
                    tocode(tree[tc++]);   // d2
                    tocode(tree[tc++]);   // длина
                }
                else if (c == COPY11)
                    tocode(-tree[tc++]);  // длина
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
                else if (c == COPY1STASS)
                    tocode(tree[tc++]);   // длина

                else if((c >= REMASS && c <= DIVASS)    || (c >= REMASSV && c <= DIVASSV) ||
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

int Expr_gen(int incond)
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
            case TFunidtoval:
                anstdispl = tree[tc++];
                if (anstdispl > 0)
                {
                    tocode(LI);
                    tocode(anstdispl);
                }
                else
                {
                    tocode(LOAD);
                    tocode(-anstdispl);
                }
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
                int n = -1, res;
                tocode(LI);
                tocode(res = pc+4);
                tocode(B);
                pc += 2;
                do
                    n++, tocode(tree[tc]);
                while (tree[tc++]);
                
                mem[res-1] = n;
                mem[res-2] = pc;
                wasstring = 1;
            }
                break;
            case TSliceident:
                tocode(LOAD);        // параметры - смещение идента и тип элемента
                tocode(tree[tc++]);  // продолжение в след case
            case TSlice:             // параметр - тип элемента
                eltype = tree[tc++];
				Expr_gen(0);
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
                    Expr_gen(0);
            }
                break;
            case TCall2:
                tocode(CALL2);
                tocode(identab[tree[tc++]+3]);
                break;

            default:
                tc--;
        }
        
        finalop();
        
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
                    Expr_gen(0);              // then
                    tocode(B);
                    mem[pc] = ad;
                    ad = pc;
                    mem[adelse] = ++pc;
                    Expr_gen(1);              // else или cond
                }
                while  (tree[tc] == TCondexpr);
                while (ad)
                {
                    int r = mem[ad];
                    mem[ad] = pc;
                    ad = r;
                }
            }
            
            finalop();
        }
        if (tree[tc] == TExprend)
        {
             tc++;
             flagprim = 0;
        }
    }
    return wasstring;
}

void compstmt_gen();

void Stmt_gen()
{
    switch (tree[tc++])
    {
        case NOP:
            break;
            
        case TCREATE:
            tocode(CREATEC);
            tocode(++curth);
            break;
            
        case TEXIT:
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
            
        case TBegin:
            compstmt_gen();
            break;
            
        case TIf:
        {
            int thenref = tree[tc++], elseref = tree[tc++], ad;
            Expr_gen(0);
            tocode(BE0);
            ad = pc++;
            Stmt_gen();
            if (elseref)
            {
                mem[ad] = pc + 2;
                tocode(B);
                ad = pc++;
                Stmt_gen();
            }
            mem[ad] = pc;
        }
            break;
        case TWhile:
        {
            //int doref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, ad = pc;
            tc++;
            adcont = ad;
            Expr_gen(0);
            tocode(BE0);
            mem[pc] = 0;
            adbreak = pc++;
            Stmt_gen();
            adcontbeg(ad);
            tocode(B);
            tocode(ad);
            adbreakend();
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TDo:
        {
            int condref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, ad = pc;
            adcont = adbreak = 0;
            Stmt_gen();
            adcontend();
            Expr_gen(0);
            tocode(BNE0);
            tocode(ad);
            adbreakend();
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TFor:
        {
            int fromref = tree[tc++], condref = tree[tc++], incrref = tree[tc++], stmtref = tree[tc++];
            int oldbreak = adbreak, oldcont = adcont, ad = pc, incrtc, endtc;
            if (fromref)
            {
                Expr_gen(0);         // init
            }
            adbreak = 0;
            adcont = ad = pc;
            if (condref)
            {
                Expr_gen(0);         // cond
                tocode(BE0);
                mem[pc] = 0;
                adbreak = pc++;
            }
            incrtc = tc;
            tc = stmtref;
            Stmt_gen();             //  ???? был 0
            if (incrref)
            {
                endtc = tc;
                tc = incrtc;
                Expr_gen(0);         // incr
                tc = endtc;
            }
            adcontbeg(ad);
            tocode(B);
            tocode(ad);
            adbreakend();
            adbreak = oldbreak;
            adcont = oldcont;
        }
            break;
        case TGoto:
        {
            int id1 = tree[tc++], a;
            int id = id1 > 0 ? id1 : -id1;
            tocode(B);
            if ( (a = identab[id+3]) > 0)           // метка уже описана
                tocode(a);
             else                                   // метка еще не описана
             {
                identab[id+3] = -pc;
                tocode(id1 < 0 ? 0 : a);            // первый раз встретился переход на еще не описанную метку или нет
             }
        }
            break;
        case TLabel:
        {
            int id = tree[tc++], a;
            if ((a = identab[id+3]) < 0)            // были переходы на метку
                while (a)                           // проставить ссылку на метку во всех ранних переходах
                {
                    int r = mem[-a];
                    mem[-a] = pc;
                    a = r;
                }
            identab[id+3] = pc;
        }
            break;
        case TSwitch:
        {
            int oldbreak = adbreak, oldcase = adcase;
            adbreak = 0;
            adcase = 0;
            Expr_gen(0);
            Stmt_gen();
            if (adcase > 0)
                mem[adcase] = pc;
            adcase = oldcase;
            adbreakend();
            adbreak = oldbreak;
        }
            break;
        case TCase:
        {
            if (adcase)
                mem[adcase] = pc;
            tocode(_DOUBLE);
            Expr_gen(0);
            tocode(EQEQ);
            tocode(BE0);
            adcase = pc++;
            Stmt_gen();
        }
            break;
        case TDefault:
        {
            if (adcase)
                mem[adcase] = pc;
            adcase = 0;
            Stmt_gen();
        }
            break;

        case TBreak:
        {
            tocode(B);
            mem[pc] = adbreak;
            adbreak = pc++;
        }
            break;
        case TContinue:
        {
            tocode(B);
            mem[pc] = adcont;
            adcont = pc++;
        }
            break;
        case TReturnvoid:
        {
            tocode(RETURNVOID);
        }
            break;
        case TReturnval:
        {
            int d = tree[tc++];
            Expr_gen(0);
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
        case TGetid:
        {
            tocode(GETID);
            tocode(tree[tc++]);  // ссылка в identtab
        }
            break;
        case SETMOTOR:
            Expr_gen(0);
            Expr_gen(0);
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

void Declid_gen()
{
	int olddispl = tree[tc++], telem = tree[tc++], N = tree[tc++],
    element_len, all = tree[tc++], iniproc = tree[tc++]; // all - общее кол-во слов и в структуре, и в массиве

    element_len = szof(telem);
    if (N == 0)                    // обычная переменная int a; или struct point p;
	{
        if (iniproc)
        {
            tocode(STRUCTWITHARR);
            tocode(olddispl);
            tocode(iniprocs[iniproc]);
        }
		if (all)                   // int a = или struct{} a =
		{
            if (telem > 0 && modetab[telem] == MSTRUCT)
            {
                do
                    Expr_gen(0);
                while (tree[tc] != TENDINIT);
                tc++;
                tocode(COPY0STASS);
                tocode(olddispl);
                tocode(all);       // общее кол-во слов
            }
            else
            {
                Expr_gen(0);
                tocode(telem == LFLOAT ? ASSRV : ASSV);
                tocode(olddispl);
            }
		}
	}
	else                                // Обработка массива int a[N1]...[NN] =
	{
		tocode(DEFARR);                 // DEFARR N, d, displ, iniproc     N1...NN уже лежат на стеке
        tocode(N);
        tocode(element_len);
		tocode(olddispl);
        tocode(iniprocs[iniproc]);
        
        if (all)                        // all - общее количество выражений в инициализации
            {
                int wasstring = 0;
                do
                    wasstring = Expr_gen(0);
                while (tree[tc] != TENDINIT);
                tc++;
                if (wasstring)
                {
                    tocode(STRINGINIT);    // на стеке адрес 0-го элемента строки
                    tocode(olddispl);
                }
                else
                {
                    tocode(ARRINIT);        // ARRINIT N d all displ
                    tocode(abs(N));
                    tocode(element_len);
                    tocode(all);
                    tocode(olddispl);
                }

            }
	}
}

void compstmt_gen()
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
                for (i=0; i<N; i++)
                    Expr_gen(0);
                break;
            }

            case TDeclid:
                tc++;
                Declid_gen();
                break;
                
            default:
                Stmt_gen();
        }
    }
    tc++;
}
void codegen()
{
    int treesize = tc;
    tc = 0;
    while (tc < treesize)
    {
        switch (tree[tc++])
        {
            case TFuncdef:
            {
                int identref = tree[tc++], maxdispl = tree[tc++];
                int fn = identab[identref+3], pred;
                functions[fn]= pc;
                tocode(FUNCBEG);
                tocode(maxdispl);
                pred = pc++;
                tc++;             // TBegin
                compstmt_gen();
                mem[pred] = pc;
            }
                break;
                
            case TDeclarr:
            {
                int i, N = tree[tc++];
                for (i=0; i<N; i++)
                    Expr_gen(0);
                break;
            }
       
            case TDeclid:
                Declid_gen();
                break;
                
            case NOP:
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

                
            default:
            {
                printf("что-то не то\n");
                printf("tc=%i tree[tc-2]=%i tree[tc-1]=%i\n", tc, tree[tc-2], tree[tc-1]);
            }
        }
    }
    
    if (wasmain == 0)
        error(no_main_in_program);
    tocode(CALL1);
    tocode(CALL2);
    tocode(identab[wasmain+3]);
    tocode(STOP);
}
