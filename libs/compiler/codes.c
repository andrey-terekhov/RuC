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
#include <string.h>
#include "defs.h"
#include "errors.h"
#include "old_tree.h"
#include "uniio.h"
#include "uniprinter.h"


#define MAX_ELEM_SIZE	32
#define INDENT			"  "


size_t elem_get_name(const item_t elem, const size_t num, char *const buffer)
{
	if (buffer == NULL)
	{
		return 0;
	}

	size_t argc = 0;
	int was_switch = 0;

	switch (elem)
	{
		case TFuncdef:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TFuncdef");
					break;
				case 1:
					sprintf(buffer, "funcn");
					break;
				case 2:
					sprintf(buffer, "maxdispl");
					break;
			}
			break;
		case TDeclarr:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TDeclarr");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
			}
			break;
		case TDeclid:
			argc = 7;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TDeclid");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "eltype");
					break;
				case 3:
					sprintf(buffer, "N");
					break;
				case 4:
					sprintf(buffer, "all");
					break;
				case 5:
					sprintf(buffer, "iniproc");
					break;
				case 6:
					sprintf(buffer, "usual");
					break;
				case 7:
					sprintf(buffer, "instuct");
					break;
			}
			break;
		case TString:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TString");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case TStringd:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TStringd");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case TCondexpr:
			sprintf(buffer, "TCondexpr");
			break;
		case TBegin:
			sprintf(buffer, "TBegin");
			break;
		case TEnd:
			sprintf(buffer, "TEnd");
			break;
		case TBeginit:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TBeginit");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case TStructinit:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TStructinit");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case TIf:
			argc = 1;
			sprintf(buffer, "TIf");
			break;
		case TWhile:
			sprintf(buffer, "TWhile");
			break;
		case TDo:
			sprintf(buffer, "TDo");
			break;
		case TFor:
			argc = 4;
			sprintf(buffer, "TFor");
			break;
		case TSwitch:
			sprintf(buffer, "TSwitch");
			break;
		case TCase:
			sprintf(buffer, "TCase");
			break;
		case TDefault:
			sprintf(buffer, "TDefault");
			break;
		case TBreak:
			sprintf(buffer, "TBreak");
			break;
		case TContinue:
			sprintf(buffer, "TContinue");
			break;
		case TReturnvoid:
			sprintf(buffer, "TReturn");
			break;
		case TReturnval:
			argc = 1;
			sprintf(buffer, "TReturnval");
			break;
		case TGoto:
			argc = 1;
			sprintf(buffer, "TGoto");
			break;
		case TIdent:
			argc = 1;
			sprintf(buffer, "TIdent");
			break;
		case TIdenttoval:
			argc = 1;
			sprintf(buffer, "TIdenttoval");
			break;
		case TIdenttovald:
			argc = 1;
			sprintf(buffer, "TIdenttovald");
			break;
		case TFunidtoval:
			argc = 1;
			sprintf(buffer, "TFunidtoval");
			break;
		case TIdenttoaddr:
			argc = 1;
			sprintf(buffer, "TIdenttoaddr");
			break;
		case TAddrtoval:
			sprintf(buffer, "TAddrtoval");
			break;
		case TAddrtovald:
			sprintf(buffer, "TAddrtovald");
			break;
		case TExprend:
			sprintf(buffer, "TExprend");
			break;
		case TConst:
			argc = 1;
			sprintf(buffer, "TConst");
			break;
		case TConstd:
			argc = 2;
			sprintf(buffer, "TConstd");
		break;
		case TSliceident:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TSliceident");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "type");
					break;
			}
			break;
		case TSlice:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TSlice");
					break;
				case 1:
					sprintf(buffer, "elem_type");
					break;
			}
			break;
		case TSelect:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "TSelect");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
			}
			break;
		case NOP:
			sprintf(buffer, "NOP");
			break;
		case ADLOGAND:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "ADLOGAND");
					break;
				case 1:
					sprintf(buffer, "addr");
					break;
			}
			break;
		case ADLOGOR:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "ADLOGOR");
					break;
				case 1:
					sprintf(buffer, "addr");
					break;
			}
			break;
		case COPY00:
			argc = 3;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY00");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "displright");
					break;
				case 3:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY01:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY01");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY10:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY10");
					break;
				case 1:
					sprintf(buffer, "displright");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY11:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY11");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY0ST:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0ST");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY1ST:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1ST");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY0STASS:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY0STASS");
					break;
				case 1:
					sprintf(buffer, "displleft");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPY1STASS:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPY1STASS");
					break;
				case 1:
					sprintf(buffer, "length");
					break;
			}
			break;
		case COPYST:
			argc = 3;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "COPYST");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "length");
					break;
				case 3:
					sprintf(buffer, "length1");
					break;
			}
			break;

		case TCall1:
			argc = 1;
			sprintf(buffer, "TCall1");
			break;
		case TCall2:
			argc = 1;
			sprintf(buffer, "TCall2");
			break;
		case TLabel:
			argc = 1;
			sprintf(buffer, "TLabel");
			break;
		case TStructbeg:
			argc = 1;
			sprintf(buffer, "TStructbeg");
			break;
		case TStructend:
			argc = 1;
			sprintf(buffer, "TStructend");
			break;
		case TPrint:
			argc = 1;
			sprintf(buffer, "TPrint");
			break;
		case TPrintid:
			argc = 1;
			sprintf(buffer, "TPrintid");
			break;
		case TPrintf:
			argc = 1;
			sprintf(buffer, "TPrintf");
			break;
		case TGetid:
			argc = 1;
			sprintf(buffer, "TGetid");
			break;
		case SETMOTORC:
			sprintf(buffer, "Setmotor");
			break;
		case CREATEC:
			sprintf(buffer, "TCREATE");
			break;
		case CREATEDIRECTC:
			sprintf(buffer, "TCREATEDIRECT");
			break;
		case EXITC:
			sprintf(buffer, "TEXIT");
			break;
		case EXITDIRECTC:
			sprintf(buffer, "TEXITDIRECT");
			break;
		case MSGSENDC:
			sprintf(buffer, "TMSGSEND");
			break;
		case MSGRECEIVEC:
			sprintf(buffer, "TMSGRECEIVE");
			break;
		case JOINC:
			sprintf(buffer, "TJOIN");
			break;
		case SLEEPC:
			sprintf(buffer, "TSLEEP");
			break;
		case SEMCREATEC:
			sprintf(buffer, "TSEMCREATE");
			break;
		case SEMWAITC:
			sprintf(buffer, "TSEMWAIT");
			break;
		case SEMPOSTC:
			sprintf(buffer, "TSEMPOST");
			break;
		case INITC:
			sprintf(buffer, "INITC");
			break;
		case DESTROYC:
			sprintf(buffer, "DESTROYC");
			break;
		case GETNUMC:
			sprintf(buffer, "GETNUMC");
			break;

		case PRINT:
			argc = 1;
			sprintf(buffer, "PRINT");
			break;
		case PRINTID:
			argc = 1;
			sprintf(buffer, "PRINTID");
			break;
		case PRINTF:
			argc = 1;
			sprintf(buffer, "PRINTF");
			break;
		case GETID:
			argc = 1;
			sprintf(buffer, "GETID");
			break;
		case GETDIGSENSORC:
			sprintf(buffer, "GETDIGSENSOR");
			break;
		case GETANSENSORC:
			sprintf(buffer, "GETANSENSOR");
			break;
		case VOLTAGEC:
			sprintf(buffer, "VOLTAGE");
			break;
		case TINIT:
			sprintf(buffer, "TINIT");
			break;
		case TDESTROY:
			sprintf(buffer, "TDESTROY");
			break;

		case ABSC:
			sprintf(buffer, "ABS");
			break;
		case ABSIC:
			sprintf(buffer, "ABSI");
			break;
		case SQRTC:
			sprintf(buffer, "SQRT");
			break;
		case EXPC:
			sprintf(buffer, "EXP");
			break;
		case SINC:
			sprintf(buffer, "SIN");
			break;
		case COSC:
			sprintf(buffer, "COS");
			break;
		case LOGC:
			sprintf(buffer, "LOG");
			break;
		case LOG10C:
			sprintf(buffer, "LOG10");
			break;
		case ASINC:
			sprintf(buffer, "ASIN");
			break;
		case RANDC:
			sprintf(buffer, "RAND");
			break;
		case ROUNDC:
			sprintf(buffer, "ROUND");
			break;

		case STRCPYC:
			sprintf(buffer, "STRCPY");
			break;
		case STRNCPYC:
			sprintf(buffer, "STRNCPY");
			break;
		case STRCATC:
			sprintf(buffer, "STRCAT");
			break;
		case STRNCATC:
			sprintf(buffer, "STRNCAT");
			break;
		case STRCMPC:
			sprintf(buffer, "STRCMP");
			break;
		case STRNCMPC:
			sprintf(buffer, "STRNCMP");
			break;
		case STRSTRC:
			sprintf(buffer, "STRSTR");
			break;
		case STRLENC:
			sprintf(buffer, "STRLENC");
			break;

		case BEGINIT:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "BEGINIT");
					break;
				case 1:
					sprintf(buffer, "n");
					break;
			}
			break;
		case STRUCTWITHARR:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "STRUCTWITHARR");
					break;
				case 1:
					sprintf(buffer, "displ");
					break;
				case 2:
					sprintf(buffer, "iniproc");
					break;
			}
			break;
		case DEFARR:
			argc = 7;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "DEFARR");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "iniproc");
					break;
				case 5:
					sprintf(buffer, "usual");
					break;
				case 6:
					sprintf(buffer, "all");
					break;
				case 7:
					sprintf(buffer, "instruct");
					break;
			}
			break;
		case ARRINIT:
			argc = 4;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "ARRINIT");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
				case 2:
					sprintf(buffer, "elem_len");
					break;
				case 3:
					sprintf(buffer, "displ");
					break;
				case 4:
					sprintf(buffer, "usual");
					break;
			}
			break;
		/*
		case STRUCTINIT:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "STRUCTINIT");
					break;
				case 1:
					sprintf(buffer, "N");
					break;
			}
			break;
		*/
		case LI:
			argc = 1;
			sprintf(buffer, "LI");
			break;
		case LID:
			argc = 2;
			sprintf(buffer, "LID");
			break;
		case LOAD:
			argc = 1;
			sprintf(buffer, "LOAD");
			break;
		case LOADD:
			argc = 1;
			sprintf(buffer, "LOADD");
			break;
		case LAT:
			sprintf(buffer, "L@");
			break;
		case LATD:
			sprintf(buffer, "L@f");
			break;
		case LA:
			argc = 1;
			sprintf(buffer, "LA");
			break;

		case LOGOR:
			argc = 1;
			sprintf(buffer, "||");
			break;
		case LOGAND:
			argc = 1;
			sprintf(buffer, "&&");
			break;
		case ORASS:
			argc = 1;
			sprintf(buffer, "|=");
			break;
		case ORASSAT:
			sprintf(buffer, "|=@");
			break;
		case ORASSV:
			argc = 1;
			sprintf(buffer, "|=V");
			break;
		case ORASSATV:
			sprintf(buffer, "|=@V");
			break;
		case LOR:
			sprintf(buffer, "|");
			break;
		case EXORASS:
			argc = 1;
			sprintf(buffer, "^=");
			break;
		case EXORASSAT:
			sprintf(buffer, "^=@");
			break;
		case EXORASSV:
			argc = 1;
			sprintf(buffer, "^=V");
			break;
		case EXORASSATV:
			sprintf(buffer, "^=@V");
			break;
		case LEXOR:
			sprintf(buffer, "^");
			break;
		case ANDASS:
			argc = 1;
			sprintf(buffer, "&=");
			break;
		case ANDASSAT:
			sprintf(buffer, "&=@");
			break;
		case ANDASSV:
			argc = 1;
			sprintf(buffer, "&=V");
			break;
		case ANDASSATV:
			sprintf(buffer, "&=@V");
			break;
		case LAND:
			sprintf(buffer, "&");
			break;

		case EQEQ:
			sprintf(buffer, "==");
			break;
		case NOTEQ:
			sprintf(buffer, "!=");
			break;
		case LLT:
			sprintf(buffer, "<");
			break;
		case LGT:
			sprintf(buffer, ">");
			break;
		case LLE:
			sprintf(buffer, "<=");
			break;
		case LGE:
			sprintf(buffer, ">=");
			break;
		case EQEQR:
			sprintf(buffer, "==f");
			break;
		case NOTEQR:
			sprintf(buffer, "!=f");
			break;
		case LLTR:
			sprintf(buffer, "<f");
			break;
		case LGTR:
			sprintf(buffer, ">f");
			break;
		case LLER:
			sprintf(buffer, "<=f");
			break;
		case LGER:
			sprintf(buffer, ">=f");
			break;

		case SHRASS:
			argc = 1;
			sprintf(buffer, ">>=");
			break;
		case SHRASSAT:
			sprintf(buffer, ">>=@");
			break;
		case SHRASSV:
			argc = 1;
			sprintf(buffer, ">>=V");
			break;
		case SHRASSATV:
			sprintf(buffer, ">>=@V");
			break;
		case LSHR:
			sprintf(buffer, ">>");
			break;
		case SHLASS:
			argc = 1;
			sprintf(buffer, "<<=");
			break;
		case SHLASSAT:
			sprintf(buffer, "<<=@");
			break;
		case SHLASSV:
			argc = 1;
			sprintf(buffer, "<<=V");
			break;
		case SHLASSATV:
			sprintf(buffer, "<<=@V");
			break;
		case LSHL:
			sprintf(buffer, "<<");
			break;

		case ASS:
			argc = 1;
			sprintf(buffer, "=");
			break;
		case ASSAT:
			sprintf(buffer, "=@");
			break;
		case ASSV:
			argc = 1;
			sprintf(buffer, "=V");
			break;
		case ASSATV:
			sprintf(buffer, "=@V");
			break;

		case PLUSASS:
			argc = 1;
			sprintf(buffer, "+=");
			break;
		case PLUSASSAT:
			sprintf(buffer, "+=@");
			break;
		case PLUSASSV:
			argc = 1;
			sprintf(buffer, "+=V");
			break;
		case PLUSASSATV:
			sprintf(buffer, "+=@V");
			break;
		case LPLUS:
			sprintf(buffer, "+");
			break;

		case MINUSASS:
			argc = 1;
			sprintf(buffer, "-=");
			break;
		case MINUSASSAT:
			sprintf(buffer, "-=@");
			break;
		case MINUSASSV:
			argc = 1;
			sprintf(buffer, "-=V");
			break;
		case MINUSASSATV:
			sprintf(buffer, "-=@V");
			break;
		case LMINUS:
			sprintf(buffer, "-");
			break;

		case MULTASS:
			argc = 1;
			sprintf(buffer, "*=");
			break;
		case MULTASSAT:
			sprintf(buffer, "*=@");
			break;
		case MULTASSV:
			argc = 1;
			sprintf(buffer, "*=V");
			break;
		case MULTASSATV:
			sprintf(buffer, "*=@V");
			break;
		case LMULT:
			sprintf(buffer, "*");
			break;

		case DIVASS:
			argc = 1;
			sprintf(buffer, "/=");
			break;
		case DIVASSAT:
			sprintf(buffer, "/=@");
			break;
		case DIVASSV:
			argc = 1;
			sprintf(buffer, "/=V");
			break;
		case DIVASSATV:
			sprintf(buffer, "/=@V");
			break;
		case LDIV:
			sprintf(buffer, "/");
			break;

		case ASSR:
			argc = 1;
			sprintf(buffer, "=f");
			break;
		case ASSRV:
			argc = 1;
			sprintf(buffer, "=fV");
			break;
		case ASSATR:
			sprintf(buffer, "=@f");
			break;
		case ASSATRV:
			sprintf(buffer, "=@fV");
			break;

		case PLUSASSR:
			argc = 1;
			sprintf(buffer, "+=f");
			break;
		case PLUSASSATR:
			sprintf(buffer, "+=@f");
			break;
		case PLUSASSRV:
			argc = 1;
			sprintf(buffer, "+=fV");
			break;
		case PLUSASSATRV:
			sprintf(buffer, "+=@fV");
			break;
		case LPLUSR:
			sprintf(buffer, "+f");
			break;
		case MINUSASSR:
			argc = 1;
			sprintf(buffer, "-=f");
			break;
		case MINUSASSATR:
			sprintf(buffer, "-=@f");
			break;
		case MINUSASSRV:
			argc = 1;
			sprintf(buffer, "-=fV");
			break;
		case MINUSASSATRV:
			sprintf(buffer, "-=@fV");
			break;
		case LMINUSR:
			sprintf(buffer, "-f");
			break;
		case MULTASSR:
			argc = 1;
			sprintf(buffer, "*=f");
			break;
		case MULTASSATR:
			sprintf(buffer, "*=@f");
			break;
		case MULTASSRV:
			argc = 1;
			sprintf(buffer, "*=fV");
			break;
		case MULTASSATRV:
			sprintf(buffer, "*=@fV");
			break;
		case LMULTR:
			sprintf(buffer, "*f");
			break;
		case DIVASSR:
			argc = 1;
			sprintf(buffer, "/=f");
			break;
		case DIVASSATR:
			sprintf(buffer, "/=@f");
			break;
		case DIVASSRV:
			argc = 1;
			sprintf(buffer, "/=fV");
			break;
		case DIVASSATRV:
			sprintf(buffer, "/=@fV");
			break;
		case LDIVR:
			sprintf(buffer, "/f");
			break;

		case REMASS:
			argc = 1;
			sprintf(buffer, "%%=");
			break;
		case REMASSAT:
			sprintf(buffer, "%%=@");
			break;
		case REMASSV:
			argc = 1;
			sprintf(buffer, "%%=V");
			break;
		case REMASSATV:
			sprintf(buffer, "%%=@V");
			break;
		case LREM:
			sprintf(buffer, "%%");
			break;

		case CALL1:
			sprintf(buffer, "CALL1");
			break;
		case CALL2:
			argc = 1;
			sprintf(buffer, "CALL2");
			break;
		case STOP:
			sprintf(buffer, "STOP");
			break;
		case RETURNVAL:
			argc = 1;
			sprintf(buffer, "RETURNVAL");
			break;
		case RETURNVOID:
			sprintf(buffer, "RETURNVOID");
			break;
		case B:
			argc = 1;
			sprintf(buffer, "B");
			break;
		case BE0:
			argc = 1;
			sprintf(buffer, "BE0");
			break;
		case BNE0:
			argc = 1;
			sprintf(buffer, "BNE0");
			break;
		case SLICE:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SLICE");
					break;
				case 1:
					sprintf(buffer, "d");
					break;
			}
			break;
		case SELECT:
			argc = 1;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "SELECT");
					break;
				case 1:
					sprintf(buffer, "field_displ");
					break;
			}
			break;
		case WIDEN:
			sprintf(buffer, "WIDEN");
			break;
		case WIDEN1:
			sprintf(buffer, "WIDEN1");
			break;
		case _DOUBLE:
			sprintf(buffer, "DOUBLE");
			break;
		case INC:
			argc = 1;
			sprintf(buffer, "INC");
			break;
		case DEC:
			argc = 1;
			sprintf(buffer, "DEC");
			break;
		case POSTINC:
			argc = 1;
			sprintf(buffer, "POSTINC");
			break;
		case POSTDEC:
			argc = 1;
			sprintf(buffer, "POSTDEC");
			break;
		case INCAT:
			sprintf(buffer, "INC@");
			break;
		case DECAT:
			sprintf(buffer, "DEC@");
			break;
		case POSTINCAT:
			sprintf(buffer, "POSTINC@");
			break;
		case POSTDECAT:
			sprintf(buffer, "POSTDEC@");
			break;
		case INCR:
			argc = 1;
			sprintf(buffer, "INCf");
			break;
		case DECR:
			argc = 1;
			sprintf(buffer, "DECf");
			break;
		case POSTINCR:
			argc = 1;
			sprintf(buffer, "POSTINCf");
			break;
		case POSTDECR:
			argc = 1;
			sprintf(buffer, "POSTDECf");
			break;
		case INCATR:
			sprintf(buffer, "INC@f");
			break;
		case DECATR:
			sprintf(buffer, "DEC@f");
			break;
		case POSTINCATR:
			sprintf(buffer, "POSTINC@f");
			break;
		case POSTDECATR:
			sprintf(buffer, "POSTDEC@f");
			break;
		case INCV:
			argc = 1;
			sprintf(buffer, "INCV");
			break;
		case DECV:
			argc = 1;
			sprintf(buffer, "DECV");
			break;
		case POSTINCV:
			argc = 1;
			sprintf(buffer, "POSTINCV");
			break;
		case POSTDECV:
			argc = 1;
			sprintf(buffer, "POSTDECV");
			break;
		case INCATV:
			sprintf(buffer, "INC@V");
			break;
		case DECATV:
			sprintf(buffer, "DEC@V");
			break;
		case POSTINCATV:
			sprintf(buffer, "POSTINC@V");
			break;
		case POSTDECATV:
			sprintf(buffer, "POSTDEC@V");
			break;
		case INCRV:
			argc = 1;
			sprintf(buffer, "INCfV");
			break;
		case DECRV:
			argc = 1;
			sprintf(buffer, "DECfV");
			break;
		case POSTINCRV:
			argc = 1;
			sprintf(buffer, "POSTINCfV");
			break;
		case POSTDECRV:
			argc = 1;
			sprintf(buffer, "POSTDECfV");
			break;
		case INCATRV:
			sprintf(buffer, "INC@fV");
			break;
		case DECATRV:
			sprintf(buffer, "DEC@fV");
			break;
		case POSTINCATRV:
			sprintf(buffer, "POSTINC@fV");
			break;
		case POSTDECATRV:
			sprintf(buffer, "POSTDEC@fV");
			break;

		case LNOT:
			sprintf(buffer, "BITNOT");
			break;
		case LOGNOT:
			sprintf(buffer, "NOT");
			break;
		case UNMINUS:
			sprintf(buffer, "UNMINUS");
			break;
		case UNMINUSR:
			sprintf(buffer, "UNMINUSf");
			break;

		case FUNCBEG:
			argc = 2;
			was_switch = 1;
			switch (num)
			{
				case 0:
					sprintf(buffer, "FUNCBEG");
					break;
				case 1:
					sprintf(buffer, "maxdispl");
					break;
				case 2:
					sprintf(buffer, "pc");
					break;
			}
			break;

		default:
			sprintf(buffer, "%" PRIitem, elem);
			break;
	}

	if ((num != 0 && !was_switch) || argc < num)
	{
		buffer[0] = '\0';
	}
	return argc;
}


void double_to_io(universal_io *const io, const int64_t fst, const int64_t snd)
{
	int64_t num = (snd << 32) | (fst & 0x00000000ffffffff);
	double numdouble;
	memcpy(&numdouble, &num, sizeof(double));
	uni_printf(io, " %f\n", numdouble);
}

size_t elem_to_io(universal_io *const io, const vector *const table, size_t i)
{
	const item_t type = vector_get(table, i++);

	char buffer[MAX_ELEM_SIZE];
	size_t argc = elem_get_name(type, 0, buffer);
	uni_printf(io, "%s", buffer);

	if (type == TConstd || type == LID)
	{
		double_to_io(io, vector_get(table, i), vector_get(table, i + 1));
		return i + 2;
	}

	for (size_t j = 1; j <= argc; j++)
	{
		elem_get_name(type, j, buffer);

		if (buffer[0] != '\0')
		{
			uni_printf(io, " %s=", buffer);
		}

		uni_printf(io, " %" PRIitem, vector_get(table, i++));
	}
	uni_printf(io, "\n");

	if (type == TString)
	{
		const size_t n = (size_t)vector_get(table, i - 1);
		for (size_t j = 0; j < n; j++)
		{
			uni_printf(io, "%" PRIitem "\n", vector_get(table, i++));
		}
	}
	else if (type == TStringd)
	{
		const size_t n = (size_t)vector_get(table, i - 1);
		for (size_t j = 0; j < n; j++)
		{
			double_to_io(io, vector_get(table, i), vector_get(table, i + 1));
			i += 2;
		}
	}

	return i;
}


size_t tree_print_recursive(universal_io *const io, node *const nd, size_t index, size_t tabs)
{
	for (size_t i = 0; i < tabs; i++)
	{
		uni_printf(io, INDENT);
	}
	uni_printf(io, "tc %zi) ", index);

	const item_t type = node_get_type(nd);
	char buffer[MAX_ELEM_SIZE];
	size_t argc = elem_get_name(type, 0, buffer);
	uni_printf(io, "%s", buffer);

	if (type == TConstd || type == LID)
	{
		double_to_io(io, node_get_arg(nd, 0), node_get_arg(nd, 1));
	}
	else
	{
		size_t i = 0;
		while (i < argc && node_get_arg(nd, i) != ITEM_MAX)
		{
			elem_get_name(type, i + 1, buffer);

			if (buffer[0] != '\0')
			{
				uni_printf(io, " %s=", buffer);
			}

			uni_printf(io, " %" PRIitem, node_get_arg(nd, i++));
		}
		uni_printf(io, "\n");

		if ((node_get_arg(nd, i) != ITEM_MAX && node_get_type(nd) != TString && node_get_type(nd) != TStringd)
			|| i != argc)
		{
			elem_get_name(type, 0, buffer);
			warning(NULL, node_argc, index, buffer);
		}
	}

	index += argc + 1;
	for (size_t j = 0; j < node_get_amount(nd); j++)
	{
		node child = node_get_child(nd, j);
		index = tree_print_recursive(io, &child, index, tabs + 1);
	}

	return index;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


/** Вывод таблиц и дерева */
void tables_and_tree(const char *const path
	, const vector *const identifiers
	, const vector *const modes
	, vector *const tree)
{
	universal_io io = io_create();
	if (!vector_is_correct(identifiers) || !vector_is_correct(modes) || !vector_is_correct(tree)
		|| out_set_file(&io, path))
	{
		return;
	}


	uni_printf(&io, "identab\n");
	for (size_t i = 2; i < vector_size(identifiers); i += 4)
	{
		for (size_t j = 0; j < 4; j++)
		{
			uni_printf(&io, "id %zi) %" PRIitem "\n", i + j, vector_get(identifiers, i + j));
		}
		uni_printf(&io, "\n");
	}

	uni_printf(&io, "\nmodetab\n");
	for (size_t i = 0; i < vector_size(modes); i++)
	{
		uni_printf(&io, "md %zi) %" PRIitem "\n", i, vector_get(modes, i));
	}

	uni_printf(&io, "\n\ntree\n");
	size_t i = 0;
#ifdef OLD_TREE
	while (i < vector_size(tree))
	{
		uni_printf(&io, "tc %zi) ", i);
		i = elem_to_io(&io, tree, i);
	}
#else
	node nd = node_get_root(tree);
	for (size_t j = 0; j < node_get_amount(&nd); j++)
	{
		node child = node_get_child(&nd, j);
		i = tree_print_recursive(&io, &child, i, 0);
	}
#endif

	io_erase(&io);
}

/** Вывод таблиц и кодов */
void tables_and_codes(const char *const path
	, const vector *const functions
	, const vector *const processes
	, const vector *const memory)
{
	universal_io io = io_create();
	if (!vector_is_correct(functions) || !vector_is_correct(processes) || !vector_is_correct(memory)
		|| out_set_file(&io, path))
	{
		return;
	}


	uni_printf(&io, "functions\n");
	for (size_t i = 0; i < vector_size(functions); i++)
	{
		uni_printf(&io, "fun %zi) %" PRIitem "\n", i, vector_get(functions, i));
	}

	uni_printf(&io, "\n\niniprocs\n");
	for (size_t i = 0; i < vector_size(processes); i++)
	{
		uni_printf(&io, "inipr %zi) %" PRIitem "\n", i, vector_get(processes, i));
	}

	uni_printf(&io, "\n\nmem\n");
	size_t i = 0;
	while (i < vector_size(memory))
	{
		uni_printf(&io, "pc %zi) ", i);
		i = elem_to_io(&io, memory, i);
	}

	io_erase(&io);
}
