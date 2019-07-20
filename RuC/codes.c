//
//  codes.c
//  RuC
//
//  Created by Andrey Terekhov on 03/06/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "global_vars.h"

extern void
fprintf_char(FILE *f, int wchar);

void
tablesandtree(ruc_context *context)
{
    int i = 0, j;

    fprintf(context->output, "\n%s\n", "source");
    for (i = 1; i < context->line; i++)
    {
        fprintf(context->output, "line %i) ", i);
        for (j = context->lines[i]; j < context->lines[i + 1]; j++)
        {
            fprintf_char(context->output, context->source[j]);
        }
    }
    fprintf(context->output, "\n");

    fprintf(context->output, "\n%s\n", "identab");
    i = 2;
    while (i < context->id)
    {
        for (j = 0; j < 4; j++)
            fprintf(context->output, "id %i) %i\n", i + j,
                    context->identab[i + j]);
        fprintf(context->output, "\n");
        i += 4;
    }
    /*
        fprintf(context->output, "\n%s\n", "repr");
        for (i = 1206; i <= rp; i++)
            fprintf(context->output, "rp %i) %i\n", i, reprtab[i]);
     */
    fprintf(context->output, "\n%s\n", "modetab");
    for (i = 0; i < context->md; i++)
        fprintf(context->output, "md %i) %i\n", i, context->modetab[i]);
    /*
        fprintf(context->output, "\n%s\n", "tree");
        for (i=0; i<=tc; i++)
            fprintf(context->output, "tc %i) %i\n", i, context->tree[i]);
    */
    fprintf(context->output, "\n");
    i = 0;
    while (i < context->tc)
    {
        fprintf(context->output, "tc %i) ", i);
        switch (context->tree[i++])
        {
            case TFuncdef:
                fprintf(context->output, "TFuncdef funcn= %i maxdispl= %i\n",
                        context->tree[i], context->tree[i + 1]);
                i += 2;
                break;
            case TDeclarr:
                fprintf(context->output, "TDeclarr N= %i\n",
                        context->tree[i++]);
                break;
            case TDeclid:
                fprintf(context->output,
                        "TDeclid displ= %i eltype= %i N= %i all= %i iniproc= "
                        "%i, usual= %i instuct= %i\n",
                        context->tree[i], context->tree[i + 1],
                        context->tree[i + 2], context->tree[i + 3],
                        context->tree[i + 4], context->tree[i + 5],
                        context->tree[i + 6]);
                i += 7;
                break;
            case TString:
                fprintf(context->output, "TString n= %i\n", context->tree[i++]);
                break;
            case TCondexpr:
                fprintf(context->output, "TCondexpr\n");
                break;
            case TBegin:
                fprintf(context->output, "TBegin\n");
                break;
            case TEnd:
                fprintf(context->output, "TEnd\n");
                break;
            case TBeginit:
                fprintf(context->output, "TBeginit n= %i\n",
                        context->tree[i++]);
                break;
            case TStructinit:
                fprintf(context->output, "TStructinit n= %i\n",
                        context->tree[i++]);
                break;
            case TIf:
                fprintf(context->output, "TIf %i\n", context->tree[i++]);
                break;
            case TWhile:
                fprintf(context->output, "TWhile\n");
                break;
            case TDo:
                fprintf(context->output, "TDo\n");
                break;
            case TFor:
                fprintf(context->output, "TFor %i %i %i %i\n", context->tree[i],
                        context->tree[i + 1], context->tree[i + 2],
                        context->tree[i + 3]);
                i += 4;
                break;
            case TSwitch:
                fprintf(context->output, "TSwitch\n");
                break;
            case TCase:
                fprintf(context->output, "TCase\n");
                break;
            case TDefault:
                fprintf(context->output, "TDefault\n");
                break;
            case TBreak:
                fprintf(context->output, "TBreak\n");
                break;
            case TContinue:
                fprintf(context->output, "TContinue\n");
                break;
            case TReturnvoid:
                fprintf(context->output, "TReturn\n");
                break;
            case TReturnval:
                fprintf(context->output, "TReturnval %i\n", context->tree[i++]);
                break;
            case TGoto:
                fprintf(context->output, "TGoto %i\n", context->tree[i++]);
                break;
            case TIdent:
                fprintf(context->output, "TIdent %i\n", context->tree[i++]);
                break;
            case TIdenttoval:
                fprintf(context->output, "TIdenttoval %i\n",
                        context->tree[i++]);
                break;
            case TIdenttovald:
                fprintf(context->output, "TIdenttovald %i\n",
                        context->tree[i++]);
                break;
            case TFunidtoval:
                fprintf(context->output, "TFunidtoval %i\n",
                        context->tree[i++]);
                break;
            case TIdenttoaddr:
                fprintf(context->output, "TIdenttoaddr %i\n",
                        context->tree[i++]);
                break;
            case TAddrtoval:
                fprintf(context->output, "TAddrtoval\n");
                break;
            case TAddrtovald:
                fprintf(context->output, "TAddrtovald\n");
                break;
            case TExprend:
                fprintf(context->output, "TExprend\n");
                break;
            case TConst:
                fprintf(context->output, "TConst %i\n", context->tree[i++]);
                break;
            case TConstd:
                memcpy(&context->numdouble, &context->tree[i], sizeof(double));
                i += 2;
                fprintf(context->output, "TConstd %f\n", context->numdouble);
                break;
            case TSliceident:
                fprintf(context->output, "TSliceident displ= %i type= %i\n",
                        context->tree[i], context->tree[i + 1]);
                i += 2;
                break;
            case TSlice:
                fprintf(context->output, "TSlice elem_type= %i\n",
                        context->tree[i++]);
                break;
            case TSelect:
                fprintf(context->output, "TSelect displ= %i\n",
                        context->tree[i++]);
                break;
            case NOP:
                fprintf(context->output, "NOP\n");
                break;
            case ADLOGAND:
                fprintf(context->output, "ADLOGAND addr= %i\n",
                        context->tree[i++]);
                break;
            case ADLOGOR:
                fprintf(context->output, "ADLOGOR addr= %i\n",
                        context->tree[i++]);
                break;
            case COPY00:
                fprintf(context->output, "COPY00 %i ",
                        context->tree[i++]); // displleft
                fprintf(context->output, "%i ",
                        context->tree[i++]); // displright
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY01:
                fprintf(context->output, "COPY01 %i ",
                        context->tree[i++]); // displleft
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY10:
                fprintf(context->output, "COPY10 %i ",
                        context->tree[i++]); // displright
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY11:
                fprintf(context->output, "COPY11 %i\n",
                        context->tree[i++]); // length
                break;
            case COPY0ST:
                fprintf(context->output, "COPY0ST %i ",
                        context->tree[i++]); // displleft
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY1ST:
                fprintf(context->output, "COPY1ST (%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY0STASS:
                fprintf(context->output, "COPY0STASS %i ",
                        context->tree[i++]); // displleft
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length
                break;
            case COPY1STASS:
                fprintf(context->output, "COPY1STASS (%i)\n",
                        context->tree[i++]); // length
                break;
            case COPYST:
                fprintf(context->output, "COPYST %i ",
                        context->tree[i++]); // displ
                fprintf(context->output, "(%i)", context->tree[i++]); // length
                fprintf(context->output, "(%i)\n",
                        context->tree[i++]); // length1
                break;

            case TCall1:
                fprintf(context->output, "TCall1 %i\n", context->tree[i++]);
                break;
            case TCall2:
                fprintf(context->output, "TCall2 %i\n", context->tree[i++]);
                break;
            case TLabel:
                fprintf(context->output, "TLabel %i\n", context->tree[i++]);
                break;
            case TStructbeg:
                fprintf(context->output, "TStructbeg %i\n", context->tree[i++]);
                break;
            case TStructend:
                fprintf(context->output, "TStructend %i\n", context->tree[i++]);
                break;
            case TPrint:
                fprintf(context->output, "TPrint %i\n", context->tree[i++]);
                break;
            case TPrintid:
                fprintf(context->output, "TPrintid %i\n", context->tree[i++]);
                break;
            case TPrintf:
                fprintf(context->output, "TPrintf %i\n", context->tree[i++]);
                break;
            case TGetid:
                fprintf(context->output, "TGetid %i\n", context->tree[i++]);
                break;
            case SETMOTORC:
                fprintf(context->output, "Setmotor\n");
                break;
            case CREATEC:
                fprintf(context->output, "TCREATE\n");
                break;
            case CREATEDIRECTC:
                fprintf(context->output, "TCREATEDIRECT\n");
                break;
            case EXITC:
                fprintf(context->output, "TEXIT\n");
                break;
            case EXITDIRECTC:
                fprintf(context->output, "TEXITDIRECT\n");
                break;
            case MSGSENDC:
                fprintf(context->output, "TMSGSEND\n");
                break;
            case MSGRECEIVEC:
                fprintf(context->output, "TMSGRECEIVE\n");
                break;
            case JOINC:
                fprintf(context->output, "TJOIN\n");
                break;
            case SLEEPC:
                fprintf(context->output, "TSLEEP\n");
                break;
            case SEMCREATEC:
                fprintf(context->output, "TSEMCREATE\n");
                break;
            case SEMWAITC:
                fprintf(context->output, "TSEMWAIT\n");
                break;
            case SEMPOSTC:
                fprintf(context->output, "TSEMPOST\n");
                break;
            case INITC:
                fprintf(context->output, "INITC\n");
                break;
            case DESTROYC:
                fprintf(context->output, "DESTROYC\n");
                break;
            case GETNUMC:
                fprintf(context->output, "GETNUMC\n");
                break;


            default:
                fprintf(context->output, "TOper %i\n", context->tree[i - 1]);
        }
    }
}

void
tablesandcode(ruc_context *context)
{
    int i = 0, j;

    fprintf(context->output, "\n%s\n", "source");
    for (i = 1; i < context->line; i++)
    {
        fprintf(context->output, "line %i) ", i);
        for (j = context->lines[i]; j < context->lines[i + 1]; j++)
        {
            fprintf_char(context->output, context->source[j]);
        }
    }

    fprintf(context->output, "\n\n%s\n", "functions");
    for (i = 1; i <= context->funcnum; i++)
        fprintf(context->output, "fun %i) %i\n", i, context->functions[i]);

    fprintf(context->output, "\n%s\n", "iniprocs");
    for (i = 1; i <= context->procd; i++)
        fprintf(context->output, "inipr %i) %i\n", i, context->iniprocs[i]);

    fprintf(context->output, "\n%s\n", "mem");
    i = 0;
    while (i < context->pc)
    {
        fprintf(context->output, "pc %i) ", i);
        switch (context->mem[i++])
        {
            case PRINT:
                fprintf(context->output, "PRINT %i\n", context->mem[i++]);
                break;
            case PRINTID:
                fprintf(context->output, "PRINTID %i\n", context->mem[i++]);
                break;
            case PRINTF:
                fprintf(context->output, "PRINTF %i\n", context->mem[i++]);
                break;
            case GETID:
                fprintf(context->output, "GETID %i\n", context->mem[i++]);
                break;
            case SETMOTORC:
                fprintf(context->output, "SETMOTOR\n");
                break;
            case GETDIGSENSORC:
                fprintf(context->output, "GETDIGSENSOR\n");
                break;
            case GETANSENSORC:
                fprintf(context->output, "GETANSENSOR\n");
                break;
            case VOLTAGEC:
                fprintf(context->output, "VOLTAGE\n");
                break;
            case CREATEC:
                fprintf(context->output, "TCREATE\n");
                break;
            case CREATEDIRECTC:
                fprintf(context->output, "TCREATEDIRECT\n");
                break;
            case MSGSENDC:
                fprintf(context->output, "TMSGSEND\n");
                break;
            case EXITC:
                fprintf(context->output, "TEXIT\n");
                break;
            case EXITDIRECTC:
                fprintf(context->output, "TEXITDIRECT\n");
                break;
            case MSGRECEIVEC:
                fprintf(context->output, "TMSGRECEIVE\n");
                break;
            case JOINC:
                fprintf(context->output, "TJOIN\n");
                break;
            case SLEEPC:
                fprintf(context->output, "TSLEEP\n");
                break;
            case SEMCREATEC:
                fprintf(context->output, "TSEMCREATE\n");
                break;
            case SEMWAITC:
                fprintf(context->output, "TSEMWAIT\n");
                break;
            case SEMPOSTC:
                fprintf(context->output, "TSEMPOST\n");
                break;
            case TINIT:
                fprintf(context->output, "TINIT\n");
                break;
            case TDESTROY:
                fprintf(context->output, "TDESTROY\n");
                break;
            case GETNUMC:
                fprintf(context->output, "GETNUM\n");
                break;

            case ABSC:
                fprintf(context->output, "ABS\n");
                break;
            case ABSIC:
                fprintf(context->output, "ABSI\n");
                break;
            case SQRTC:
                fprintf(context->output, "SQRT\n");
                break;
            case EXPC:
                fprintf(context->output, "EXP\n");
                break;
            case SINC:
                fprintf(context->output, "SIN\n");
                break;
            case COSC:
                fprintf(context->output, "COS\n");
                break;
            case LOGC:
                fprintf(context->output, "LOG\n");
                break;
            case LOG10C:
                fprintf(context->output, "LOG10\n");
                break;
            case ASINC:
                fprintf(context->output, "ASIN\n");
                break;
            case RANDC:
                fprintf(context->output, "RAND\n");
                break;
            case ROUNDC:
                fprintf(context->output, "ROUND\n");
                break;

            case STRCPYC:
                fprintf(context->output, "STRCPY\n");
                break;
            case STRNCPYC:
                fprintf(context->output, "STRNCPY\n");
                break;
            case STRCATC:
                fprintf(context->output, "STRCAT\n");
                break;
            case STRNCATC:
                fprintf(context->output, "STRNCAT\n");
                break;
            case STRCMPC:
                fprintf(context->output, "STRCMP\n");
                break;
            case STRNCMPC:
                fprintf(context->output, "STRNCMP\n");
                break;
            case STRSTRC:
                fprintf(context->output, "STRSTR\n");
                break;
            case STRLENC:
                fprintf(context->output, "STRLENC\n");
                break;

            case BEGINIT:
                fprintf(context->output, "BEGINIT n= %i\n", context->mem[i++]);
                break;
            case STRUCTWITHARR:
                fprintf(context->output, "STRUCTWITHARR displ= %i ",
                        context->mem[i++]);
                fprintf(context->output, "iniproc= %i\n", context->mem[i++]);
                break;
            case DEFARR:
                fprintf(context->output, "DEFARR N= %i ",
                        context->mem[i++]); // N
                fprintf(context->output, "elem_len= %i ",
                        context->mem[i++]); // elem length
                fprintf(context->output, "displ= %i ",
                        context->mem[i++]); // displ
                fprintf(context->output, "iniproc= %i ",
                        context->mem[i++]); // iniproc
                fprintf(context->output, "usual= %i ",
                        context->mem[i++]); // usual
                fprintf(context->output, "all= %i ", context->mem[i++]); // all
                fprintf(context->output, "instruct= %i\n",
                        context->mem[i++]); // instruct
                break;
            case ARRINIT:
                fprintf(context->output, "ARRINIT N= %i ", context->mem[i++]);
                fprintf(context->output, "elem_len= %i ", context->mem[i++]);
                fprintf(context->output, "displ= %i ", context->mem[i++]);
                fprintf(context->output, "usual= %i\n", context->mem[i++]);
                break;
                //            case STRUCTINIT:
                //                fprintf(context->output, "STRUCTINIT N= %i ",
                //                context->mem[i++]);
                //            break;
            case NOP:
                fprintf(context->output, "NOP\n");
                break;
            case LI:
                fprintf(context->output, "LI %i\n", context->mem[i++]);
                break;
            case LID:
                memcpy(&context->numdouble, &context->mem[i], sizeof(double));
                i += 2;
                fprintf(context->output, "LID %.15f\n", context->numdouble);
                break;
            case LOAD:
                fprintf(context->output, "LOAD %i\n", context->mem[i++]);
                break;
            case LOADD:
                fprintf(context->output, "LOADD %i\n", context->mem[i++]);
                break;
            case LAT:
                fprintf(context->output, "L@\n");
                break;
            case LATD:
                fprintf(context->output, "L@f\n");
                break;
            case LA:
                fprintf(context->output, "LA %i\n", context->mem[i++]);
                break;

            case LOGOR:
                fprintf(context->output, "||\n");
                break;
            case LOGAND:
                fprintf(context->output, "&&\n");
                break;
            case ORASS:
                fprintf(context->output, "|= %i\n", context->mem[i++]);
                break;
            case ORASSAT:
                fprintf(context->output, "|=@\n");
                break;
            case ORASSV:
                fprintf(context->output, "|=V %i\n", context->mem[i++]);
                break;
            case ORASSATV:
                fprintf(context->output, "|=@V\n");
                break;
            case LOR:
                fprintf(context->output, "|\n");
                break;
            case EXORASS:
                fprintf(context->output, "^= %i\n", context->mem[i++]);
                break;
            case EXORASSAT:
                fprintf(context->output, "^=@\n");
                break;
            case EXORASSV:
                fprintf(context->output, "^=V %i\n", context->mem[i++]);
                break;
            case EXORASSATV:
                fprintf(context->output, "^=@V\n");
                break;
            case LEXOR:
                fprintf(context->output, "^\n");
                break;
            case ANDASS:
                fprintf(context->output, "&= %i\n", context->mem[i++]);
                break;
            case ANDASSAT:
                fprintf(context->output, "&=@\n");
                break;
            case ANDASSV:
                fprintf(context->output, "&=V %i\n", context->mem[i++]);
                break;
            case ANDASSATV:
                fprintf(context->output, "&=@V\n");
                break;
            case LAND:
                fprintf(context->output, "&\n");
                break;

            case EQEQ:
                fprintf(context->output, "==\n");
                break;
            case NOTEQ:
                fprintf(context->output, "!=\n");
                break;
            case LLT:
                fprintf(context->output, "<\n");
                break;
            case LGT:
                fprintf(context->output, ">\n");
                break;
            case LLE:
                fprintf(context->output, "<=\n");
                break;
            case LGE:
                fprintf(context->output, ">=\n");
                break;
            case EQEQR:
                fprintf(context->output, "==f\n");
                break;
            case NOTEQR:
                fprintf(context->output, "!=f\n");
                break;
            case LLTR:
                fprintf(context->output, "<f\n");
                break;
            case LGTR:
                fprintf(context->output, ">f\n");
                break;
            case LLER:
                fprintf(context->output, "<=f\n");
                break;
            case LGER:
                fprintf(context->output, ">=f\n");
                break;

            case SHRASS:
                fprintf(context->output, ">>= %i\n", context->mem[i++]);
                break;
            case SHRASSAT:
                fprintf(context->output, ">>=@\n");
                break;
            case SHRASSV:
                fprintf(context->output, ">>=V %i\n", context->mem[i++]);
                break;
            case SHRASSATV:
                fprintf(context->output, ">>=@V\n");
                break;
            case LSHR:
                fprintf(context->output, ">>\n");
                break;
            case SHLASS:
                fprintf(context->output, "<<= %i\n", context->mem[i++]);
                break;
            case SHLASSAT:
                fprintf(context->output, "<<=@\n");
                break;
            case SHLASSV:
                fprintf(context->output, "<<=V %i\n", context->mem[i++]);
                break;
            case SHLASSATV:
                fprintf(context->output, "<<=@V\n");
                break;
            case LSHL:
                fprintf(context->output, "<<\n");
                break;

            case ASS:
                fprintf(context->output, "= %i\n", context->mem[i++]);
                break;
            case ASSAT:
                fprintf(context->output, "=@\n");
                break;
            case ASSV:
                fprintf(context->output, "=V %i\n", context->mem[i++]);
                break;
            case ASSATV:
                fprintf(context->output, "=@V\n");
                break;

            case PLUSASS:
                fprintf(context->output, "+= %i\n", context->mem[i++]);
                break;
            case PLUSASSAT:
                fprintf(context->output, "+=@\n");
                break;
            case PLUSASSV:
                fprintf(context->output, "+=V %i\n", context->mem[i++]);
                break;
            case PLUSASSATV:
                fprintf(context->output, "+=@V\n");
                break;
            case LPLUS:
                fprintf(context->output, "+\n");
                break;

            case MINUSASS:
                fprintf(context->output, "-= %i\n", context->mem[i++]);
                break;
            case MINUSASSAT:
                fprintf(context->output, "-=@\n");
                break;
            case MINUSASSV:
                fprintf(context->output, "-=V %i\n", context->mem[i++]);
                break;
            case MINUSASSATV:
                fprintf(context->output, "-=@V\n");
                break;
            case LMINUS:
                fprintf(context->output, "-\n");
                break;

            case MULTASS:
                fprintf(context->output, "*= %i\n", context->mem[i++]);
                break;
            case MULTASSAT:
                fprintf(context->output, "*=@\n");
                break;
            case MULTASSV:
                fprintf(context->output, "*=V %i\n", context->mem[i++]);
                break;
            case MULTASSATV:
                fprintf(context->output, "*=@V\n");
                break;
            case LMULT:
                fprintf(context->output, "*\n");
                break;

            case DIVASS:
                fprintf(context->output, "/= %i\n", context->mem[i++]);
                break;
            case DIVASSAT:
                fprintf(context->output, "/=@\n");
                break;
            case DIVASSV:
                fprintf(context->output, "/=V %i\n", context->mem[i++]);
                break;
            case DIVASSATV:
                fprintf(context->output, "/=@V\n");
                break;
            case LDIV:
                fprintf(context->output, "/\n");
                break;

            case ASSR:
                fprintf(context->output, "=f %i\n", context->mem[i++]);
                break;
            case ASSRV:
                fprintf(context->output, "=fV %i\n", context->mem[i++]);
                break;
            case ASSATR:
                fprintf(context->output, "=@f\n");
                break;
            case ASSATRV:
                fprintf(context->output, "=@fV\n");
                break;

            case PLUSASSR:
                fprintf(context->output, "+=f %i\n", context->mem[i++]);
                break;
            case PLUSASSATR:
                fprintf(context->output, "+=@f\n");
                break;
            case PLUSASSRV:
                fprintf(context->output, "+=fV %i\n", context->mem[i++]);
                break;
            case PLUSASSATRV:
                fprintf(context->output, "+=@fV\n");
                break;
            case LPLUSR:
                fprintf(context->output, "+f\n");
                break;
            case MINUSASSR:
                fprintf(context->output, "-=f %i\n", context->mem[i++]);
                break;
            case MINUSASSATR:
                fprintf(context->output, "-=@f\n");
                break;
            case MINUSASSRV:
                fprintf(context->output, "-=fV %i\n", context->mem[i++]);
                break;
            case MINUSASSATRV:
                fprintf(context->output, "-=@fV\n");
                break;
            case LMINUSR:
                fprintf(context->output, "-f\n");
                break;
            case MULTASSR:
                fprintf(context->output, "*=f %i\n", context->mem[i++]);
                break;
            case MULTASSATR:
                fprintf(context->output, "*=@f\n");
                break;
            case MULTASSRV:
                fprintf(context->output, "*=fV %i\n", context->mem[i++]);
                break;
            case MULTASSATRV:
                fprintf(context->output, "*=@fV\n");
                break;
            case LMULTR:
                fprintf(context->output, "*f\n");
                break;
            case DIVASSR:
                fprintf(context->output, "/=f %i\n", context->mem[i++]);
                break;
            case DIVASSATR:
                fprintf(context->output, "/=@f\n");
                break;
            case DIVASSRV:
                fprintf(context->output, "/=fV %i\n", context->mem[i++]);
                break;
            case DIVASSATRV:
                fprintf(context->output, "/=@fV\n");
                break;
            case LDIVR:
                fprintf(context->output, "/f\n");
                break;
            case COPY00:
                fprintf(context->output, "COPY00 %i ",
                        context->mem[i++]); // displleft
                fprintf(context->output, "%i ",
                        context->mem[i++]); // displright
                fprintf(context->output, "(%i)\n", context->mem[i++]); // length
                break;
            case COPY01:
                fprintf(context->output, "COPY01 %i      ",
                        context->mem[i++]); // displleft
                fprintf(context->output, "(%i)\n", context->mem[i++]); // length
                break;
            case COPY10:
                fprintf(context->output, "COPY10      %i ",
                        context->mem[i++]); // displright
                fprintf(context->output, "(%i)\n", context->mem[i++]); // length
                break;
            case COPY11:
                fprintf(context->output, "COPY11 %i\n",
                        context->mem[i++]); // length
                break;
            case COPY0ST:
                fprintf(context->output, "COPY0ST %i ",
                        context->mem[i++]); // displright
                fprintf(context->output, "(%i)\n", context->mem[i++]); // length
                break;
            case COPY1ST:
                fprintf(context->output, "COPY1ST %i\n",
                        context->mem[i++]); // length
                break;
            case COPY0STASS:
                fprintf(context->output, "COPY0STASS %i ",
                        context->mem[i++]); // displleft
                fprintf(context->output, "(%i)\n", context->mem[i++]); // length
                break;
            case COPY1STASS:
                fprintf(context->output, "COPY1STASS %i\n",
                        context->mem[i++]); // length
                break;
            case COPYST:
                fprintf(context->output, "COPYST %i ",
                        context->mem[i++]); // displ
                fprintf(context->output, "(%i)", context->mem[i++]); // length
                fprintf(context->output, "(%i)\n",
                        context->mem[i++]); // length1
                break;

            case REMASS:
                fprintf(context->output, "%%= %i\n", context->mem[i++]);
                break;
            case REMASSAT:
                fprintf(context->output, "%%=@\n");
                break;
            case REMASSV:
                fprintf(context->output, "%%=V %i\n", context->mem[i++]);
                break;
            case REMASSATV:
                fprintf(context->output, "%%=@V\n");
                break;
            case LREM:
                fprintf(context->output, "%%\n");
                break;

            case CALL1:
                fprintf(context->output, "CALL1\n");
                break;
            case CALL2:
                fprintf(context->output, "CALL2 ");
                fprintf(context->output, "%i\n", context->mem[i++]);
                break;
            case STOP:
                fprintf(context->output, "STOP\n");
                break;
            case RETURNVAL:
                fprintf(context->output, "RETURNVAL %i\n", context->mem[i++]);
                break;
            case RETURNVOID:
                fprintf(context->output, "RETURNVOID\n");
                break;
            case B:
                fprintf(context->output, "B %i\n", context->mem[i++]);
                break;
            case BE0:
                fprintf(context->output, "BE0 %i\n", context->mem[i++]);
                break;
            case BNE0:
                fprintf(context->output, "BNE0 %i\n", context->mem[i++]);
                break;
            case SLICE:
                fprintf(context->output, "SLICE d= %i\n", context->mem[i++]);
                break;
            case SELECT:
                fprintf(context->output, "SELECT field_displ= %i\n",
                        context->mem[i++]);
                break;
            case WIDEN:
                fprintf(context->output, "WIDEN\n");
                break;
            case WIDEN1:
                fprintf(context->output, "WIDEN1\n");
                break;
            case _DOUBLE:
                fprintf(context->output, "DOUBLE\n");
                break;
            case INC:
                fprintf(context->output, "INC %i\n", context->mem[i++]);
                break;
            case DEC:
                fprintf(context->output, "DEC %i\n", context->mem[i++]);
                break;
            case POSTINC:
                fprintf(context->output, "POSTINC %i\n", context->mem[i++]);
                break;
            case POSTDEC:
                fprintf(context->output, "POSTDEC %i\n", context->mem[i++]);
                break;
            case INCAT:
                fprintf(context->output, "INC@\n");
                break;
            case DECAT:
                fprintf(context->output, "DEC@\n");
                break;
            case POSTINCAT:
                fprintf(context->output, "POSTINC@\n");
                break;
            case POSTDECAT:
                fprintf(context->output, "POSTDEC@\n");
                break;
            case INCR:
                fprintf(context->output, "INCf %i\n", context->mem[i++]);
                break;
            case DECR:
                fprintf(context->output, "DECf %i\n", context->mem[i++]);
                break;
            case POSTINCR:
                fprintf(context->output, "POSTINCf %i\n", context->mem[i++]);
                break;
            case POSTDECR:
                fprintf(context->output, "POSTDECf %i\n", context->mem[i++]);
                break;
            case INCATR:
                fprintf(context->output, "INC@f\n");
                break;
            case DECATR:
                fprintf(context->output, "DEC@f\n");
                break;
            case POSTINCATR:
                fprintf(context->output, "POSTINC@f\n");
                break;
            case POSTDECATR:
                fprintf(context->output, "POSTDEC@f\n");
                break;
            case INCV:
                fprintf(context->output, "INCV %i\n", context->mem[i++]);
                break;
            case DECV:
                fprintf(context->output, "DECV %i\n", context->mem[i++]);
                break;
            case POSTINCV:
                fprintf(context->output, "POSTINCV %i\n", context->mem[i++]);
                break;
            case POSTDECV:
                fprintf(context->output, "POSTDECV %i\n", context->mem[i++]);
                break;
            case INCATV:
                fprintf(context->output, "INC@V\n");
                break;
            case DECATV:
                fprintf(context->output, "DEC@V\n");
                break;
            case POSTINCATV:
                fprintf(context->output, "POSTINC@V\n");
                break;
            case POSTDECATV:
                fprintf(context->output, "POSTDEC@V\n");
                break;
            case INCRV:
                fprintf(context->output, "INCfV %i\n", context->mem[i++]);
                break;
            case DECRV:
                fprintf(context->output, "DECfV %i\n", context->mem[i++]);
                break;
            case POSTINCRV:
                fprintf(context->output, "POSTINCfV %i\n", context->mem[i++]);
                break;
            case POSTDECRV:
                fprintf(context->output, "POSTDECfV %i\n", context->mem[i++]);
                break;
            case INCATRV:
                fprintf(context->output, "INC@fV\n");
                break;
            case DECATRV:
                fprintf(context->output, "DEC@fV\n");
                break;
            case POSTINCATRV:
                fprintf(context->output, "POSTINC@fV\n");
                break;
            case POSTDECATRV:
                fprintf(context->output, "POSTDEC@fV\n");
                break;

            case LNOT:
                fprintf(context->output, "BITNOT\n");
                break;
            case LOGNOT:
                fprintf(context->output, "NOT\n");
                break;
            case UNMINUS:
                fprintf(context->output, "UNMINUS\n");
                break;
            case UNMINUSR:
                fprintf(context->output, "UNMINUSf\n");
                break;

            case FUNCBEG:
                fprintf(context->output, "FUNCBEG maxdispl= %i ",
                        context->mem[i++]);
                fprintf(context->output, "pc= %i\n", context->mem[i++]);
                break;


            default:
                fprintf(context->output, "%i\n", context->mem[i - 1]);
        }
    }
}
