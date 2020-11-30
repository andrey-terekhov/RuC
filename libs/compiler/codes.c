/*
 *	Copyright 2014 Andrey Terekhov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "codes.h"
#include "defs.h"
#include "uniprinter.h"
#include <stdio.h>
#include <string.h>


void tables_and_tree(compiler_context *context)
{
	uni_printf(context->io, "\n%s\n", "identab");
	for (int i = 2; i < context->id; i += 4)
	{
		for (int j = 0; j < 4; j++)
		{
			uni_printf(context->io, "id %i) %i\n", i + j, context->identab[i + j]);
		}
		uni_printf(context->io, "\n");
	}

	/*
	uni_printf(context->io, "\n%s\n", "repr");
	for (int i = 1206; i <= rp; i++)
	{
		uni_printf(context->io, "rp %i) %i\n", i, reprtab[i]);
	}
	*/

	uni_printf(context->io, "\n%s\n", "modetab");
	for (int i = 0; i < context->md; i++)
	{
		uni_printf(context->io, "md %i) %i\n", i, context->modetab[i]);
	}

	/*
	uni_printf(context->io, "\n%s\n", "tree");
	for (int i = 0; i <= tc; i++)
	{
		uni_printf(context->io, "tc %i) %i\n", i, context->tree[i]);
	}
	*/

	uni_printf(context->io, "\n");

	int i = 0;
	while (i < context->tc)
	{
		uni_printf(context->io, "tc %i) ", i);
		switch (context->tree[i++])
		{
			case TFuncdef:
				uni_printf(context->io, "TFuncdef funcn= %i maxdispl= %i\n", context->tree[i],
							   context->tree[i + 1]);
				i += 2;
				break;
			case TDeclarr:
				uni_printf(context->io, "TDeclarr N= %i\n", context->tree[i++]);
				break;
			case TDeclid:
				uni_printf(context->io,
							   "TDeclid displ= %i eltype= %i N= %i all= %i iniproc= "
							   "%i, usual= %i instuct= %i\n",
							   context->tree[i], context->tree[i + 1], context->tree[i + 2], context->tree[i + 3],
							   context->tree[i + 4], context->tree[i + 5], context->tree[i + 6]);
				i += 7;
				break;
			case TString:
			{
				int n = context->tree[i++];
				uni_printf(context->io, "TString n= %i\n", n);
				for (int j = 0; j < n; ++j)
				{
					uni_printf(context->io, "%i\n", context->tree[i++]);
				}
			}
			break;
			case TStringd:
			{
				int n = context->tree[i++];
				uni_printf(context->io, "TStringd n= %i\n", n);
				for (int j = 0; j < n; ++j)
				{
					double d;
					memcpy(&d, &context->tree[i], sizeof(double));
					i += 2;
					uni_printf(context->io, "%f\n", d);
				}
			}
			break;
			case TCondexpr:
				uni_printf(context->io, "TCondexpr\n");
				break;
			case TBegin:
				uni_printf(context->io, "TBegin\n");
				break;
			case TEnd:
				uni_printf(context->io, "TEnd\n");
				break;
			case TBeginit:
				uni_printf(context->io, "TBeginit n= %i\n", context->tree[i++]);
				break;
			case TStructinit:
				uni_printf(context->io, "TStructinit n= %i\n", context->tree[i++]);
				break;
			case TIf:
				uni_printf(context->io, "TIf %i\n", context->tree[i++]);
				break;
			case TWhile:
				uni_printf(context->io, "TWhile\n");
				break;
			case TDo:
				uni_printf(context->io, "TDo\n");
				break;
			case TFor:
				uni_printf(context->io, "TFor %i %i %i %i\n", context->tree[i], context->tree[i + 1],
							   context->tree[i + 2], context->tree[i + 3]);
				i += 4;
				break;
			case TSwitch:
				uni_printf(context->io, "TSwitch\n");
				break;
			case TCase:
				uni_printf(context->io, "TCase\n");
				break;
			case TDefault:
				uni_printf(context->io, "TDefault\n");
				break;
			case TBreak:
				uni_printf(context->io, "TBreak\n");
				break;
			case TContinue:
				uni_printf(context->io, "TContinue\n");
				break;
			case TReturnvoid:
				uni_printf(context->io, "TReturn\n");
				break;
			case TReturnval:
				uni_printf(context->io, "TReturnval %i\n", context->tree[i++]);
				break;
			case TGoto:
				uni_printf(context->io, "TGoto %i\n", context->tree[i++]);
				break;
			case TIdent:
				uni_printf(context->io, "TIdent %i\n", context->tree[i++]);
				break;
			case TIdenttoval:
				uni_printf(context->io, "TIdenttoval %i\n", context->tree[i++]);
				break;
			case TIdenttovald:
				uni_printf(context->io, "TIdenttovald %i\n", context->tree[i++]);
				break;
			case TFunidtoval:
				uni_printf(context->io, "TFunidtoval %i\n", context->tree[i++]);
				break;
			case TIdenttoaddr:
				uni_printf(context->io, "TIdenttoaddr %i\n", context->tree[i++]);
				break;
			case TAddrtoval:
				uni_printf(context->io, "TAddrtoval\n");
				break;
			case TAddrtovald:
				uni_printf(context->io, "TAddrtovald\n");
				break;
			case TExprend:
				uni_printf(context->io, "TExprend\n");
				break;
			case TConst:
				uni_printf(context->io, "TConst %i\n", context->tree[i++]);
				break;
			case TConstd:
				memcpy(&context->numdouble, &context->tree[i], sizeof(double));
				i += 2;
				uni_printf(context->io, "TConstd %f\n", context->numdouble);
				break;
			case TSliceident:
				uni_printf(context->io, "TSliceident displ= %i type= %i\n", context->tree[i],
							   context->tree[i + 1]);
				i += 2;
				break;
			case TSlice:
				uni_printf(context->io, "TSlice elem_type= %i\n", context->tree[i++]);
				break;
			case TSelect:
				uni_printf(context->io, "TSelect displ= %i\n", context->tree[i++]);
				break;
			case NOP:
				uni_printf(context->io, "NOP\n");
				break;
			case ADLOGAND:
				uni_printf(context->io, "ADLOGAND addr= %i\n", context->tree[i++]);
				break;
			case ADLOGOR:
				uni_printf(context->io, "ADLOGOR addr= %i\n", context->tree[i++]);
				break;
			case COPY00:
				uni_printf(context->io, "COPY00 %i ",
							   context->tree[i++]); // displleft
				uni_printf(context->io, "%i ",
							   context->tree[i++]); // displright
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY01:
				uni_printf(context->io, "COPY01 %i ",
							   context->tree[i++]); // displleft
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY10:
				uni_printf(context->io, "COPY10 %i ",
							   context->tree[i++]); // displright
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY11:
				uni_printf(context->io, "COPY11 %i\n",
							   context->tree[i++]); // length
				break;
			case COPY0ST:
				uni_printf(context->io, "COPY0ST %i ",
							   context->tree[i++]); // displleft
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY1ST:
				uni_printf(context->io, "COPY1ST (%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY0STASS:
				uni_printf(context->io, "COPY0STASS %i ",
							   context->tree[i++]); // displleft
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length
				break;
			case COPY1STASS:
				uni_printf(context->io, "COPY1STASS (%i)\n",
							   context->tree[i++]); // length
				break;
			case COPYST:
				uni_printf(context->io, "COPYST %i ",
							   context->tree[i++]); // displ
				uni_printf(context->io, "(%i)",
							   context->tree[i++]); // length
				uni_printf(context->io, "(%i)\n",
							   context->tree[i++]); // length1
				break;

			case TCall1:
				uni_printf(context->io, "TCall1 %i\n", context->tree[i++]);
				break;
			case TCall2:
				uni_printf(context->io, "TCall2 %i\n", context->tree[i++]);
				break;
			case TLabel:
				uni_printf(context->io, "TLabel %i\n", context->tree[i++]);
				break;
			case TStructbeg:
				uni_printf(context->io, "TStructbeg %i\n", context->tree[i++]);
				break;
			case TStructend:
				uni_printf(context->io, "TStructend %i\n", context->tree[i++]);
				break;
			case TPrint:
				uni_printf(context->io, "TPrint %i\n", context->tree[i++]);
				break;
			case TPrintid:
				uni_printf(context->io, "TPrintid %i\n", context->tree[i++]);
				break;
			case TPrintf:
				uni_printf(context->io, "TPrintf %i\n", context->tree[i++]);
				break;
			case TGetid:
				uni_printf(context->io, "TGetid %i\n", context->tree[i++]);
				break;
			case SETMOTORC:
				uni_printf(context->io, "Setmotor\n");
				break;
			case CREATEC:
				uni_printf(context->io, "TCREATE\n");
				break;
			case CREATEDIRECTC:
				uni_printf(context->io, "TCREATEDIRECT\n");
				break;
			case EXITC:
				uni_printf(context->io, "TEXIT\n");
				break;
			case EXITDIRECTC:
				uni_printf(context->io, "TEXITDIRECT\n");
				break;
			case MSGSENDC:
				uni_printf(context->io, "TMSGSEND\n");
				break;
			case MSGRECEIVEC:
				uni_printf(context->io, "TMSGRECEIVE\n");
				break;
			case JOINC:
				uni_printf(context->io, "TJOIN\n");
				break;
			case SLEEPC:
				uni_printf(context->io, "TSLEEP\n");
				break;
			case SEMCREATEC:
				uni_printf(context->io, "TSEMCREATE\n");
				break;
			case SEMWAITC:
				uni_printf(context->io, "TSEMWAIT\n");
				break;
			case SEMPOSTC:
				uni_printf(context->io, "TSEMPOST\n");
				break;
			case INITC:
				uni_printf(context->io, "INITC\n");
				break;
			case DESTROYC:
				uni_printf(context->io, "DESTROYC\n");
				break;
			case GETNUMC:
				uni_printf(context->io, "GETNUMC\n");
				break;


			default:
				uni_printf(context->io, "TOper %i\n", context->tree[i - 1]);
		}
	}
}

void tables_and_code(compiler_context *context)
{
	uni_printf(context->io, "\n\n%s\n", "functions");
	for (int i = 1; i <= context->funcnum; i++)
	{
		uni_printf(context->io, "fun %i) %i\n", i, context->functions[i]);
	}

	uni_printf(context->io, "\n%s\n", "iniprocs");
	for (int i = 1; i <= context->procd; i++)
	{
		uni_printf(context->io, "inipr %i) %i\n", i, context->iniprocs[i]);
	}

	uni_printf(context->io, "\n%s\n", "mem");
	int i = 0;
	while (i < context->pc)
	{
		uni_printf(context->io, "pc %i) ", i);
		switch (context->mem[i++])
		{
			case PRINT:
				uni_printf(context->io, "PRINT %i\n", context->mem[i++]);
				break;
			case PRINTID:
				uni_printf(context->io, "PRINTID %i\n", context->mem[i++]);
				break;
			case PRINTF:
				uni_printf(context->io, "PRINTF %i\n", context->mem[i++]);
				break;
			case GETID:
				uni_printf(context->io, "GETID %i\n", context->mem[i++]);
				break;
			case SETMOTORC:
				uni_printf(context->io, "SETMOTOR\n");
				break;
			case GETDIGSENSORC:
				uni_printf(context->io, "GETDIGSENSOR\n");
				break;
			case GETANSENSORC:
				uni_printf(context->io, "GETANSENSOR\n");
				break;
			case VOLTAGEC:
				uni_printf(context->io, "VOLTAGE\n");
				break;
			case CREATEC:
				uni_printf(context->io, "TCREATE\n");
				break;
			case CREATEDIRECTC:
				uni_printf(context->io, "TCREATEDIRECT\n");
				break;
			case MSGSENDC:
				uni_printf(context->io, "TMSGSEND\n");
				break;
			case EXITC:
				uni_printf(context->io, "TEXIT\n");
				break;
			case EXITDIRECTC:
				uni_printf(context->io, "TEXITDIRECT\n");
				break;
			case MSGRECEIVEC:
				uni_printf(context->io, "TMSGRECEIVE\n");
				break;
			case JOINC:
				uni_printf(context->io, "TJOIN\n");
				break;
			case SLEEPC:
				uni_printf(context->io, "TSLEEP\n");
				break;
			case SEMCREATEC:
				uni_printf(context->io, "TSEMCREATE\n");
				break;
			case SEMWAITC:
				uni_printf(context->io, "TSEMWAIT\n");
				break;
			case SEMPOSTC:
				uni_printf(context->io, "TSEMPOST\n");
				break;
			case TINIT:
				uni_printf(context->io, "TINIT\n");
				break;
			case TDESTROY:
				uni_printf(context->io, "TDESTROY\n");
				break;
			case GETNUMC:
				uni_printf(context->io, "GETNUM\n");
				break;

			case ABSC:
				uni_printf(context->io, "ABS\n");
				break;
			case ABSIC:
				uni_printf(context->io, "ABSI\n");
				break;
			case SQRTC:
				uni_printf(context->io, "SQRT\n");
				break;
			case EXPC:
				uni_printf(context->io, "EXP\n");
				break;
			case SINC:
				uni_printf(context->io, "SIN\n");
				break;
			case COSC:
				uni_printf(context->io, "COS\n");
				break;
			case LOGC:
				uni_printf(context->io, "LOG\n");
				break;
			case LOG10C:
				uni_printf(context->io, "LOG10\n");
				break;
			case ASINC:
				uni_printf(context->io, "ASIN\n");
				break;
			case RANDC:
				uni_printf(context->io, "RAND\n");
				break;
			case ROUNDC:
				uni_printf(context->io, "ROUND\n");
				break;

			case STRCPYC:
				uni_printf(context->io, "STRCPY\n");
				break;
			case STRNCPYC:
				uni_printf(context->io, "STRNCPY\n");
				break;
			case STRCATC:
				uni_printf(context->io, "STRCAT\n");
				break;
			case STRNCATC:
				uni_printf(context->io, "STRNCAT\n");
				break;
			case STRCMPC:
				uni_printf(context->io, "STRCMP\n");
				break;
			case STRNCMPC:
				uni_printf(context->io, "STRNCMP\n");
				break;
			case STRSTRC:
				uni_printf(context->io, "STRSTR\n");
				break;
			case STRLENC:
				uni_printf(context->io, "STRLENC\n");
				break;

			case BEGINIT:
				uni_printf(context->io, "BEGINIT n= %i\n", context->mem[i++]);
				break;
			case STRUCTWITHARR:
				uni_printf(context->io, "STRUCTWITHARR displ= %i ", context->mem[i++]);
				uni_printf(context->io, "iniproc= %i\n", context->mem[i++]);
				break;
			case DEFARR:
				uni_printf(context->io, "DEFARR N= %i ",
							   context->mem[i++]); // N
				uni_printf(context->io, "elem_len= %i ",
							   context->mem[i++]); // elem length
				uni_printf(context->io, "displ= %i ",
							   context->mem[i++]); // displ
				uni_printf(context->io, "iniproc= %i ",
							   context->mem[i++]); // iniproc
				uni_printf(context->io, "usual= %i ",
							   context->mem[i++]); // usual
				uni_printf(context->io, "all= %i ",
							   context->mem[i++]); // all
				uni_printf(context->io, "instruct= %i\n",
							   context->mem[i++]); // instruct
				break;
			case ARRINIT:
				uni_printf(context->io, "ARRINIT N= %i ", context->mem[i++]);
				uni_printf(context->io, "elem_len= %i ", context->mem[i++]);
				uni_printf(context->io, "displ= %i ", context->mem[i++]);
				uni_printf(context->io, "usual= %i\n", context->mem[i++]);
				break;
			/*
			case STRUCTINIT:
				uni_printf(context->io,	"STRUCTINIT N= %i ", context->mem[i++]);
				break;
			*/
			case NOP:
				uni_printf(context->io, "NOP\n");
				break;
			case LI:
				uni_printf(context->io, "LI %i\n", context->mem[i++]);
				break;
			case LID:
				memcpy(&context->numdouble, &context->mem[i], sizeof(double));
				i += 2;
				uni_printf(context->io, "LID %.15f\n", context->numdouble);
				break;
			case LOAD:
				uni_printf(context->io, "LOAD %i\n", context->mem[i++]);
				break;
			case LOADD:
				uni_printf(context->io, "LOADD %i\n", context->mem[i++]);
				break;
			case LAT:
				uni_printf(context->io, "L@\n");
				break;
			case LATD:
				uni_printf(context->io, "L@f\n");
				break;
			case LA:
				uni_printf(context->io, "LA %i\n", context->mem[i++]);
				break;

			case LOGOR:
				uni_printf(context->io, "||\n");
				break;
			case LOGAND:
				uni_printf(context->io, "&&\n");
				break;
			case ORASS:
				uni_printf(context->io, "|= %i\n", context->mem[i++]);
				break;
			case ORASSAT:
				uni_printf(context->io, "|=@\n");
				break;
			case ORASSV:
				uni_printf(context->io, "|=V %i\n", context->mem[i++]);
				break;
			case ORASSATV:
				uni_printf(context->io, "|=@V\n");
				break;
			case LOR:
				uni_printf(context->io, "|\n");
				break;
			case EXORASS:
				uni_printf(context->io, "^= %i\n", context->mem[i++]);
				break;
			case EXORASSAT:
				uni_printf(context->io, "^=@\n");
				break;
			case EXORASSV:
				uni_printf(context->io, "^=V %i\n", context->mem[i++]);
				break;
			case EXORASSATV:
				uni_printf(context->io, "^=@V\n");
				break;
			case LEXOR:
				uni_printf(context->io, "^\n");
				break;
			case ANDASS:
				uni_printf(context->io, "&= %i\n", context->mem[i++]);
				break;
			case ANDASSAT:
				uni_printf(context->io, "&=@\n");
				break;
			case ANDASSV:
				uni_printf(context->io, "&=V %i\n", context->mem[i++]);
				break;
			case ANDASSATV:
				uni_printf(context->io, "&=@V\n");
				break;
			case LAND:
				uni_printf(context->io, "&\n");
				break;

			case EQEQ:
				uni_printf(context->io, "==\n");
				break;
			case NOTEQ:
				uni_printf(context->io, "!=\n");
				break;
			case LLT:
				uni_printf(context->io, "<\n");
				break;
			case LGT:
				uni_printf(context->io, ">\n");
				break;
			case LLE:
				uni_printf(context->io, "<=\n");
				break;
			case LGE:
				uni_printf(context->io, ">=\n");
				break;
			case EQEQR:
				uni_printf(context->io, "==f\n");
				break;
			case NOTEQR:
				uni_printf(context->io, "!=f\n");
				break;
			case LLTR:
				uni_printf(context->io, "<f\n");
				break;
			case LGTR:
				uni_printf(context->io, ">f\n");
				break;
			case LLER:
				uni_printf(context->io, "<=f\n");
				break;
			case LGER:
				uni_printf(context->io, ">=f\n");
				break;

			case SHRASS:
				uni_printf(context->io, ">>= %i\n", context->mem[i++]);
				break;
			case SHRASSAT:
				uni_printf(context->io, ">>=@\n");
				break;
			case SHRASSV:
				uni_printf(context->io, ">>=V %i\n", context->mem[i++]);
				break;
			case SHRASSATV:
				uni_printf(context->io, ">>=@V\n");
				break;
			case LSHR:
				uni_printf(context->io, ">>\n");
				break;
			case SHLASS:
				uni_printf(context->io, "<<= %i\n", context->mem[i++]);
				break;
			case SHLASSAT:
				uni_printf(context->io, "<<=@\n");
				break;
			case SHLASSV:
				uni_printf(context->io, "<<=V %i\n", context->mem[i++]);
				break;
			case SHLASSATV:
				uni_printf(context->io, "<<=@V\n");
				break;
			case LSHL:
				uni_printf(context->io, "<<\n");
				break;

			case ASS:
				uni_printf(context->io, "= %i\n", context->mem[i++]);
				break;
			case ASSAT:
				uni_printf(context->io, "=@\n");
				break;
			case ASSV:
				uni_printf(context->io, "=V %i\n", context->mem[i++]);
				break;
			case ASSATV:
				uni_printf(context->io, "=@V\n");
				break;

			case PLUSASS:
				uni_printf(context->io, "+= %i\n", context->mem[i++]);
				break;
			case PLUSASSAT:
				uni_printf(context->io, "+=@\n");
				break;
			case PLUSASSV:
				uni_printf(context->io, "+=V %i\n", context->mem[i++]);
				break;
			case PLUSASSATV:
				uni_printf(context->io, "+=@V\n");
				break;
			case LPLUS:
				uni_printf(context->io, "+\n");
				break;

			case MINUSASS:
				uni_printf(context->io, "-= %i\n", context->mem[i++]);
				break;
			case MINUSASSAT:
				uni_printf(context->io, "-=@\n");
				break;
			case MINUSASSV:
				uni_printf(context->io, "-=V %i\n", context->mem[i++]);
				break;
			case MINUSASSATV:
				uni_printf(context->io, "-=@V\n");
				break;
			case LMINUS:
				uni_printf(context->io, "-\n");
				break;

			case MULTASS:
				uni_printf(context->io, "*= %i\n", context->mem[i++]);
				break;
			case MULTASSAT:
				uni_printf(context->io, "*=@\n");
				break;
			case MULTASSV:
				uni_printf(context->io, "*=V %i\n", context->mem[i++]);
				break;
			case MULTASSATV:
				uni_printf(context->io, "*=@V\n");
				break;
			case LMULT:
				uni_printf(context->io, "*\n");
				break;

			case DIVASS:
				uni_printf(context->io, "/= %i\n", context->mem[i++]);
				break;
			case DIVASSAT:
				uni_printf(context->io, "/=@\n");
				break;
			case DIVASSV:
				uni_printf(context->io, "/=V %i\n", context->mem[i++]);
				break;
			case DIVASSATV:
				uni_printf(context->io, "/=@V\n");
				break;
			case LDIV:
				uni_printf(context->io, "/\n");
				break;

			case ASSR:
				uni_printf(context->io, "=f %i\n", context->mem[i++]);
				break;
			case ASSRV:
				uni_printf(context->io, "=fV %i\n", context->mem[i++]);
				break;
			case ASSATR:
				uni_printf(context->io, "=@f\n");
				break;
			case ASSATRV:
				uni_printf(context->io, "=@fV\n");
				break;

			case PLUSASSR:
				uni_printf(context->io, "+=f %i\n", context->mem[i++]);
				break;
			case PLUSASSATR:
				uni_printf(context->io, "+=@f\n");
				break;
			case PLUSASSRV:
				uni_printf(context->io, "+=fV %i\n", context->mem[i++]);
				break;
			case PLUSASSATRV:
				uni_printf(context->io, "+=@fV\n");
				break;
			case LPLUSR:
				uni_printf(context->io, "+f\n");
				break;
			case MINUSASSR:
				uni_printf(context->io, "-=f %i\n", context->mem[i++]);
				break;
			case MINUSASSATR:
				uni_printf(context->io, "-=@f\n");
				break;
			case MINUSASSRV:
				uni_printf(context->io, "-=fV %i\n", context->mem[i++]);
				break;
			case MINUSASSATRV:
				uni_printf(context->io, "-=@fV\n");
				break;
			case LMINUSR:
				uni_printf(context->io, "-f\n");
				break;
			case MULTASSR:
				uni_printf(context->io, "*=f %i\n", context->mem[i++]);
				break;
			case MULTASSATR:
				uni_printf(context->io, "*=@f\n");
				break;
			case MULTASSRV:
				uni_printf(context->io, "*=fV %i\n", context->mem[i++]);
				break;
			case MULTASSATRV:
				uni_printf(context->io, "*=@fV\n");
				break;
			case LMULTR:
				uni_printf(context->io, "*f\n");
				break;
			case DIVASSR:
				uni_printf(context->io, "/=f %i\n", context->mem[i++]);
				break;
			case DIVASSATR:
				uni_printf(context->io, "/=@f\n");
				break;
			case DIVASSRV:
				uni_printf(context->io, "/=fV %i\n", context->mem[i++]);
				break;
			case DIVASSATRV:
				uni_printf(context->io, "/=@fV\n");
				break;
			case LDIVR:
				uni_printf(context->io, "/f\n");
				break;
			case COPY00:
				uni_printf(context->io, "COPY00 %i ",
							   context->mem[i++]); // displleft
				uni_printf(context->io, "%i ",
							   context->mem[i++]); // displright
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length
				break;
			case COPY01:
				uni_printf(context->io, "COPY01 %i      ",
							   context->mem[i++]); // displleft
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length
				break;
			case COPY10:
				uni_printf(context->io, "COPY10      %i ",
							   context->mem[i++]); // displright
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length
				break;
			case COPY11:
				uni_printf(context->io, "COPY11 %i\n",
							   context->mem[i++]); // length
				break;
			case COPY0ST:
				uni_printf(context->io, "COPY0ST %i ",
							   context->mem[i++]); // displright
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length
				break;
			case COPY1ST:
				uni_printf(context->io, "COPY1ST %i\n",
							   context->mem[i++]); // length
				break;
			case COPY0STASS:
				uni_printf(context->io, "COPY0STASS %i ",
							   context->mem[i++]); // displleft
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length
				break;
			case COPY1STASS:
				uni_printf(context->io, "COPY1STASS %i\n",
							   context->mem[i++]); // length
				break;
			case COPYST:
				uni_printf(context->io, "COPYST %i ",
							   context->mem[i++]); // displ
				uni_printf(context->io, "(%i)",
							   context->mem[i++]); // length
				uni_printf(context->io, "(%i)\n",
							   context->mem[i++]); // length1
				break;

			case REMASS:
				uni_printf(context->io, "%%= %i\n", context->mem[i++]);
				break;
			case REMASSAT:
				uni_printf(context->io, "%%=@\n");
				break;
			case REMASSV:
				uni_printf(context->io, "%%=V %i\n", context->mem[i++]);
				break;
			case REMASSATV:
				uni_printf(context->io, "%%=@V\n");
				break;
			case LREM:
				uni_printf(context->io, "%%\n");
				break;

			case CALL1:
				uni_printf(context->io, "CALL1\n");
				break;
			case CALL2:
				uni_printf(context->io, "CALL2 ");
				uni_printf(context->io, "%i\n", context->mem[i++]);
				break;
			case STOP:
				uni_printf(context->io, "STOP\n");
				break;
			case RETURNVAL:
				uni_printf(context->io, "RETURNVAL %i\n", context->mem[i++]);
				break;
			case RETURNVOID:
				uni_printf(context->io, "RETURNVOID\n");
				break;
			case B:
				uni_printf(context->io, "B %i\n", context->mem[i++]);
				break;
			case BE0:
				uni_printf(context->io, "BE0 %i\n", context->mem[i++]);
				break;
			case BNE0:
				uni_printf(context->io, "BNE0 %i\n", context->mem[i++]);
				break;
			case SLICE:
				uni_printf(context->io, "SLICE d= %i\n", context->mem[i++]);
				break;
			case SELECT:
				uni_printf(context->io, "SELECT field_displ= %i\n", context->mem[i++]);
				break;
			case WIDEN:
				uni_printf(context->io, "WIDEN\n");
				break;
			case WIDEN1:
				uni_printf(context->io, "WIDEN1\n");
				break;
			case _DOUBLE:
				uni_printf(context->io, "DOUBLE\n");
				break;
			case INC:
				uni_printf(context->io, "INC %i\n", context->mem[i++]);
				break;
			case DEC:
				uni_printf(context->io, "DEC %i\n", context->mem[i++]);
				break;
			case POSTINC:
				uni_printf(context->io, "POSTINC %i\n", context->mem[i++]);
				break;
			case POSTDEC:
				uni_printf(context->io, "POSTDEC %i\n", context->mem[i++]);
				break;
			case INCAT:
				uni_printf(context->io, "INC@\n");
				break;
			case DECAT:
				uni_printf(context->io, "DEC@\n");
				break;
			case POSTINCAT:
				uni_printf(context->io, "POSTINC@\n");
				break;
			case POSTDECAT:
				uni_printf(context->io, "POSTDEC@\n");
				break;
			case INCR:
				uni_printf(context->io, "INCf %i\n", context->mem[i++]);
				break;
			case DECR:
				uni_printf(context->io, "DECf %i\n", context->mem[i++]);
				break;
			case POSTINCR:
				uni_printf(context->io, "POSTINCf %i\n", context->mem[i++]);
				break;
			case POSTDECR:
				uni_printf(context->io, "POSTDECf %i\n", context->mem[i++]);
				break;
			case INCATR:
				uni_printf(context->io, "INC@f\n");
				break;
			case DECATR:
				uni_printf(context->io, "DEC@f\n");
				break;
			case POSTINCATR:
				uni_printf(context->io, "POSTINC@f\n");
				break;
			case POSTDECATR:
				uni_printf(context->io, "POSTDEC@f\n");
				break;
			case INCV:
				uni_printf(context->io, "INCV %i\n", context->mem[i++]);
				break;
			case DECV:
				uni_printf(context->io, "DECV %i\n", context->mem[i++]);
				break;
			case POSTINCV:
				uni_printf(context->io, "POSTINCV %i\n", context->mem[i++]);
				break;
			case POSTDECV:
				uni_printf(context->io, "POSTDECV %i\n", context->mem[i++]);
				break;
			case INCATV:
				uni_printf(context->io, "INC@V\n");
				break;
			case DECATV:
				uni_printf(context->io, "DEC@V\n");
				break;
			case POSTINCATV:
				uni_printf(context->io, "POSTINC@V\n");
				break;
			case POSTDECATV:
				uni_printf(context->io, "POSTDEC@V\n");
				break;
			case INCRV:
				uni_printf(context->io, "INCfV %i\n", context->mem[i++]);
				break;
			case DECRV:
				uni_printf(context->io, "DECfV %i\n", context->mem[i++]);
				break;
			case POSTINCRV:
				uni_printf(context->io, "POSTINCfV %i\n", context->mem[i++]);
				break;
			case POSTDECRV:
				uni_printf(context->io, "POSTDECfV %i\n", context->mem[i++]);
				break;
			case INCATRV:
				uni_printf(context->io, "INC@fV\n");
				break;
			case DECATRV:
				uni_printf(context->io, "DEC@fV\n");
				break;
			case POSTINCATRV:
				uni_printf(context->io, "POSTINC@fV\n");
				break;
			case POSTDECATRV:
				uni_printf(context->io, "POSTDEC@fV\n");
				break;

			case LNOT:
				uni_printf(context->io, "BITNOT\n");
				break;
			case LOGNOT:
				uni_printf(context->io, "NOT\n");
				break;
			case UNMINUS:
				uni_printf(context->io, "UNMINUS\n");
				break;
			case UNMINUSR:
				uni_printf(context->io, "UNMINUSf\n");
				break;

			case FUNCBEG:
				uni_printf(context->io, "FUNCBEG maxdispl= %i ", context->mem[i++]);
				uni_printf(context->io, "pc= %i\n", context->mem[i++]);
				break;


			default:
				uni_printf(context->io, "%i\n", context->mem[i - 1]);
		}
	}
}
