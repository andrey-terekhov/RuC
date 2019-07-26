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

void
tablesandtree(compiler_context *context)
{
    int i = 0, j;

    printer_printf(&context->output_options, "\n%s\n", "source");
    for (i = 1; i < context->line; i++)
    {
        printer_printf(&context->output_options, "line %i) ", i);
        for (j = context->lines[i]; j < context->lines[i + 1]; j++)
        {
            printer_printchar(&context->output_options, context->source[j]);
        }
    }
    printer_printf(&context->output_options, "\n");

    printer_printf(&context->output_options, "\n%s\n", "identab");
    i = 2;
    while (i < context->id)
    {
        for (j = 0; j < 4; j++)
            printer_printf(&context->output_options, "id %i) %i\n", i + j,
                           context->identab[i + j]);
        printer_printf(&context->output_options, "\n");
        i += 4;
    }
    /*
        printer_printf(&context->output_options, "\n%s\n", "repr");
        for (i = 1206; i <= rp; i++)
            printer_printf(&context->output_options, "rp %i) %i\n", i,
       reprtab[i]);
     */
    printer_printf(&context->output_options, "\n%s\n", "modetab");
    for (i = 0; i < context->md; i++)
        printer_printf(&context->output_options, "md %i) %i\n", i,
                       context->modetab[i]);
    /*
        printer_printf(&context->output_options, "\n%s\n", "tree");
        for (i=0; i<=tc; i++)
            printer_printf(&context->output_options, "tc %i) %i\n", i,
       context->tree[i]);
    */
    printer_printf(&context->output_options, "\n");
    i = 0;
    while (i < context->tc)
    {
        printer_printf(&context->output_options, "tc %i) ", i);
        switch (context->tree[i++])
        {
            case TFuncdef:
                printer_printf(&context->output_options,
                               "TFuncdef funcn= %i maxdispl= %i\n",
                               context->tree[i], context->tree[i + 1]);
                i += 2;
                break;
            case TDeclarr:
                printer_printf(&context->output_options, "TDeclarr N= %i\n",
                               context->tree[i++]);
                break;
            case TDeclid:
                printer_printf(
                    &context->output_options,
                    "TDeclid displ= %i eltype= %i N= %i all= %i iniproc= "
                    "%i, usual= %i instuct= %i\n",
                    context->tree[i], context->tree[i + 1],
                    context->tree[i + 2], context->tree[i + 3],
                    context->tree[i + 4], context->tree[i + 5],
                    context->tree[i + 6]);
                i += 7;
                break;
            case TString:
                printer_printf(&context->output_options, "TString n= %i\n",
                               context->tree[i++]);
                break;
            case TCondexpr:
                printer_printf(&context->output_options, "TCondexpr\n");
                break;
            case TBegin:
                printer_printf(&context->output_options, "TBegin\n");
                break;
            case TEnd:
                printer_printf(&context->output_options, "TEnd\n");
                break;
            case TBeginit:
                printer_printf(&context->output_options, "TBeginit n= %i\n",
                               context->tree[i++]);
                break;
            case TStructinit:
                printer_printf(&context->output_options, "TStructinit n= %i\n",
                               context->tree[i++]);
                break;
            case TIf:
                printer_printf(&context->output_options, "TIf %i\n",
                               context->tree[i++]);
                break;
            case TWhile:
                printer_printf(&context->output_options, "TWhile\n");
                break;
            case TDo:
                printer_printf(&context->output_options, "TDo\n");
                break;
            case TFor:
                printer_printf(&context->output_options, "TFor %i %i %i %i\n",
                               context->tree[i], context->tree[i + 1],
                               context->tree[i + 2], context->tree[i + 3]);
                i += 4;
                break;
            case TSwitch:
                printer_printf(&context->output_options, "TSwitch\n");
                break;
            case TCase:
                printer_printf(&context->output_options, "TCase\n");
                break;
            case TDefault:
                printer_printf(&context->output_options, "TDefault\n");
                break;
            case TBreak:
                printer_printf(&context->output_options, "TBreak\n");
                break;
            case TContinue:
                printer_printf(&context->output_options, "TContinue\n");
                break;
            case TReturnvoid:
                printer_printf(&context->output_options, "TReturn\n");
                break;
            case TReturnval:
                printer_printf(&context->output_options, "TReturnval %i\n",
                               context->tree[i++]);
                break;
            case TGoto:
                printer_printf(&context->output_options, "TGoto %i\n",
                               context->tree[i++]);
                break;
            case TIdent:
                printer_printf(&context->output_options, "TIdent %i\n",
                               context->tree[i++]);
                break;
            case TIdenttoval:
                printer_printf(&context->output_options, "TIdenttoval %i\n",
                               context->tree[i++]);
                break;
            case TIdenttovald:
                printer_printf(&context->output_options, "TIdenttovald %i\n",
                               context->tree[i++]);
                break;
            case TFunidtoval:
                printer_printf(&context->output_options, "TFunidtoval %i\n",
                               context->tree[i++]);
                break;
            case TIdenttoaddr:
                printer_printf(&context->output_options, "TIdenttoaddr %i\n",
                               context->tree[i++]);
                break;
            case TAddrtoval:
                printer_printf(&context->output_options, "TAddrtoval\n");
                break;
            case TAddrtovald:
                printer_printf(&context->output_options, "TAddrtovald\n");
                break;
            case TExprend:
                printer_printf(&context->output_options, "TExprend\n");
                break;
            case TConst:
                printer_printf(&context->output_options, "TConst %i\n",
                               context->tree[i++]);
                break;
            case TConstd:
                memcpy(&context->numdouble, &context->tree[i], sizeof(double));
                i += 2;
                printer_printf(&context->output_options, "TConstd %f\n",
                               context->numdouble);
                break;
            case TSliceident:
                printer_printf(&context->output_options,
                               "TSliceident displ= %i type= %i\n",
                               context->tree[i], context->tree[i + 1]);
                i += 2;
                break;
            case TSlice:
                printer_printf(&context->output_options,
                               "TSlice elem_type= %i\n", context->tree[i++]);
                break;
            case TSelect:
                printer_printf(&context->output_options, "TSelect displ= %i\n",
                               context->tree[i++]);
                break;
            case NOP:
                printer_printf(&context->output_options, "NOP\n");
                break;
            case ADLOGAND:
                printer_printf(&context->output_options, "ADLOGAND addr= %i\n",
                               context->tree[i++]);
                break;
            case ADLOGOR:
                printer_printf(&context->output_options, "ADLOGOR addr= %i\n",
                               context->tree[i++]);
                break;
            case COPY00:
                printer_printf(&context->output_options, "COPY00 %i ",
                               context->tree[i++]); // displleft
                printer_printf(&context->output_options, "%i ",
                               context->tree[i++]); // displright
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY01:
                printer_printf(&context->output_options, "COPY01 %i ",
                               context->tree[i++]); // displleft
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY10:
                printer_printf(&context->output_options, "COPY10 %i ",
                               context->tree[i++]); // displright
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY11:
                printer_printf(&context->output_options, "COPY11 %i\n",
                               context->tree[i++]); // length
                break;
            case COPY0ST:
                printer_printf(&context->output_options, "COPY0ST %i ",
                               context->tree[i++]); // displleft
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY1ST:
                printer_printf(&context->output_options, "COPY1ST (%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY0STASS:
                printer_printf(&context->output_options, "COPY0STASS %i ",
                               context->tree[i++]); // displleft
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length
                break;
            case COPY1STASS:
                printer_printf(&context->output_options, "COPY1STASS (%i)\n",
                               context->tree[i++]); // length
                break;
            case COPYST:
                printer_printf(&context->output_options, "COPYST %i ",
                               context->tree[i++]); // displ
                printer_printf(&context->output_options, "(%i)",
                               context->tree[i++]); // length
                printer_printf(&context->output_options, "(%i)\n",
                               context->tree[i++]); // length1
                break;

            case TCall1:
                printer_printf(&context->output_options, "TCall1 %i\n",
                               context->tree[i++]);
                break;
            case TCall2:
                printer_printf(&context->output_options, "TCall2 %i\n",
                               context->tree[i++]);
                break;
            case TLabel:
                printer_printf(&context->output_options, "TLabel %i\n",
                               context->tree[i++]);
                break;
            case TStructbeg:
                printer_printf(&context->output_options, "TStructbeg %i\n",
                               context->tree[i++]);
                break;
            case TStructend:
                printer_printf(&context->output_options, "TStructend %i\n",
                               context->tree[i++]);
                break;
            case TPrint:
                printer_printf(&context->output_options, "TPrint %i\n",
                               context->tree[i++]);
                break;
            case TPrintid:
                printer_printf(&context->output_options, "TPrintid %i\n",
                               context->tree[i++]);
                break;
            case TPrintf:
                printer_printf(&context->output_options, "TPrintf %i\n",
                               context->tree[i++]);
                break;
            case TGetid:
                printer_printf(&context->output_options, "TGetid %i\n",
                               context->tree[i++]);
                break;
            case SETMOTORC:
                printer_printf(&context->output_options, "Setmotor\n");
                break;
            case CREATEC:
                printer_printf(&context->output_options, "TCREATE\n");
                break;
            case CREATEDIRECTC:
                printer_printf(&context->output_options, "TCREATEDIRECT\n");
                break;
            case EXITC:
                printer_printf(&context->output_options, "TEXIT\n");
                break;
            case EXITDIRECTC:
                printer_printf(&context->output_options, "TEXITDIRECT\n");
                break;
            case MSGSENDC:
                printer_printf(&context->output_options, "TMSGSEND\n");
                break;
            case MSGRECEIVEC:
                printer_printf(&context->output_options, "TMSGRECEIVE\n");
                break;
            case JOINC:
                printer_printf(&context->output_options, "TJOIN\n");
                break;
            case SLEEPC:
                printer_printf(&context->output_options, "TSLEEP\n");
                break;
            case SEMCREATEC:
                printer_printf(&context->output_options, "TSEMCREATE\n");
                break;
            case SEMWAITC:
                printer_printf(&context->output_options, "TSEMWAIT\n");
                break;
            case SEMPOSTC:
                printer_printf(&context->output_options, "TSEMPOST\n");
                break;
            case INITC:
                printer_printf(&context->output_options, "INITC\n");
                break;
            case DESTROYC:
                printer_printf(&context->output_options, "DESTROYC\n");
                break;
            case GETNUMC:
                printer_printf(&context->output_options, "GETNUMC\n");
                break;


            default:
                printer_printf(&context->output_options, "TOper %i\n",
                               context->tree[i - 1]);
        }
    }
}

void
tablesandcode(compiler_context *context)
{
    int i = 0, j;

    printer_printf(&context->output_options, "\n%s\n", "source");
    for (i = 1; i < context->line; i++)
    {
        printer_printf(&context->output_options, "line %i) ", i);
        for (j = context->lines[i]; j < context->lines[i + 1]; j++)
        {
            printer_printchar(&context->output_options, context->source[j]);
        }
    }

    printer_printf(&context->output_options, "\n\n%s\n", "functions");
    for (i = 1; i <= context->funcnum; i++)
        printer_printf(&context->output_options, "fun %i) %i\n", i,
                       context->functions[i]);

    printer_printf(&context->output_options, "\n%s\n", "iniprocs");
    for (i = 1; i <= context->procd; i++)
        printer_printf(&context->output_options, "inipr %i) %i\n", i,
                       context->iniprocs[i]);

    printer_printf(&context->output_options, "\n%s\n", "mem");
    i = 0;
    while (i < context->pc)
    {
        printer_printf(&context->output_options, "pc %i) ", i);
        switch (context->mem[i++])
        {
            case PRINT:
                printer_printf(&context->output_options, "PRINT %i\n",
                               context->mem[i++]);
                break;
            case PRINTID:
                printer_printf(&context->output_options, "PRINTID %i\n",
                               context->mem[i++]);
                break;
            case PRINTF:
                printer_printf(&context->output_options, "PRINTF %i\n",
                               context->mem[i++]);
                break;
            case GETID:
                printer_printf(&context->output_options, "GETID %i\n",
                               context->mem[i++]);
                break;
            case SETMOTORC:
                printer_printf(&context->output_options, "SETMOTOR\n");
                break;
            case GETDIGSENSORC:
                printer_printf(&context->output_options, "GETDIGSENSOR\n");
                break;
            case GETANSENSORC:
                printer_printf(&context->output_options, "GETANSENSOR\n");
                break;
            case VOLTAGEC:
                printer_printf(&context->output_options, "VOLTAGE\n");
                break;
            case CREATEC:
                printer_printf(&context->output_options, "TCREATE\n");
                break;
            case CREATEDIRECTC:
                printer_printf(&context->output_options, "TCREATEDIRECT\n");
                break;
            case MSGSENDC:
                printer_printf(&context->output_options, "TMSGSEND\n");
                break;
            case EXITC:
                printer_printf(&context->output_options, "TEXIT\n");
                break;
            case EXITDIRECTC:
                printer_printf(&context->output_options, "TEXITDIRECT\n");
                break;
            case MSGRECEIVEC:
                printer_printf(&context->output_options, "TMSGRECEIVE\n");
                break;
            case JOINC:
                printer_printf(&context->output_options, "TJOIN\n");
                break;
            case SLEEPC:
                printer_printf(&context->output_options, "TSLEEP\n");
                break;
            case SEMCREATEC:
                printer_printf(&context->output_options, "TSEMCREATE\n");
                break;
            case SEMWAITC:
                printer_printf(&context->output_options, "TSEMWAIT\n");
                break;
            case SEMPOSTC:
                printer_printf(&context->output_options, "TSEMPOST\n");
                break;
            case TINIT:
                printer_printf(&context->output_options, "TINIT\n");
                break;
            case TDESTROY:
                printer_printf(&context->output_options, "TDESTROY\n");
                break;
            case GETNUMC:
                printer_printf(&context->output_options, "GETNUM\n");
                break;

            case ABSC:
                printer_printf(&context->output_options, "ABS\n");
                break;
            case ABSIC:
                printer_printf(&context->output_options, "ABSI\n");
                break;
            case SQRTC:
                printer_printf(&context->output_options, "SQRT\n");
                break;
            case EXPC:
                printer_printf(&context->output_options, "EXP\n");
                break;
            case SINC:
                printer_printf(&context->output_options, "SIN\n");
                break;
            case COSC:
                printer_printf(&context->output_options, "COS\n");
                break;
            case LOGC:
                printer_printf(&context->output_options, "LOG\n");
                break;
            case LOG10C:
                printer_printf(&context->output_options, "LOG10\n");
                break;
            case ASINC:
                printer_printf(&context->output_options, "ASIN\n");
                break;
            case RANDC:
                printer_printf(&context->output_options, "RAND\n");
                break;
            case ROUNDC:
                printer_printf(&context->output_options, "ROUND\n");
                break;

            case STRCPYC:
                printer_printf(&context->output_options, "STRCPY\n");
                break;
            case STRNCPYC:
                printer_printf(&context->output_options, "STRNCPY\n");
                break;
            case STRCATC:
                printer_printf(&context->output_options, "STRCAT\n");
                break;
            case STRNCATC:
                printer_printf(&context->output_options, "STRNCAT\n");
                break;
            case STRCMPC:
                printer_printf(&context->output_options, "STRCMP\n");
                break;
            case STRNCMPC:
                printer_printf(&context->output_options, "STRNCMP\n");
                break;
            case STRSTRC:
                printer_printf(&context->output_options, "STRSTR\n");
                break;
            case STRLENC:
                printer_printf(&context->output_options, "STRLENC\n");
                break;

            case BEGINIT:
                printer_printf(&context->output_options, "BEGINIT n= %i\n",
                               context->mem[i++]);
                break;
            case STRUCTWITHARR:
                printer_printf(&context->output_options,
                               "STRUCTWITHARR displ= %i ", context->mem[i++]);
                printer_printf(&context->output_options, "iniproc= %i\n",
                               context->mem[i++]);
                break;
            case DEFARR:
                printer_printf(&context->output_options, "DEFARR N= %i ",
                               context->mem[i++]); // N
                printer_printf(&context->output_options, "elem_len= %i ",
                               context->mem[i++]); // elem length
                printer_printf(&context->output_options, "displ= %i ",
                               context->mem[i++]); // displ
                printer_printf(&context->output_options, "iniproc= %i ",
                               context->mem[i++]); // iniproc
                printer_printf(&context->output_options, "usual= %i ",
                               context->mem[i++]); // usual
                printer_printf(&context->output_options, "all= %i ",
                               context->mem[i++]); // all
                printer_printf(&context->output_options, "instruct= %i\n",
                               context->mem[i++]); // instruct
                break;
            case ARRINIT:
                printer_printf(&context->output_options, "ARRINIT N= %i ",
                               context->mem[i++]);
                printer_printf(&context->output_options, "elem_len= %i ",
                               context->mem[i++]);
                printer_printf(&context->output_options, "displ= %i ",
                               context->mem[i++]);
                printer_printf(&context->output_options, "usual= %i\n",
                               context->mem[i++]);
                break;
                //            case STRUCTINIT:
                //                printer_printf(&context->output_options,
                //                "STRUCTINIT N= %i ", context->mem[i++]);
                //            break;
            case NOP:
                printer_printf(&context->output_options, "NOP\n");
                break;
            case LI:
                printer_printf(&context->output_options, "LI %i\n",
                               context->mem[i++]);
                break;
            case LID:
                memcpy(&context->numdouble, &context->mem[i], sizeof(double));
                i += 2;
                printer_printf(&context->output_options, "LID %.15f\n",
                               context->numdouble);
                break;
            case LOAD:
                printer_printf(&context->output_options, "LOAD %i\n",
                               context->mem[i++]);
                break;
            case LOADD:
                printer_printf(&context->output_options, "LOADD %i\n",
                               context->mem[i++]);
                break;
            case LAT:
                printer_printf(&context->output_options, "L@\n");
                break;
            case LATD:
                printer_printf(&context->output_options, "L@f\n");
                break;
            case LA:
                printer_printf(&context->output_options, "LA %i\n",
                               context->mem[i++]);
                break;

            case LOGOR:
                printer_printf(&context->output_options, "||\n");
                break;
            case LOGAND:
                printer_printf(&context->output_options, "&&\n");
                break;
            case ORASS:
                printer_printf(&context->output_options, "|= %i\n",
                               context->mem[i++]);
                break;
            case ORASSAT:
                printer_printf(&context->output_options, "|=@\n");
                break;
            case ORASSV:
                printer_printf(&context->output_options, "|=V %i\n",
                               context->mem[i++]);
                break;
            case ORASSATV:
                printer_printf(&context->output_options, "|=@V\n");
                break;
            case LOR:
                printer_printf(&context->output_options, "|\n");
                break;
            case EXORASS:
                printer_printf(&context->output_options, "^= %i\n",
                               context->mem[i++]);
                break;
            case EXORASSAT:
                printer_printf(&context->output_options, "^=@\n");
                break;
            case EXORASSV:
                printer_printf(&context->output_options, "^=V %i\n",
                               context->mem[i++]);
                break;
            case EXORASSATV:
                printer_printf(&context->output_options, "^=@V\n");
                break;
            case LEXOR:
                printer_printf(&context->output_options, "^\n");
                break;
            case ANDASS:
                printer_printf(&context->output_options, "&= %i\n",
                               context->mem[i++]);
                break;
            case ANDASSAT:
                printer_printf(&context->output_options, "&=@\n");
                break;
            case ANDASSV:
                printer_printf(&context->output_options, "&=V %i\n",
                               context->mem[i++]);
                break;
            case ANDASSATV:
                printer_printf(&context->output_options, "&=@V\n");
                break;
            case LAND:
                printer_printf(&context->output_options, "&\n");
                break;

            case EQEQ:
                printer_printf(&context->output_options, "==\n");
                break;
            case NOTEQ:
                printer_printf(&context->output_options, "!=\n");
                break;
            case LLT:
                printer_printf(&context->output_options, "<\n");
                break;
            case LGT:
                printer_printf(&context->output_options, ">\n");
                break;
            case LLE:
                printer_printf(&context->output_options, "<=\n");
                break;
            case LGE:
                printer_printf(&context->output_options, ">=\n");
                break;
            case EQEQR:
                printer_printf(&context->output_options, "==f\n");
                break;
            case NOTEQR:
                printer_printf(&context->output_options, "!=f\n");
                break;
            case LLTR:
                printer_printf(&context->output_options, "<f\n");
                break;
            case LGTR:
                printer_printf(&context->output_options, ">f\n");
                break;
            case LLER:
                printer_printf(&context->output_options, "<=f\n");
                break;
            case LGER:
                printer_printf(&context->output_options, ">=f\n");
                break;

            case SHRASS:
                printer_printf(&context->output_options, ">>= %i\n",
                               context->mem[i++]);
                break;
            case SHRASSAT:
                printer_printf(&context->output_options, ">>=@\n");
                break;
            case SHRASSV:
                printer_printf(&context->output_options, ">>=V %i\n",
                               context->mem[i++]);
                break;
            case SHRASSATV:
                printer_printf(&context->output_options, ">>=@V\n");
                break;
            case LSHR:
                printer_printf(&context->output_options, ">>\n");
                break;
            case SHLASS:
                printer_printf(&context->output_options, "<<= %i\n",
                               context->mem[i++]);
                break;
            case SHLASSAT:
                printer_printf(&context->output_options, "<<=@\n");
                break;
            case SHLASSV:
                printer_printf(&context->output_options, "<<=V %i\n",
                               context->mem[i++]);
                break;
            case SHLASSATV:
                printer_printf(&context->output_options, "<<=@V\n");
                break;
            case LSHL:
                printer_printf(&context->output_options, "<<\n");
                break;

            case ASS:
                printer_printf(&context->output_options, "= %i\n",
                               context->mem[i++]);
                break;
            case ASSAT:
                printer_printf(&context->output_options, "=@\n");
                break;
            case ASSV:
                printer_printf(&context->output_options, "=V %i\n",
                               context->mem[i++]);
                break;
            case ASSATV:
                printer_printf(&context->output_options, "=@V\n");
                break;

            case PLUSASS:
                printer_printf(&context->output_options, "+= %i\n",
                               context->mem[i++]);
                break;
            case PLUSASSAT:
                printer_printf(&context->output_options, "+=@\n");
                break;
            case PLUSASSV:
                printer_printf(&context->output_options, "+=V %i\n",
                               context->mem[i++]);
                break;
            case PLUSASSATV:
                printer_printf(&context->output_options, "+=@V\n");
                break;
            case LPLUS:
                printer_printf(&context->output_options, "+\n");
                break;

            case MINUSASS:
                printer_printf(&context->output_options, "-= %i\n",
                               context->mem[i++]);
                break;
            case MINUSASSAT:
                printer_printf(&context->output_options, "-=@\n");
                break;
            case MINUSASSV:
                printer_printf(&context->output_options, "-=V %i\n",
                               context->mem[i++]);
                break;
            case MINUSASSATV:
                printer_printf(&context->output_options, "-=@V\n");
                break;
            case LMINUS:
                printer_printf(&context->output_options, "-\n");
                break;

            case MULTASS:
                printer_printf(&context->output_options, "*= %i\n",
                               context->mem[i++]);
                break;
            case MULTASSAT:
                printer_printf(&context->output_options, "*=@\n");
                break;
            case MULTASSV:
                printer_printf(&context->output_options, "*=V %i\n",
                               context->mem[i++]);
                break;
            case MULTASSATV:
                printer_printf(&context->output_options, "*=@V\n");
                break;
            case LMULT:
                printer_printf(&context->output_options, "*\n");
                break;

            case DIVASS:
                printer_printf(&context->output_options, "/= %i\n",
                               context->mem[i++]);
                break;
            case DIVASSAT:
                printer_printf(&context->output_options, "/=@\n");
                break;
            case DIVASSV:
                printer_printf(&context->output_options, "/=V %i\n",
                               context->mem[i++]);
                break;
            case DIVASSATV:
                printer_printf(&context->output_options, "/=@V\n");
                break;
            case LDIV:
                printer_printf(&context->output_options, "/\n");
                break;

            case ASSR:
                printer_printf(&context->output_options, "=f %i\n",
                               context->mem[i++]);
                break;
            case ASSRV:
                printer_printf(&context->output_options, "=fV %i\n",
                               context->mem[i++]);
                break;
            case ASSATR:
                printer_printf(&context->output_options, "=@f\n");
                break;
            case ASSATRV:
                printer_printf(&context->output_options, "=@fV\n");
                break;

            case PLUSASSR:
                printer_printf(&context->output_options, "+=f %i\n",
                               context->mem[i++]);
                break;
            case PLUSASSATR:
                printer_printf(&context->output_options, "+=@f\n");
                break;
            case PLUSASSRV:
                printer_printf(&context->output_options, "+=fV %i\n",
                               context->mem[i++]);
                break;
            case PLUSASSATRV:
                printer_printf(&context->output_options, "+=@fV\n");
                break;
            case LPLUSR:
                printer_printf(&context->output_options, "+f\n");
                break;
            case MINUSASSR:
                printer_printf(&context->output_options, "-=f %i\n",
                               context->mem[i++]);
                break;
            case MINUSASSATR:
                printer_printf(&context->output_options, "-=@f\n");
                break;
            case MINUSASSRV:
                printer_printf(&context->output_options, "-=fV %i\n",
                               context->mem[i++]);
                break;
            case MINUSASSATRV:
                printer_printf(&context->output_options, "-=@fV\n");
                break;
            case LMINUSR:
                printer_printf(&context->output_options, "-f\n");
                break;
            case MULTASSR:
                printer_printf(&context->output_options, "*=f %i\n",
                               context->mem[i++]);
                break;
            case MULTASSATR:
                printer_printf(&context->output_options, "*=@f\n");
                break;
            case MULTASSRV:
                printer_printf(&context->output_options, "*=fV %i\n",
                               context->mem[i++]);
                break;
            case MULTASSATRV:
                printer_printf(&context->output_options, "*=@fV\n");
                break;
            case LMULTR:
                printer_printf(&context->output_options, "*f\n");
                break;
            case DIVASSR:
                printer_printf(&context->output_options, "/=f %i\n",
                               context->mem[i++]);
                break;
            case DIVASSATR:
                printer_printf(&context->output_options, "/=@f\n");
                break;
            case DIVASSRV:
                printer_printf(&context->output_options, "/=fV %i\n",
                               context->mem[i++]);
                break;
            case DIVASSATRV:
                printer_printf(&context->output_options, "/=@fV\n");
                break;
            case LDIVR:
                printer_printf(&context->output_options, "/f\n");
                break;
            case COPY00:
                printer_printf(&context->output_options, "COPY00 %i ",
                               context->mem[i++]); // displleft
                printer_printf(&context->output_options, "%i ",
                               context->mem[i++]); // displright
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length
                break;
            case COPY01:
                printer_printf(&context->output_options, "COPY01 %i      ",
                               context->mem[i++]); // displleft
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length
                break;
            case COPY10:
                printer_printf(&context->output_options, "COPY10      %i ",
                               context->mem[i++]); // displright
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length
                break;
            case COPY11:
                printer_printf(&context->output_options, "COPY11 %i\n",
                               context->mem[i++]); // length
                break;
            case COPY0ST:
                printer_printf(&context->output_options, "COPY0ST %i ",
                               context->mem[i++]); // displright
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length
                break;
            case COPY1ST:
                printer_printf(&context->output_options, "COPY1ST %i\n",
                               context->mem[i++]); // length
                break;
            case COPY0STASS:
                printer_printf(&context->output_options, "COPY0STASS %i ",
                               context->mem[i++]); // displleft
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length
                break;
            case COPY1STASS:
                printer_printf(&context->output_options, "COPY1STASS %i\n",
                               context->mem[i++]); // length
                break;
            case COPYST:
                printer_printf(&context->output_options, "COPYST %i ",
                               context->mem[i++]); // displ
                printer_printf(&context->output_options, "(%i)",
                               context->mem[i++]); // length
                printer_printf(&context->output_options, "(%i)\n",
                               context->mem[i++]); // length1
                break;

            case REMASS:
                printer_printf(&context->output_options, "%%= %i\n",
                               context->mem[i++]);
                break;
            case REMASSAT:
                printer_printf(&context->output_options, "%%=@\n");
                break;
            case REMASSV:
                printer_printf(&context->output_options, "%%=V %i\n",
                               context->mem[i++]);
                break;
            case REMASSATV:
                printer_printf(&context->output_options, "%%=@V\n");
                break;
            case LREM:
                printer_printf(&context->output_options, "%%\n");
                break;

            case CALL1:
                printer_printf(&context->output_options, "CALL1\n");
                break;
            case CALL2:
                printer_printf(&context->output_options, "CALL2 ");
                printer_printf(&context->output_options, "%i\n",
                               context->mem[i++]);
                break;
            case STOP:
                printer_printf(&context->output_options, "STOP\n");
                break;
            case RETURNVAL:
                printer_printf(&context->output_options, "RETURNVAL %i\n",
                               context->mem[i++]);
                break;
            case RETURNVOID:
                printer_printf(&context->output_options, "RETURNVOID\n");
                break;
            case B:
                printer_printf(&context->output_options, "B %i\n",
                               context->mem[i++]);
                break;
            case BE0:
                printer_printf(&context->output_options, "BE0 %i\n",
                               context->mem[i++]);
                break;
            case BNE0:
                printer_printf(&context->output_options, "BNE0 %i\n",
                               context->mem[i++]);
                break;
            case SLICE:
                printer_printf(&context->output_options, "SLICE d= %i\n",
                               context->mem[i++]);
                break;
            case SELECT:
                printer_printf(&context->output_options,
                               "SELECT field_displ= %i\n", context->mem[i++]);
                break;
            case WIDEN:
                printer_printf(&context->output_options, "WIDEN\n");
                break;
            case WIDEN1:
                printer_printf(&context->output_options, "WIDEN1\n");
                break;
            case _DOUBLE:
                printer_printf(&context->output_options, "DOUBLE\n");
                break;
            case INC:
                printer_printf(&context->output_options, "INC %i\n",
                               context->mem[i++]);
                break;
            case DEC:
                printer_printf(&context->output_options, "DEC %i\n",
                               context->mem[i++]);
                break;
            case POSTINC:
                printer_printf(&context->output_options, "POSTINC %i\n",
                               context->mem[i++]);
                break;
            case POSTDEC:
                printer_printf(&context->output_options, "POSTDEC %i\n",
                               context->mem[i++]);
                break;
            case INCAT:
                printer_printf(&context->output_options, "INC@\n");
                break;
            case DECAT:
                printer_printf(&context->output_options, "DEC@\n");
                break;
            case POSTINCAT:
                printer_printf(&context->output_options, "POSTINC@\n");
                break;
            case POSTDECAT:
                printer_printf(&context->output_options, "POSTDEC@\n");
                break;
            case INCR:
                printer_printf(&context->output_options, "INCf %i\n",
                               context->mem[i++]);
                break;
            case DECR:
                printer_printf(&context->output_options, "DECf %i\n",
                               context->mem[i++]);
                break;
            case POSTINCR:
                printer_printf(&context->output_options, "POSTINCf %i\n",
                               context->mem[i++]);
                break;
            case POSTDECR:
                printer_printf(&context->output_options, "POSTDECf %i\n",
                               context->mem[i++]);
                break;
            case INCATR:
                printer_printf(&context->output_options, "INC@f\n");
                break;
            case DECATR:
                printer_printf(&context->output_options, "DEC@f\n");
                break;
            case POSTINCATR:
                printer_printf(&context->output_options, "POSTINC@f\n");
                break;
            case POSTDECATR:
                printer_printf(&context->output_options, "POSTDEC@f\n");
                break;
            case INCV:
                printer_printf(&context->output_options, "INCV %i\n",
                               context->mem[i++]);
                break;
            case DECV:
                printer_printf(&context->output_options, "DECV %i\n",
                               context->mem[i++]);
                break;
            case POSTINCV:
                printer_printf(&context->output_options, "POSTINCV %i\n",
                               context->mem[i++]);
                break;
            case POSTDECV:
                printer_printf(&context->output_options, "POSTDECV %i\n",
                               context->mem[i++]);
                break;
            case INCATV:
                printer_printf(&context->output_options, "INC@V\n");
                break;
            case DECATV:
                printer_printf(&context->output_options, "DEC@V\n");
                break;
            case POSTINCATV:
                printer_printf(&context->output_options, "POSTINC@V\n");
                break;
            case POSTDECATV:
                printer_printf(&context->output_options, "POSTDEC@V\n");
                break;
            case INCRV:
                printer_printf(&context->output_options, "INCfV %i\n",
                               context->mem[i++]);
                break;
            case DECRV:
                printer_printf(&context->output_options, "DECfV %i\n",
                               context->mem[i++]);
                break;
            case POSTINCRV:
                printer_printf(&context->output_options, "POSTINCfV %i\n",
                               context->mem[i++]);
                break;
            case POSTDECRV:
                printer_printf(&context->output_options, "POSTDECfV %i\n",
                               context->mem[i++]);
                break;
            case INCATRV:
                printer_printf(&context->output_options, "INC@fV\n");
                break;
            case DECATRV:
                printer_printf(&context->output_options, "DEC@fV\n");
                break;
            case POSTINCATRV:
                printer_printf(&context->output_options, "POSTINC@fV\n");
                break;
            case POSTDECATRV:
                printer_printf(&context->output_options, "POSTDEC@fV\n");
                break;

            case LNOT:
                printer_printf(&context->output_options, "BITNOT\n");
                break;
            case LOGNOT:
                printer_printf(&context->output_options, "NOT\n");
                break;
            case UNMINUS:
                printer_printf(&context->output_options, "UNMINUS\n");
                break;
            case UNMINUSR:
                printer_printf(&context->output_options, "UNMINUSf\n");
                break;

            case FUNCBEG:
                printer_printf(&context->output_options,
                               "FUNCBEG maxdispl= %i ", context->mem[i++]);
                printer_printf(&context->output_options, "pc= %i\n",
                               context->mem[i++]);
                break;


            default:
                printer_printf(&context->output_options, "%i\n",
                               context->mem[i - 1]);
        }
    }
}
