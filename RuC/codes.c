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

extern void fprintf_char(FILE *f, int wchar);

void tablesandtree()
{
    int i=0, j;
    
    fprintf(output, "\n%s\n", "source");
    for (i=1; i<line; i++)
    {
        fprintf(output, "line %i) ", i);
        for (j=lines[i]; j<lines[i+1]; j++)
        {
            fprintf_char(output, source[j]);
        }
    }
    fprintf(output, "\n");
    
    fprintf(output, "\n%s\n", "identab");
    i = 2;
    while (i < id)
    {
        for (j=0; j<4; j++)
            fprintf(output, "id %i) %i\n", i +j, identab[i+j]);
        fprintf(output, "\n");
        i +=4;
    }
/*
	fprintf(output, "\n%s\n", "repr");
	for (i = 1206; i <= rp; i++)
		fprintf(output, "rp %i) %i\n", i, reprtab[i]);
 */
    fprintf(output, "\n%s\n", "modetab");
    for (i=0; i<md; i++)
        fprintf(output, "md %i) %i\n", i, modetab[i]);
/*
    fprintf(output, "\n%s\n", "tree");
    for (i=0; i<=tc; i++)
        fprintf(output, "tc %i) %i\n", i, tree[i]);
*/
    fprintf(output, "\n");
    i = 0;
    while (i < tc)
    {
        fprintf(output, "tc %i) ", i);
        switch (tree[i++])
        {
            case TFuncdef:
                fprintf(output, "TFuncdef %i %i\n", tree[i], tree[i+1]);
				i += 2;
                break;
            case TDeclarr:
                fprintf(output, "TDeclarr N= %i\n", tree[i++]);
                break;
            case TDeclid:
                fprintf(output, "TDeclid displ= %i elem_type= %i N= %i all= %i iniproc= %i\n",
                        tree[i], tree[i+1], tree[i+2], tree[i+3], tree[i+4]);
				i += 5;
                break;
            case TCondexpr:
                fprintf(output, "TCondexpr %i %i\n", tree[i], tree[i+1]);
				i += 2;
                break;
            case TBegin:
                fprintf(output, "TBegin\n");
                break;
            case TEnd:
                fprintf(output, "TEnd\n");
                break;
            case TIf:
                fprintf(output, "TIf %i %i\n", tree[i], tree[i+1]);
				i += 2;
                break;
            case TWhile:
                fprintf(output, "TWhile %i\n", tree[i++]);
                break;
            case TDo:
                fprintf(output, "TDo %i\n", tree[i++]);
                break;
            case TFor:
                fprintf(output, "TFor %i %i %i %i\n", tree[i], tree[i+1], tree[i+2], tree[i+3]);
				i += 4;
                break;
            case TSwitch:
                fprintf(output, "TSwitch\n");
                break;
            case TCase:
                fprintf(output, "TCase\n");
                break;
            case TDefault:
                fprintf(output, "TDefault\n");
                break;
            case TBreak:
                fprintf(output, "TBreak\n");
                break;
            case TContinue:
                fprintf(output, "TContinue\n");
                break;
            case TReturn:
                fprintf(output, "TReturn\n");
                break;
            case TReturnval:
                fprintf(output, "TReturnval %i\n", tree[i++]);
                break;
            case TGoto:
                fprintf(output, "TGoto %i\n", tree[i++]);
                break;
            case TIdent:
                fprintf(output, "TIdent %i\n", tree[i++]);
                break;
            case TIdenttoval:
                fprintf(output, "TIdenttoval %i\n", tree[i++]);
                break;
                case TFunidtoval:
                fprintf(output, "TFunidtoval %i\n", tree[i++]);
                break;
            case TIdenttoaddr:
                fprintf(output, "TIdenttoaddr %i\n", tree[i++]);
                break;
            case TAddrtoval:
                fprintf(output, "TAddrtoval\n");
                break;
            case TExprend:
                fprintf(output, "TExprend\n");
                break;
            case TConst:
            {
                int t = tree[i++];
                memcpy(&numfloat, &t, 4);
                fprintf(output, "TConst %i %f\n", t, numfloat);
            }
                break;
            case TSliceident:
                fprintf(output, "TSliceident displ= %i ", tree[i++]);
                fprintf(output, "d= %i\n", tree[i++]);
                break;
            case TSlice:
                fprintf(output, "TSlice d= %i\n", tree[i++]);
                break;
			case TSelect:
				fprintf(output, "TSelect %i\n", tree[i++]);
				break;
                case NOP:
                fprintf(output, "NOP\n");
                break;
            case COPY00:
                fprintf(output, "COPY00 %i ", tree[i++]);     // displleft
                fprintf(output, "%i ", tree[i++]);            // displright
                fprintf(output, "(%i)\n", tree[i++]);         // length
                break;
            case COPY01:
                fprintf(output, "COPY01 %i ", tree[i++]);     // displleft
                fprintf(output, "(%i)\n", tree[i++]);         // length
                break;
            case COPY10:
                fprintf(output, "COPY10 %i ", tree[i++]);     // displright
                fprintf(output, "(%i)\n", tree[i++]);         // length
                break;
            case COPY11:
                fprintf(output, "COPY11 %i\n", tree[i++]);     // length
                break;
            case COPY0ST:
                fprintf(output, "COPY0ST %i ", tree[i++]);     // displleft
                fprintf(output, "(%i)\n", tree[i++]);          // length
                break;
            case COPY1ST:
                fprintf(output, "COPY1ST (%i)\n", tree[i++]);  // length
                break;
            case COPY0STASS:
                fprintf(output, "COPY0STASS %i ", tree[i++]);  // displleft
                fprintf(output, "(%i)\n", tree[i++]);          // length
                break;
            case COPY1STASS:
                fprintf(output, "COPY1STASS (%i)\n", tree[i++]);// length
                break;

            case TCall1:
                fprintf(output, "TCall1 %i\n", tree[i++]);
                break;
            case TCall2:
                fprintf(output, "TCall2 %i\n", tree[i++]);
                break;
            case TLabel:
                fprintf(output, "TLabel %i\n", tree[i++]);
                break;
            case TStructbeg:
                fprintf(output, "TStructbeg %i\n", tree[i++]);
                break;
            case TStructend:
                fprintf(output, "TStructend %i\n", tree[i++]);
                break;
            case TPrint:
                fprintf(output, "TPrint %i\n", tree[i++]);
                break;
            case TPrintid:
                fprintf(output, "TPrintid %i\n", tree[i++]);
                break;
            case TGetid:
                fprintf(output, "TGetid %i\n", tree[i++]);
                break;
            case SETMOTOR:
                fprintf(output, "Setmotor\n");
                break;
            case SLEEP:
                fprintf(output, "Sleep\n");

            default:
                fprintf(output, "TOper %i\n", tree[i-1]);
        }
    }
}

void tablesandcode()
{
    int i=0, j;
    
    fprintf(output, "\n%s\n", "source");
    for (i=1; i<line; i++)
    {
        fprintf(output, "line %i) ", i);
        for (j=lines[i]; j<lines[i+1]; j++)
        {
            fprintf_char(output, source[j]);
        }
    }
    
    fprintf(output, "\n\n%s\n", "functions");
    for (i=1; i<=funcnum; i++)
        fprintf(output, "fun %i) %i\n", i, functions[i]);
    
    fprintf(output, "\n%s\n", "iniprocs");
    for (i=1; i<=procd; i++)
        fprintf(output, "inipr %i) %i\n", i, iniprocs[i]);
    
    fprintf(output, "\n%s\n", "mem");
    i = 0;
    while (i < pc)
    {
        fprintf(output, "pc %i) ", i);
        switch (mem[i++])
        {
            case PRINT:
                fprintf(output, "PRINT ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case PRINTID:
                fprintf(output, "PRINTID ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case GETID:
                fprintf(output, "GETID ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SETMOTOR:
                fprintf(output, "SETMOTOR\n");
                break;
            case SLEEP:
                fprintf(output, "SLEEP\n");
                break;
            case GETDIGSENSORC:
                fprintf(output, "GETDIGSENSOR\n");
                break;
            case GETANSENSORC:
                fprintf(output, "GETANSENSOR\n");
                break;
            case ABSC:
                fprintf(output, "ABS\n");
                break;
            case ABSIC:
                fprintf(output, "ABSI\n");
                break;
            case SQRTC:
                fprintf(output, "SQRT\n");
                break;
            case EXPC:
                fprintf(output, "EXP\n");
                break;
            case SINC:
                fprintf(output, "SIN\n");
                break;
            case COSC:
                fprintf(output, "COS\n");
                break;
            case LOGC:
                fprintf(output, "LOG\n");
                break;
            case LOG10C:
                fprintf(output, "LOG10\n");
                break;
            case ASINC:
                fprintf(output, "ASIN\n");
                break;
            case RANDC:
                fprintf(output, "RAND\n");
                break;
            case ROUNDC:
                fprintf(output, "ROUND\n");
                break;
             
                case STRUCTWITHARR:
                fprintf(output, "STRUCTWITHARR displ= %i ", mem[i++]);
                fprintf(output, "iniproc= %i\n", mem[i++]);
                break;
            case DEFARR:
                fprintf(output, "DEFARR ");
                fprintf(output, "N= %i ", mem[i++]);           // N
				fprintf(output, "elem_len= %i ", mem[i++]);    // elem length
				fprintf(output, "displ= %i ", mem[i++]);       // displ
                fprintf(output, "iniproc= %i\n", mem[i++]);    // iniproc
                break;
            case NOP:
                fprintf(output, "NOP\n");
                break;
            case LI:
                fprintf(output, "LI ");
                memcpy(&numfloat, &mem[i], sizeof(int));
                fprintf(output, "%i %f\n", mem[i++], numfloat);
                break;
            case LOAD:
                fprintf(output, "LOAD ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case LAT:
                fprintf(output,"L@\n");
                break;
            case LA:
                fprintf(output, "LA ");
                fprintf(output, "%i\n", mem[i++]);
                break;
                
            case LOGOR:
                fprintf(output, "||\n");
                break;
            case LOGAND:
                fprintf(output, "&&\n");
                break;
            case ORASS:
                fprintf(output, "|= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ORASSAT:
                fprintf(output, "|=@\n");
                break;
            case ORASSV:
                fprintf(output, "|=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ORASSATV:
                fprintf(output, "|=@V\n");
                break;
            case LOR:
                fprintf(output, "|\n");
                break;
            case EXORASS:
                fprintf(output, "^= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case EXORASSAT:
                fprintf(output, "^=@\n");
                break;
            case EXORASSV:
                fprintf(output, "^=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case EXORASSATV:
                fprintf(output, "^=@V\n");
                break;
            case LEXOR:
                fprintf(output, "^\n");
                break;
            case ANDASS:
                fprintf(output, "&= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ANDASSAT:
                fprintf(output, "&=@\n");
                break;
            case ANDASSV:
                fprintf(output, "&=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ANDASSATV:
                fprintf(output, "&=@V\n");
                break;
            case LAND:
                fprintf(output, "&\n");
                break;
                
            case EQEQ:
                fprintf(output, "==\n");
                break;
            case NOTEQ:
                fprintf(output, "!=\n");
                break;
            case LLT:
                fprintf(output, "<\n");
                break;
            case LGT:
                fprintf(output, ">\n");
                break;
            case LLE:
                fprintf(output, "<=\n");
                break;
            case LGE:
                fprintf(output, ">=\n");
                break;
            case EQEQR:
                fprintf(output, "==f\n");
                break;
            case NOTEQR:
                fprintf(output, "!=f\n");
                break;
            case LLTR:
                fprintf(output, "<f\n");
                break;
            case LGTR:
                fprintf(output, ">f\n");
                break;
            case LLER:
                fprintf(output, "<=f\n");
                break;
            case LGER:
                fprintf(output, ">=f\n");
                break;
                
            case SHRASS:
                fprintf(output, ">>= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SHRASSAT:
                fprintf(output, ">>=@\n");
                break;
            case SHRASSV:
                fprintf(output, ">>=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SHRASSATV:
                fprintf(output, ">>=@V\n");
                break;
            case LSHR:
                fprintf(output, ">>\n");
                break;
            case SHLASS:
                fprintf(output, "<<= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SHLASSAT:
                fprintf(output, "<<=@\n");
                break;
            case SHLASSV:
                fprintf(output, "<<=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SHLASSATV:
                fprintf(output, "<<=@V\n");
                break;
            case LSHL:
                fprintf(output, "<<\n");
                break;
            case ASS:
                fprintf(output, "= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ASSAT:
                fprintf(output, "=@\n");
                break;
            case ASSV:
                fprintf(output, "=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case ASSATV:
                fprintf(output, "=@V\n");
                break;
             
            case PLUSASS:
                fprintf(output, "+= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case PLUSASSAT:
                fprintf(output, "+=@\n");
                break;
            case PLUSASSV:
                fprintf(output, "+=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case PLUSASSATV:
                fprintf(output, "+=@V\n");
                break;

            case LPLUS:
                fprintf(output, "+\n");
                break;
            case MINUSASS:
                fprintf(output, "-= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MINUSASSAT:
                fprintf(output, "-=@\n");
                break;
            case MINUSASSV:
                fprintf(output, "-=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MINUSASSATV:
                fprintf(output, "-=@V\n");
                break;

            case LMINUS:
                fprintf(output, "-\n");
                break;
            case MULTASS:
                fprintf(output, "*= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MULTASSAT:
                fprintf(output, "*=@\n");
                break;
            case MULTASSV:
                fprintf(output, "*=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MULTASSATV:
                fprintf(output, "*=@V\n");
                break;

            case LMULT:
                fprintf(output, "*\n");
                break;
            case DIVASS:
                fprintf(output, "/= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DIVASSAT:
                fprintf(output, "/=@\n");
                break;
            case DIVASSV:
                fprintf(output, "/=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DIVASSATV:
                fprintf(output, "/=@V\n");
                break;
            case LDIV:
                fprintf(output, "/\n");
                break;
                
            case PLUSASSR:
                fprintf(output, "+=f ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case PLUSASSATR:
                fprintf(output, "+=@f\n");
                break;
            case PLUSASSRV:
                fprintf(output, "+=fV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case PLUSASSATRV:
                fprintf(output, "+=@fV\n");
                break;
            case LPLUSR:
                fprintf(output, "+f\n");
                break;
            case MINUSASSR:
                fprintf(output, "-=f ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MINUSASSATR:
                fprintf(output, "-=@f\n");
                break;
            case MINUSASSRV:
                fprintf(output, "-=fV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MINUSASSATRV:
                fprintf(output, "-=@fV\n");
                break;
            case LMINUSR:
                fprintf(output, "-f\n");
                break;
            case MULTASSR:
                fprintf(output, "*=f ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MULTASSATR:
                fprintf(output, "*=@f\n");
                break;
            case MULTASSRV:
                fprintf(output, "*=fV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case MULTASSATRV:
                fprintf(output, "*=@fV\n");
                break;
            case LMULTR:
                fprintf(output, "*f\n");
                break;
            case DIVASSR:
                fprintf(output, "/=f ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DIVASSATR:
                fprintf(output, "/=@f\n");
                break;
            case DIVASSRV:
                fprintf(output, "/=fV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DIVASSATRV:
                fprintf(output, "/=@fV\n");
                break;
            case LDIVR:
                fprintf(output, "/f\n");
                break;
			case COPY00:
				fprintf(output, "COPY00 %i ", mem[i++]);          // displleft
				fprintf(output, "%i ", mem[i++]);                 // displright
				fprintf(output, "(%i)\n", mem[i++]);              // длина структуры
				break;
			case COPY01:
				fprintf(output, "COPY01 %i      ", mem[i++]);     // displleft
				fprintf(output, "(%i)\n", mem[i++]);              // длина структуры
				break;
            case COPY10:
                fprintf(output, "COPY10      %i ", mem[i++]);     // displright
                fprintf(output, "(%i)\n", mem[i++]);              // длина структуры
                break;
            case COPY11:
                fprintf(output, "COPY11 %i\n", mem[i++]);         // длина структуры
                break;
            case COPY0ST:
                fprintf(output, "COPY0ST %i ", mem[i++]);         // displright
                fprintf(output, "(%i)\n", mem[i++]);              // длина структуры
                break;
            case COPY1ST:
                fprintf(output, "COPY1ST %i\n", mem[i++]);        // длина структуры
                break;
            case COPY0STASS:
                fprintf(output, "COPY0STASS %i ", mem[i++]);      // displleft
                fprintf(output, "(%i)\n", mem[i++]);              // длина структуры
                break;
            case COPY1STASS:
                fprintf(output, "COPY1STASS %i\n", mem[i++]);     // длина структуры
                break;


            case REMASS:
                fprintf(output, "%%= ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case REMASSAT:
                fprintf(output, "%%=@\n");
                break;
            case REMASSV:
                fprintf(output, "%%=V ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case REMASSATV:
                fprintf(output, "%%=@V\n");
                break;
            case LREM:
                fprintf(output, "%%\n");
                break;
                
            case CALL1:
                fprintf(output, "CALL1\n");
                break;
            case CALL2:
                fprintf(output, "CALL2 ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case STOP:
                fprintf(output, "STOP\n");
                break;
            case _RETURN:
                fprintf(output, "RETURN ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case RETURNV:
                fprintf(output, "RETURNV\n");
                break;
            case B:
                fprintf(output, "B ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case STRING:
            {
                int j, n;
                fprintf(output,"STRING ");
                fprintf(output, "%i\n", mem[i++]);
                fprintf(output, "n=%i\n", n = mem[i++]);
                for (j=0; j<n; j++)
                    fprintf(output, "%c\n", mem[i++]);
                fprintf(output,"%i\n", mem[i++]);
                break;
            }
                break;
            case BE0:
                fprintf(output, "BE0 ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case BNE0:
                fprintf(output, "BNE0 ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case SLICE:
                fprintf(output, "SLICE d= %i\n", mem[i++]);
                break;
			case SELECT:
				fprintf(output, "SELECT field_displ= %i\n", mem[i++]);
				break;
            case ARRINIT:
                fprintf(output, "ARRINIT N= %i ", mem[i++]);
                fprintf(output, "d= %i ", mem[i++]);
                fprintf(output, "all= %i ", mem[i++]);
                fprintf(output, "displ= %i\n", mem[i++]);
                break;
            case STRUCTINIT:
                fprintf(output, "STRUCTINIT all= %i displ= %i\n", mem[i], mem[i+1]);
				i += 2;
                break;
            case WIDEN:
                fprintf(output, "WIDEN\n");
                break;
            case WIDEN1:
                fprintf(output, "WIDEN1\n");
                break;
            case _DOUBLE:
                fprintf(output, "DOUBLE\n");
                break;
            case INC:
                fprintf(output, "INC ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DEC:
                fprintf(output, "DEC ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTINC:
                fprintf(output, "POSTINC ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTDEC:
                fprintf(output, "POSTDEC ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case INCAT:
                fprintf(output, "INC@\n");
                break;
            case DECAT:
                fprintf(output, "DEC@\n");
                break;
            case POSTINCAT:
                fprintf(output, "POSTINC@\n");
                break;
            case POSTDECAT:
                fprintf(output, "POSTDEC@\n");
                break;
            case INCR:
                fprintf(output, "INCf ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DECR:
                fprintf(output, "DECf ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTINCR:
                fprintf(output, "POSTINCf ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTDECR:
                fprintf(output, "POSTDECf ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case INCATR:
                fprintf(output, "INC@f\n");
                break;
            case DECATR:
                fprintf(output, "DEC@f\n");
                break;
            case POSTINCATR:
                fprintf(output, "POSTINC@f\n");
                break;
            case POSTDECATR:
                fprintf(output, "POSTDEC@f\n");
                break;
            case INCV:
                fprintf(output, "INCV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DECV:
                fprintf(output, "DECV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTINCV:
                fprintf(output, "POSTINCV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTDECV:
                fprintf(output, "POSTDECV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case INCATV:
                fprintf(output, "INC@V\n");
                break;
            case DECATV:
                fprintf(output, "DEC@V\n");
                break;
            case POSTINCATV:
                fprintf(output, "POSTINC@V\n");
                break;
            case POSTDECATV:
                fprintf(output, "POSTDEC@V\n");
                break;
            case INCRV:
                fprintf(output, "INCfV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case DECRV:
                fprintf(output, "DECfV");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTINCRV:
                fprintf(output, "POSTINCfV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case POSTDECRV:
                fprintf(output, "POSTDECfV ");
                fprintf(output, "%i\n", mem[i++]);
                break;
            case INCATRV:
                fprintf(output, "INC@fV\n");
                break;
            case DECATRV:
                fprintf(output, "DEC@fV\n");
                break;
            case POSTINCATRV:
                fprintf(output, "POSTINC@fV\n");
                break;
            case POSTDECATRV:
                fprintf(output, "POSTDEC@fV\n");
                break;

            case LNOT:
                fprintf(output, "BITNOT\n");
                break;
            case LOGNOT:
                fprintf(output, "NOT\n");
                break;
            case UNMINUS:
                fprintf(output, "UNMINUS\n");
                break;
            case UNMINUSR:
                fprintf(output, "UNMINUSf\n");
                break;
                
            case FUNCBEG:
                fprintf(output, "FUNCBEG ");
                fprintf(output, "%i ", mem[i++]);
                fprintf(output, "%i\n", mem[i++]);
                break;
                
                
            default:
                fprintf(output, "%i\n", mem[i-1]);
                break;
        }
    }
    
}
