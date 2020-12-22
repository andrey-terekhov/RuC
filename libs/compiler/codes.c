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
#include "uniio.h"
#include "uniprinter.h"
#include <string.h>


size_t node_type_name(const int elem, const size_t num, char *const buffer)
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

		default:
			sprintf(buffer, "TOper %i", elem);
	}

	if ((num != 0 && !was_switch) || argc < num)
	{
		buffer[0] = '\0';
	}
	return argc;
}


/** Вывод таблиц и дерева */
void tables_and_tree(const syntax *const sx, const char *const path)
{
	universal_io io = io_create();
	if (sx == NULL || out_set_file(&io, path))
	{
		return;
	}

	uni_printf(&io, "\n%s\n", "identab");
	for (int i = 2; i < sx->id; i += 4)
	{
		for (int j = 0; j < 4; j++)
		{
			uni_printf(&io, "id %i) %i\n", i + j, sx->identab[i + j]);
		}
		uni_printf(&io, "\n");
	}

	/*
	uni_printf(&io, "\n%s\n", "repr");
	for (int i = 1206; i <= sx->rp; i++)
	{
		uni_printf(&io, "rp %i) %i\n", i, sx->reprtab[i]);
	}
	*/

	uni_printf(&io, "\n%s\n", "modetab");
	for (int i = 0; i < sx->md; i++)
	{
		uni_printf(&io, "md %i) %i\n", i, sx->modetab[i]);
	}

	/*
	uni_printf(&io, "\n%s\n", "tree");
	for (int i = 0; i <= tc; i++)
	{
		uni_printf(&io, "tc %i) %i\n", i, sx->tree[i]);
	}
	*/

	uni_printf(&io, "\n");

	int i = 0;
	while (i < sx->tc)
	{
		uni_printf(&io, "tc %i) ", i);
		const int type = sx->tree[i++];

		char buffer[32];
		size_t argc = node_type_name(type, 0, buffer);
		uni_printf(&io, "%s", buffer);

		if (type == TConstd)
		{
			double numdouble;
			memcpy(&numdouble, &sx->tree[i], sizeof(double));
			i += 2;
			uni_printf(&io, " %f\n", numdouble);
			continue;
		}

		for (size_t j = 1; j <= argc; j++)
		{
			node_type_name(type, j, buffer);

			if (buffer[0] != '\0')
			{
				uni_printf(&io, " %s=", buffer);
			}

			uni_printf(&io, " %i", sx->tree[i++]);
		}
		uni_printf(&io, "\n");

		if (type == TString)
		{
			const size_t n = sx->tree[i - 1];
			for (size_t j = 0; j < n; j++)
			{
				uni_printf(&io, "%i\n", sx->tree[i++]);
			}
		}
		else if (type == TStringd)
		{
			const size_t n = sx->tree[i - 1] * 2;
			for (size_t j = 0; j < n; j++)
			{
				double d;
				memcpy(&d, &sx->tree[i], sizeof(double));
				i += 2;
				uni_printf(&io, "%f\n", d);
			}
		}
	}

	io_erase(&io);
}

/** Вывод таблиц и кодов */
void tables_and_codes(const syntax *const sx, const char *const path)
{
	universal_io io = io_create();
	if (sx == NULL || out_set_file(&io, path))
	{
		return;
	}

	uni_printf(&io, "\n\n%s\n", "functions");
	for (int i = 1; i <= sx->funcnum; i++)
	{
		uni_printf(&io, "fun %i) %i\n", i, sx->functions[i]);
	}

	uni_printf(&io, "\n%s\n", "iniprocs");
	for (int i = 1; i <= sx->procd; i++)
	{
		uni_printf(&io, "inipr %i) %i\n", i, sx->iniprocs[i]);
	}

	uni_printf(&io, "\n%s\n", "mem");
	int i = 0;
	while (i < sx->pc)
	{
		uni_printf(&io, "pc %i) ", i);
		switch (sx->mem[i++])
		{
			case PRINT:
				uni_printf(&io, "PRINT %i\n", sx->mem[i++]);
				break;
			case PRINTID:
				uni_printf(&io, "PRINTID %i\n", sx->mem[i++]);
				break;
			case PRINTF:
				uni_printf(&io, "PRINTF %i\n", sx->mem[i++]);
				break;
			case GETID:
				uni_printf(&io, "GETID %i\n", sx->mem[i++]);
				break;
			case SETMOTORC:
				uni_printf(&io, "SETMOTOR\n");
				break;
			case GETDIGSENSORC:
				uni_printf(&io, "GETDIGSENSOR\n");
				break;
			case GETANSENSORC:
				uni_printf(&io, "GETANSENSOR\n");
				break;
			case VOLTAGEC:
				uni_printf(&io, "VOLTAGE\n");
				break;
			case CREATEC:
				uni_printf(&io, "TCREATE\n");
				break;
			case CREATEDIRECTC:
				uni_printf(&io, "TCREATEDIRECT\n");
				break;
			case MSGSENDC:
				uni_printf(&io, "TMSGSEND\n");
				break;
			case EXITC:
				uni_printf(&io, "TEXIT\n");
				break;
			case EXITDIRECTC:
				uni_printf(&io, "TEXITDIRECT\n");
				break;
			case MSGRECEIVEC:
				uni_printf(&io, "TMSGRECEIVE\n");
				break;
			case JOINC:
				uni_printf(&io, "TJOIN\n");
				break;
			case SLEEPC:
				uni_printf(&io, "TSLEEP\n");
				break;
			case SEMCREATEC:
				uni_printf(&io, "TSEMCREATE\n");
				break;
			case SEMWAITC:
				uni_printf(&io, "TSEMWAIT\n");
				break;
			case SEMPOSTC:
				uni_printf(&io, "TSEMPOST\n");
				break;
			case TINIT:
				uni_printf(&io, "TINIT\n");
				break;
			case TDESTROY:
				uni_printf(&io, "TDESTROY\n");
				break;
			case GETNUMC:
				uni_printf(&io, "GETNUM\n");
				break;

			case ABSC:
				uni_printf(&io, "ABS\n");
				break;
			case ABSIC:
				uni_printf(&io, "ABSI\n");
				break;
			case SQRTC:
				uni_printf(&io, "SQRT\n");
				break;
			case EXPC:
				uni_printf(&io, "EXP\n");
				break;
			case SINC:
				uni_printf(&io, "SIN\n");
				break;
			case COSC:
				uni_printf(&io, "COS\n");
				break;
			case LOGC:
				uni_printf(&io, "LOG\n");
				break;
			case LOG10C:
				uni_printf(&io, "LOG10\n");
				break;
			case ASINC:
				uni_printf(&io, "ASIN\n");
				break;
			case RANDC:
				uni_printf(&io, "RAND\n");
				break;
			case ROUNDC:
				uni_printf(&io, "ROUND\n");
				break;

			case STRCPYC:
				uni_printf(&io, "STRCPY\n");
				break;
			case STRNCPYC:
				uni_printf(&io, "STRNCPY\n");
				break;
			case STRCATC:
				uni_printf(&io, "STRCAT\n");
				break;
			case STRNCATC:
				uni_printf(&io, "STRNCAT\n");
				break;
			case STRCMPC:
				uni_printf(&io, "STRCMP\n");
				break;
			case STRNCMPC:
				uni_printf(&io, "STRNCMP\n");
				break;
			case STRSTRC:
				uni_printf(&io, "STRSTR\n");
				break;
			case STRLENC:
				uni_printf(&io, "STRLENC\n");
				break;

			case BEGINIT:
				uni_printf(&io, "BEGINIT n= %i\n", sx->mem[i++]);
				break;
			case STRUCTWITHARR:
				uni_printf(&io, "STRUCTWITHARR displ= %i ", sx->mem[i++]);
				uni_printf(&io, "iniproc= %i\n", sx->mem[i++]);
				break;
			case DEFARR:
				uni_printf(&io, "DEFARR N= %i ",
							   sx->mem[i++]); // N
				uni_printf(&io, "elem_len= %i ",
							   sx->mem[i++]); // elem length
				uni_printf(&io, "displ= %i ",
							   sx->mem[i++]); // displ
				uni_printf(&io, "iniproc= %i ",
							   sx->mem[i++]); // iniproc
				uni_printf(&io, "usual= %i ",
							   sx->mem[i++]); // usual
				uni_printf(&io, "all= %i ",
							   sx->mem[i++]); // all
				uni_printf(&io, "instruct= %i\n",
							   sx->mem[i++]); // instruct
				break;
			case ARRINIT:
				uni_printf(&io, "ARRINIT N= %i ", sx->mem[i++]);
				uni_printf(&io, "elem_len= %i ", sx->mem[i++]);
				uni_printf(&io, "displ= %i ", sx->mem[i++]);
				uni_printf(&io, "usual= %i\n", sx->mem[i++]);
				break;
			/*
			case STRUCTINIT:
				uni_printf(&io,	"STRUCTINIT N= %i ", sx->mem[i++]);
				break;
			*/
			case NOP:
				uni_printf(&io, "NOP\n");
				break;
			case LI:
				uni_printf(&io, "LI %i\n", sx->mem[i++]);
				break;
			case LID:
			{
				double numdouble;
				memcpy(&numdouble, &sx->mem[i], sizeof(double));
				i += 2;
				uni_printf(&io, "LID %.15f\n", numdouble);
			}
			break;
			case LOAD:
				uni_printf(&io, "LOAD %i\n", sx->mem[i++]);
				break;
			case LOADD:
				uni_printf(&io, "LOADD %i\n", sx->mem[i++]);
				break;
			case LAT:
				uni_printf(&io, "L@\n");
				break;
			case LATD:
				uni_printf(&io, "L@f\n");
				break;
			case LA:
				uni_printf(&io, "LA %i\n", sx->mem[i++]);
				break;

			case LOGOR:
				uni_printf(&io, "||\n");
				break;
			case LOGAND:
				uni_printf(&io, "&&\n");
				break;
			case ORASS:
				uni_printf(&io, "|= %i\n", sx->mem[i++]);
				break;
			case ORASSAT:
				uni_printf(&io, "|=@\n");
				break;
			case ORASSV:
				uni_printf(&io, "|=V %i\n", sx->mem[i++]);
				break;
			case ORASSATV:
				uni_printf(&io, "|=@V\n");
				break;
			case LOR:
				uni_printf(&io, "|\n");
				break;
			case EXORASS:
				uni_printf(&io, "^= %i\n", sx->mem[i++]);
				break;
			case EXORASSAT:
				uni_printf(&io, "^=@\n");
				break;
			case EXORASSV:
				uni_printf(&io, "^=V %i\n", sx->mem[i++]);
				break;
			case EXORASSATV:
				uni_printf(&io, "^=@V\n");
				break;
			case LEXOR:
				uni_printf(&io, "^\n");
				break;
			case ANDASS:
				uni_printf(&io, "&= %i\n", sx->mem[i++]);
				break;
			case ANDASSAT:
				uni_printf(&io, "&=@\n");
				break;
			case ANDASSV:
				uni_printf(&io, "&=V %i\n", sx->mem[i++]);
				break;
			case ANDASSATV:
				uni_printf(&io, "&=@V\n");
				break;
			case LAND:
				uni_printf(&io, "&\n");
				break;

			case EQEQ:
				uni_printf(&io, "==\n");
				break;
			case NOTEQ:
				uni_printf(&io, "!=\n");
				break;
			case LLT:
				uni_printf(&io, "<\n");
				break;
			case LGT:
				uni_printf(&io, ">\n");
				break;
			case LLE:
				uni_printf(&io, "<=\n");
				break;
			case LGE:
				uni_printf(&io, ">=\n");
				break;
			case EQEQR:
				uni_printf(&io, "==f\n");
				break;
			case NOTEQR:
				uni_printf(&io, "!=f\n");
				break;
			case LLTR:
				uni_printf(&io, "<f\n");
				break;
			case LGTR:
				uni_printf(&io, ">f\n");
				break;
			case LLER:
				uni_printf(&io, "<=f\n");
				break;
			case LGER:
				uni_printf(&io, ">=f\n");
				break;

			case SHRASS:
				uni_printf(&io, ">>= %i\n", sx->mem[i++]);
				break;
			case SHRASSAT:
				uni_printf(&io, ">>=@\n");
				break;
			case SHRASSV:
				uni_printf(&io, ">>=V %i\n", sx->mem[i++]);
				break;
			case SHRASSATV:
				uni_printf(&io, ">>=@V\n");
				break;
			case LSHR:
				uni_printf(&io, ">>\n");
				break;
			case SHLASS:
				uni_printf(&io, "<<= %i\n", sx->mem[i++]);
				break;
			case SHLASSAT:
				uni_printf(&io, "<<=@\n");
				break;
			case SHLASSV:
				uni_printf(&io, "<<=V %i\n", sx->mem[i++]);
				break;
			case SHLASSATV:
				uni_printf(&io, "<<=@V\n");
				break;
			case LSHL:
				uni_printf(&io, "<<\n");
				break;

			case ASS:
				uni_printf(&io, "= %i\n", sx->mem[i++]);
				break;
			case ASSAT:
				uni_printf(&io, "=@\n");
				break;
			case ASSV:
				uni_printf(&io, "=V %i\n", sx->mem[i++]);
				break;
			case ASSATV:
				uni_printf(&io, "=@V\n");
				break;

			case PLUSASS:
				uni_printf(&io, "+= %i\n", sx->mem[i++]);
				break;
			case PLUSASSAT:
				uni_printf(&io, "+=@\n");
				break;
			case PLUSASSV:
				uni_printf(&io, "+=V %i\n", sx->mem[i++]);
				break;
			case PLUSASSATV:
				uni_printf(&io, "+=@V\n");
				break;
			case LPLUS:
				uni_printf(&io, "+\n");
				break;

			case MINUSASS:
				uni_printf(&io, "-= %i\n", sx->mem[i++]);
				break;
			case MINUSASSAT:
				uni_printf(&io, "-=@\n");
				break;
			case MINUSASSV:
				uni_printf(&io, "-=V %i\n", sx->mem[i++]);
				break;
			case MINUSASSATV:
				uni_printf(&io, "-=@V\n");
				break;
			case LMINUS:
				uni_printf(&io, "-\n");
				break;

			case MULTASS:
				uni_printf(&io, "*= %i\n", sx->mem[i++]);
				break;
			case MULTASSAT:
				uni_printf(&io, "*=@\n");
				break;
			case MULTASSV:
				uni_printf(&io, "*=V %i\n", sx->mem[i++]);
				break;
			case MULTASSATV:
				uni_printf(&io, "*=@V\n");
				break;
			case LMULT:
				uni_printf(&io, "*\n");
				break;

			case DIVASS:
				uni_printf(&io, "/= %i\n", sx->mem[i++]);
				break;
			case DIVASSAT:
				uni_printf(&io, "/=@\n");
				break;
			case DIVASSV:
				uni_printf(&io, "/=V %i\n", sx->mem[i++]);
				break;
			case DIVASSATV:
				uni_printf(&io, "/=@V\n");
				break;
			case LDIV:
				uni_printf(&io, "/\n");
				break;

			case ASSR:
				uni_printf(&io, "=f %i\n", sx->mem[i++]);
				break;
			case ASSRV:
				uni_printf(&io, "=fV %i\n", sx->mem[i++]);
				break;
			case ASSATR:
				uni_printf(&io, "=@f\n");
				break;
			case ASSATRV:
				uni_printf(&io, "=@fV\n");
				break;

			case PLUSASSR:
				uni_printf(&io, "+=f %i\n", sx->mem[i++]);
				break;
			case PLUSASSATR:
				uni_printf(&io, "+=@f\n");
				break;
			case PLUSASSRV:
				uni_printf(&io, "+=fV %i\n", sx->mem[i++]);
				break;
			case PLUSASSATRV:
				uni_printf(&io, "+=@fV\n");
				break;
			case LPLUSR:
				uni_printf(&io, "+f\n");
				break;
			case MINUSASSR:
				uni_printf(&io, "-=f %i\n", sx->mem[i++]);
				break;
			case MINUSASSATR:
				uni_printf(&io, "-=@f\n");
				break;
			case MINUSASSRV:
				uni_printf(&io, "-=fV %i\n", sx->mem[i++]);
				break;
			case MINUSASSATRV:
				uni_printf(&io, "-=@fV\n");
				break;
			case LMINUSR:
				uni_printf(&io, "-f\n");
				break;
			case MULTASSR:
				uni_printf(&io, "*=f %i\n", sx->mem[i++]);
				break;
			case MULTASSATR:
				uni_printf(&io, "*=@f\n");
				break;
			case MULTASSRV:
				uni_printf(&io, "*=fV %i\n", sx->mem[i++]);
				break;
			case MULTASSATRV:
				uni_printf(&io, "*=@fV\n");
				break;
			case LMULTR:
				uni_printf(&io, "*f\n");
				break;
			case DIVASSR:
				uni_printf(&io, "/=f %i\n", sx->mem[i++]);
				break;
			case DIVASSATR:
				uni_printf(&io, "/=@f\n");
				break;
			case DIVASSRV:
				uni_printf(&io, "/=fV %i\n", sx->mem[i++]);
				break;
			case DIVASSATRV:
				uni_printf(&io, "/=@fV\n");
				break;
			case LDIVR:
				uni_printf(&io, "/f\n");
				break;
			case COPY00:
				uni_printf(&io, "COPY00 %i ",
							   sx->mem[i++]); // displleft
				uni_printf(&io, "%i ",
							   sx->mem[i++]); // displright
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length
				break;
			case COPY01:
				uni_printf(&io, "COPY01 %i      ",
							   sx->mem[i++]); // displleft
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length
				break;
			case COPY10:
				uni_printf(&io, "COPY10      %i ",
							   sx->mem[i++]); // displright
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length
				break;
			case COPY11:
				uni_printf(&io, "COPY11 %i\n",
							   sx->mem[i++]); // length
				break;
			case COPY0ST:
				uni_printf(&io, "COPY0ST %i ",
							   sx->mem[i++]); // displright
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length
				break;
			case COPY1ST:
				uni_printf(&io, "COPY1ST %i\n",
							   sx->mem[i++]); // length
				break;
			case COPY0STASS:
				uni_printf(&io, "COPY0STASS %i ",
							   sx->mem[i++]); // displleft
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length
				break;
			case COPY1STASS:
				uni_printf(&io, "COPY1STASS %i\n",
							   sx->mem[i++]); // length
				break;
			case COPYST:
				uni_printf(&io, "COPYST %i ",
							   sx->mem[i++]); // displ
				uni_printf(&io, "(%i)",
							   sx->mem[i++]); // length
				uni_printf(&io, "(%i)\n",
							   sx->mem[i++]); // length1
				break;

			case REMASS:
				uni_printf(&io, "%%= %i\n", sx->mem[i++]);
				break;
			case REMASSAT:
				uni_printf(&io, "%%=@\n");
				break;
			case REMASSV:
				uni_printf(&io, "%%=V %i\n", sx->mem[i++]);
				break;
			case REMASSATV:
				uni_printf(&io, "%%=@V\n");
				break;
			case LREM:
				uni_printf(&io, "%%\n");
				break;

			case CALL1:
				uni_printf(&io, "CALL1\n");
				break;
			case CALL2:
				uni_printf(&io, "CALL2 ");
				uni_printf(&io, "%i\n", sx->mem[i++]);
				break;
			case STOP:
				uni_printf(&io, "STOP\n");
				break;
			case RETURNVAL:
				uni_printf(&io, "RETURNVAL %i\n", sx->mem[i++]);
				break;
			case RETURNVOID:
				uni_printf(&io, "RETURNVOID\n");
				break;
			case B:
				uni_printf(&io, "B %i\n", sx->mem[i++]);
				break;
			case BE0:
				uni_printf(&io, "BE0 %i\n", sx->mem[i++]);
				break;
			case BNE0:
				uni_printf(&io, "BNE0 %i\n", sx->mem[i++]);
				break;
			case SLICE:
				uni_printf(&io, "SLICE d= %i\n", sx->mem[i++]);
				break;
			case SELECT:
				uni_printf(&io, "SELECT field_displ= %i\n", sx->mem[i++]);
				break;
			case WIDEN:
				uni_printf(&io, "WIDEN\n");
				break;
			case WIDEN1:
				uni_printf(&io, "WIDEN1\n");
				break;
			case _DOUBLE:
				uni_printf(&io, "DOUBLE\n");
				break;
			case INC:
				uni_printf(&io, "INC %i\n", sx->mem[i++]);
				break;
			case DEC:
				uni_printf(&io, "DEC %i\n", sx->mem[i++]);
				break;
			case POSTINC:
				uni_printf(&io, "POSTINC %i\n", sx->mem[i++]);
				break;
			case POSTDEC:
				uni_printf(&io, "POSTDEC %i\n", sx->mem[i++]);
				break;
			case INCAT:
				uni_printf(&io, "INC@\n");
				break;
			case DECAT:
				uni_printf(&io, "DEC@\n");
				break;
			case POSTINCAT:
				uni_printf(&io, "POSTINC@\n");
				break;
			case POSTDECAT:
				uni_printf(&io, "POSTDEC@\n");
				break;
			case INCR:
				uni_printf(&io, "INCf %i\n", sx->mem[i++]);
				break;
			case DECR:
				uni_printf(&io, "DECf %i\n", sx->mem[i++]);
				break;
			case POSTINCR:
				uni_printf(&io, "POSTINCf %i\n", sx->mem[i++]);
				break;
			case POSTDECR:
				uni_printf(&io, "POSTDECf %i\n", sx->mem[i++]);
				break;
			case INCATR:
				uni_printf(&io, "INC@f\n");
				break;
			case DECATR:
				uni_printf(&io, "DEC@f\n");
				break;
			case POSTINCATR:
				uni_printf(&io, "POSTINC@f\n");
				break;
			case POSTDECATR:
				uni_printf(&io, "POSTDEC@f\n");
				break;
			case INCV:
				uni_printf(&io, "INCV %i\n", sx->mem[i++]);
				break;
			case DECV:
				uni_printf(&io, "DECV %i\n", sx->mem[i++]);
				break;
			case POSTINCV:
				uni_printf(&io, "POSTINCV %i\n", sx->mem[i++]);
				break;
			case POSTDECV:
				uni_printf(&io, "POSTDECV %i\n", sx->mem[i++]);
				break;
			case INCATV:
				uni_printf(&io, "INC@V\n");
				break;
			case DECATV:
				uni_printf(&io, "DEC@V\n");
				break;
			case POSTINCATV:
				uni_printf(&io, "POSTINC@V\n");
				break;
			case POSTDECATV:
				uni_printf(&io, "POSTDEC@V\n");
				break;
			case INCRV:
				uni_printf(&io, "INCfV %i\n", sx->mem[i++]);
				break;
			case DECRV:
				uni_printf(&io, "DECfV %i\n", sx->mem[i++]);
				break;
			case POSTINCRV:
				uni_printf(&io, "POSTINCfV %i\n", sx->mem[i++]);
				break;
			case POSTDECRV:
				uni_printf(&io, "POSTDECfV %i\n", sx->mem[i++]);
				break;
			case INCATRV:
				uni_printf(&io, "INC@fV\n");
				break;
			case DECATRV:
				uni_printf(&io, "DEC@fV\n");
				break;
			case POSTINCATRV:
				uni_printf(&io, "POSTINC@fV\n");
				break;
			case POSTDECATRV:
				uni_printf(&io, "POSTDEC@fV\n");
				break;

			case LNOT:
				uni_printf(&io, "BITNOT\n");
				break;
			case LOGNOT:
				uni_printf(&io, "NOT\n");
				break;
			case UNMINUS:
				uni_printf(&io, "UNMINUS\n");
				break;
			case UNMINUSR:
				uni_printf(&io, "UNMINUSf\n");
				break;

			case FUNCBEG:
				uni_printf(&io, "FUNCBEG maxdispl= %i ", sx->mem[i++]);
				uni_printf(&io, "pc= %i\n", sx->mem[i++]);
				break;


			default:
				uni_printf(&io, "%i\n", sx->mem[i - 1]);
		}
	}

	io_erase(&io);
}
