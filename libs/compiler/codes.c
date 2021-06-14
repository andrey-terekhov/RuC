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
#include "errors.h"
#include "old_tree.h"
#include "operations.h"
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
	bool was_switch = false;

	switch (elem)
	{
		case OP_FUNC_DEF:
			argc = 2;
			was_switch = true;
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
		case OP_DECL_ARR:
			argc = 1;
			was_switch = true;
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
		case OP_DECL_ID:
			argc = 7;
			was_switch = true;
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
		case OP_STRING:
			argc = 1;
			was_switch = true;
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
		case OP_STRING_D:
			argc = 1;
			was_switch = true;
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
		case OP_CONDITIONAL:
			sprintf(buffer, "TCondexpr");
			break;
		case OP_BLOCK:
			sprintf(buffer, "TBegin");
			break;
		case OP_BLOCK_END:
			sprintf(buffer, "TEnd");
			break;
		case OP_ARRAY_INIT:
			argc = 1;
			was_switch = true;
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
		case OP_STRUCT_INIT:
			argc = 1;
			was_switch = true;
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
		case OP_IF:
			argc = 1;
			sprintf(buffer, "TIf");
			break;
		case OP_WHILE:
			sprintf(buffer, "TWhile");
			break;
		case OP_DO:
			sprintf(buffer, "TDo");
			break;
		case OP_FOR:
			argc = 4;
			sprintf(buffer, "TFor");
			break;
		case OP_SWITCH:
			sprintf(buffer, "TSwitch");
			break;
		case OP_CASE:
			sprintf(buffer, "TCase");
			break;
		case OP_DEFAULT:
			sprintf(buffer, "TDefault");
			break;
		case OP_BREAK:
			sprintf(buffer, "TBreak");
			break;
		case OP_CONTINUE:
			sprintf(buffer, "TContinue");
			break;
		case OP_RETURN_VOID:
			sprintf(buffer, "TReturn");
			break;
		case OP_RETURN_VAL:
			argc = 1;
			sprintf(buffer, "TReturnval");
			break;
		case OP_GOTO:
			argc = 1;
			sprintf(buffer, "TGoto");
			break;
		case OP_IDENT:
			argc = 1;
			sprintf(buffer, "TIdent");
			break;
		case OP_IDENT_TO_VAL:
			argc = 1;
			sprintf(buffer, "TIdenttoval");
			break;
		case OP_IDENT_TO_VAL_D:
			argc = 1;
			sprintf(buffer, "TIdenttovald");
			break;
		case OP_IDENT_TO_ADDR:
			argc = 1;
			sprintf(buffer, "TIdenttoaddr");
			break;
		case OP_ADDR_TO_VAL:
			sprintf(buffer, "TAddrtoval");
			break;
		case OP_ADDR_TO_VAL_D:
			sprintf(buffer, "TAddrtovald");
			break;
		case OP_EXPR_END:
			sprintf(buffer, "TExprend");
			break;
		case OP_CONST:
			argc = 1;
			sprintf(buffer, "TConst");
			break;
		case OP_CONST_D:
			argc = 2;
			sprintf(buffer, "TConstd");
		break;
		case OP_SLICE_IDENT:
			argc = 2;
			was_switch = true;
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
		case OP_SLICE:
			argc = 1;
			was_switch = true;
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
		case OP_SELECT:
			argc = 1;
			was_switch = true;
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
		case OP_NOP:
			sprintf(buffer, "NOP");
			break;
		case OP_AD_LOG_AND:
			argc = 1;
			was_switch = true;
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
		case OP_AD_LOG_OR:
			argc = 1;
			was_switch = true;
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
		case OP_COPY00:
			argc = 3;
			was_switch = true;
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
		case OP_COPY01:
			argc = 2;
			was_switch = true;
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
		case OP_COPY10:
			argc = 2;
			was_switch = true;
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
		case OP_COPY11:
			argc = 1;
			was_switch = true;
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
		case OP_COPY0ST:
			argc = 2;
			was_switch = true;
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
		case OP_COPY1ST:
			argc = 1;
			was_switch = true;
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
		case OP_COPY0ST_ASSIGN:
			argc = 2;
			was_switch = true;
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
		case OP_COPY1ST_ASSIGN:
			argc = 1;
			was_switch = true;
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
		case OP_COPYST:
			argc = 3;
			was_switch = true;
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

		case OP_CALL1:
			argc = 1;
			sprintf(buffer, "TCall1");
			break;
		case OP_CALL2:
			argc = 1;
			sprintf(buffer, "TCall2");
			break;
		case OP_LABEL:
			argc = 1;
			sprintf(buffer, "TLabel");
			break;
		case OP_DECL_STRUCT:
			argc = 1;
			sprintf(buffer, "TStructbeg");
			break;
		case OP_DECL_STRUCT_END:
			argc = 1;
			sprintf(buffer, "TStructend");
			break;
		case OP_PRINT:
			argc = 1;
			sprintf(buffer, "TPrint");
			break;
		case OP_PRINTID:
			argc = 1;
			sprintf(buffer, "TPrintid");
			break;
		case OP_PRINTF:
			argc = 1;
			sprintf(buffer, "TPrintf");
			break;
		case OP_GETID:
			argc = 1;
			sprintf(buffer, "TGetid");
			break;
		case OP_CREATE:
			sprintf(buffer, "TCREATE");
			break;
		case OP_CREATE_DIRECT:
			sprintf(buffer, "TCREATEDIRECT");
			break;
		case OP_EXIT:
			sprintf(buffer, "TEXIT");
			break;
		case OP_EXIT_DIRECT:
			sprintf(buffer, "TEXITDIRECT");
			break;
		case OP_MSG_SEND:
			sprintf(buffer, "TMSGSEND");
			break;
		case OP_MSG_RECEIVE:
			sprintf(buffer, "TMSGRECEIVE");
			break;
		case OP_JOIN:
			sprintf(buffer, "TJOIN");
			break;
		case OP_SLEEP:
			sprintf(buffer, "TSLEEP");
			break;
		case OP_SEM_CREATE:
			sprintf(buffer, "TSEMCREATE");
			break;
		case OP_SEM_WAIT:
			sprintf(buffer, "TSEMWAIT");
			break;
		case OP_SEM_POST:
			sprintf(buffer, "TSEMPOST");
			break;
		case OP_INIT:
			sprintf(buffer, "INITC");
			break;
		case OP_DESTROY:
			sprintf(buffer, "DESTROYC");
			break;
		case OP_GETNUM:
			sprintf(buffer, "GETNUMC");
			break;

		case IC_PRINT:
			argc = 1;
			sprintf(buffer, "PRINT");
			break;
		case IC_PRINTID:
			argc = 1;
			sprintf(buffer, "PRINTID");
			break;
		case IC_PRINTF:
			argc = 1;
			sprintf(buffer, "PRINTF");
			break;
		case IC_GETID:
			argc = 1;
			sprintf(buffer, "GETID");
			break;

		case OP_ABS:
			sprintf(buffer, "ABS");
			break;
		case OP_ABSI:
			sprintf(buffer, "ABSI");
			break;
		case OP_SQRT:
			sprintf(buffer, "SQRT");
			break;
		case OP_EXP:
			sprintf(buffer, "EXP");
			break;
		case OP_SIN:
			sprintf(buffer, "SIN");
			break;
		case OP_COS:
			sprintf(buffer, "COS");
			break;
		case OP_LOG:
			sprintf(buffer, "LOG");
			break;
		case OP_LOG10:
			sprintf(buffer, "LOG10");
			break;
		case OP_ASIN:
			sprintf(buffer, "ASIN");
			break;
		case OP_RAND:
			sprintf(buffer, "RAND");
			break;
		case OP_ROUND:
			sprintf(buffer, "ROUND");
			break;

		case OP_STRCPY:
			sprintf(buffer, "STRCPY");
			break;
		case OP_STRNCPY:
			sprintf(buffer, "STRNCPY");
			break;
		case OP_STRCAT:
			sprintf(buffer, "STRCAT");
			break;
		case OP_STRNCAT:
			sprintf(buffer, "STRNCAT");
			break;
		case OP_STRCMP:
			sprintf(buffer, "STRCMP");
			break;
		case OP_STRNCMP:
			sprintf(buffer, "STRNCMP");
			break;
		case OP_STRSTR:
			sprintf(buffer, "STRSTR");
			break;
		case OP_STRLEN:
			sprintf(buffer, "STRLENC");
			break;

		case IC_BEG_INIT:
			argc = 1;
			was_switch = true;
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
		case IC_STRUCT_WITH_ARR:
			argc = 2;
			was_switch = true;
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
		case IC_DEFARR:
			argc = 7;
			was_switch = true;
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
		case IC_ARR_INIT:
			argc = 4;
			was_switch = true;
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
		case IC_LI:
			argc = 1;
			sprintf(buffer, "LI");
			break;
		case IC_LID:
			argc = 2;
			sprintf(buffer, "LID");
			break;
		case IC_LOAD:
			argc = 1;
			sprintf(buffer, "LOAD");
			break;
		case IC_LOADD:
			argc = 1;
			sprintf(buffer, "LOADD");
			break;
		case IC_LAT:
			sprintf(buffer, "L@");
			break;
		case IC_LATD:
			sprintf(buffer, "L@f");
			break;
		case IC_LA:
			argc = 1;
			sprintf(buffer, "LA");
			break;

		case OP_LOG_OR:
			argc = 1;
			sprintf(buffer, "||");
			break;
		case OP_LOG_AND:
			argc = 1;
			sprintf(buffer, "&&");
			break;
		case OP_OR_ASSIGN:
			argc = 1;
			sprintf(buffer, "|=");
			break;
		case OP_OR_ASSIGN_AT:
			sprintf(buffer, "|=@");
			break;
		case OP_OR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "|=V");
			break;
		case OP_OR_ASSIGN_AT_V:
			sprintf(buffer, "|=@V");
			break;
		case OP_OR:
			sprintf(buffer, "|");
			break;
		case OP_XOR_ASSIGN:
			argc = 1;
			sprintf(buffer, "^=");
			break;
		case OP_XOR_ASSIGN_AT:
			sprintf(buffer, "^=@");
			break;
		case OP_XOR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "^=V");
			break;
		case OP_XOR_ASSIGN_AT_V:
			sprintf(buffer, "^=@V");
			break;
		case OP_XOR:
			sprintf(buffer, "^");
			break;
		case OP_AND_ASSIGN:
			argc = 1;
			sprintf(buffer, "&=");
			break;
		case OP_AND_ASSIGN_AT:
			sprintf(buffer, "&=@");
			break;
		case OP_AND_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "&=V");
			break;
		case OP_AND_ASSIGN_AT_V:
			sprintf(buffer, "&=@V");
			break;
		case OP_AND:
			sprintf(buffer, "&");
			break;

		case OP_EQ:
			sprintf(buffer, "==");
			break;
		case OP_NE:
			sprintf(buffer, "!=");
			break;
		case OP_LT:
			sprintf(buffer, "<");
			break;
		case OP_GT:
			sprintf(buffer, ">");
			break;
		case OP_LE:
			sprintf(buffer, "<=");
			break;
		case OP_GE:
			sprintf(buffer, ">=");
			break;
		case OP_EQ_R:
			sprintf(buffer, "==f");
			break;
		case OP_NE_R:
			sprintf(buffer, "!=f");
			break;
		case OP_LT_R:
			sprintf(buffer, "<f");
			break;
		case OP_GT_R:
			sprintf(buffer, ">f");
			break;
		case OP_LE_R:
			sprintf(buffer, "<=f");
			break;
		case OP_GE_R:
			sprintf(buffer, ">=f");
			break;

		case OP_SHR_ASSIGN:
			argc = 1;
			sprintf(buffer, ">>=");
			break;
		case OP_SHR_ASSIGN_AT:
			sprintf(buffer, ">>=@");
			break;
		case OP_SHR_ASSIGN_V:
			argc = 1;
			sprintf(buffer, ">>=V");
			break;
		case OP_SHR_ASSIGN_AT_V:
			sprintf(buffer, ">>=@V");
			break;
		case OP_SHR:
			sprintf(buffer, ">>");
			break;
		case OP_SHL_ASSIGN:
			argc = 1;
			sprintf(buffer, "<<=");
			break;
		case OP_SHL_ASSIGN_AT:
			sprintf(buffer, "<<=@");
			break;
		case OP_SHL_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "<<=V");
			break;
		case OP_SHL_ASSIGN_AT_V:
			sprintf(buffer, "<<=@V");
			break;
		case OP_SHL:
			sprintf(buffer, "<<");
			break;

		case OP_ASSIGN:
			argc = 1;
			sprintf(buffer, "=");
			break;
		case OP_ASSIGN_AT:
			sprintf(buffer, "=@");
			break;
		case OP_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "=V");
			break;
		case OP_ASSIGN_AT_V:
			sprintf(buffer, "=@V");
			break;

		case OP_ADD_ASSIGN:
			argc = 1;
			sprintf(buffer, "+=");
			break;
		case OP_ADD_ASSIGN_AT:
			sprintf(buffer, "+=@");
			break;
		case OP_ADD_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "+=V");
			break;
		case OP_ADD_ASSIGN_AT_V:
			sprintf(buffer, "+=@V");
			break;
		case OP_ADD:
			sprintf(buffer, "+");
			break;

		case OP_SUB_ASSIGN:
			argc = 1;
			sprintf(buffer, "-=");
			break;
		case OP_SUB_ASSIGN_AT:
			sprintf(buffer, "-=@");
			break;
		case OP_SUB_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "-=V");
			break;
		case OP_SUB_ASSIGN_AT_V:
			sprintf(buffer, "-=@V");
			break;
		case OP_SUB:
			sprintf(buffer, "-");
			break;

		case OP_MUL_ASSIGN:
			argc = 1;
			sprintf(buffer, "*=");
			break;
		case OP_MUL_ASSIGN_AT:
			sprintf(buffer, "*=@");
			break;
		case OP_MUL_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "*=V");
			break;
		case OP_MUL_ASSIGN_AT_V:
			sprintf(buffer, "*=@V");
			break;
		case OP_MUL:
			sprintf(buffer, "*");
			break;

		case OP_DIV_ASSIGN:
			argc = 1;
			sprintf(buffer, "/=");
			break;
		case OP_DIV_ASSIGN_AT:
			sprintf(buffer, "/=@");
			break;
		case OP_DIV_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "/=V");
			break;
		case OP_DIV_ASSIGN_AT_V:
			sprintf(buffer, "/=@V");
			break;
		case OP_DIV:
			sprintf(buffer, "/");
			break;

		case OP_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "=f");
			break;
		case OP_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "=fV");
			break;
		case OP_ASSIGN_AT_R:
			sprintf(buffer, "=@f");
			break;
		case OP_ASSIGN_AT_R_V:
			sprintf(buffer, "=@fV");
			break;

		case OP_ADD_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "+=f");
			break;
		case OP_ADD_ASSIGN_AT_R:
			sprintf(buffer, "+=@f");
			break;
		case OP_ADD_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "+=fV");
			break;
		case OP_ADD_ASSIGN_AT_R_V:
			sprintf(buffer, "+=@fV");
			break;
		case OP_ADD_R:
			sprintf(buffer, "+f");
			break;
		case OP_SUB_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "-=f");
			break;
		case OP_SUB_ASSIGN_AT_R:
			sprintf(buffer, "-=@f");
			break;
		case OP_SUB_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "-=fV");
			break;
		case OP_SUB_ASSIGN_AT_R_V:
			sprintf(buffer, "-=@fV");
			break;
		case OP_SUB_R:
			sprintf(buffer, "-f");
			break;
		case OP_MUL_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "*=f");
			break;
		case OP_MUL_ASSIGN_AT_R:
			sprintf(buffer, "*=@f");
			break;
		case OP_MUL_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "*=fV");
			break;
		case OP_MUL_ASSIGN_AT_R_V:
			sprintf(buffer, "*=@fV");
			break;
		case OP_MUL_R:
			sprintf(buffer, "*f");
			break;
		case OP_DIV_ASSIGN_R:
			argc = 1;
			sprintf(buffer, "/=f");
			break;
		case OP_DIV_ASSIGN_AT_R:
			sprintf(buffer, "/=@f");
			break;
		case OP_DIV_ASSIGN_R_V:
			argc = 1;
			sprintf(buffer, "/=fV");
			break;
		case OP_DIV_ASSIGN_AT_R_V:
			sprintf(buffer, "/=@fV");
			break;
		case OP_DIV_R:
			sprintf(buffer, "/f");
			break;

		case OP_REM_ASSIGN:
			argc = 1;
			sprintf(buffer, "%%=");
			break;
		case OP_REM_ASSIGN_AT:
			sprintf(buffer, "%%=@");
			break;
		case OP_REM_ASSIGN_V:
			argc = 1;
			sprintf(buffer, "%%=V");
			break;
		case OP_REM_ASSIGN_AT_V:
			sprintf(buffer, "%%=@V");
			break;
		case OP_REM:
			sprintf(buffer, "%%");
			break;

		case IC_CALL1:
			sprintf(buffer, "CALL1");
			break;
		case IC_CALL2:
			argc = 1;
			sprintf(buffer, "CALL2");
			break;
		case IC_STOP:
			sprintf(buffer, "STOP");
			break;
		case IC_RETURN_VAL:
			argc = 1;
			sprintf(buffer, "RETURNVAL");
			break;
		case IC_RETURN_VOID:
			sprintf(buffer, "RETURNVOID");
			break;
		case IC_B:
			argc = 1;
			sprintf(buffer, "B");
			break;
		case IC_BE0:
			argc = 1;
			sprintf(buffer, "BE0");
			break;
		case IC_BNE0:
			argc = 1;
			sprintf(buffer, "BNE0");
			break;
		case IC_SLICE:
			argc = 1;
			was_switch = true;
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
		case IC_SELECT:
			argc = 1;
			was_switch = true;
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
		case OP_WIDEN:
			sprintf(buffer, "WIDEN");
			break;
		case OP_WIDEN1:
			sprintf(buffer, "WIDEN1");
			break;
		case IC_DUPLICATE:
			sprintf(buffer, "DOUBLE");
			break;
		case OP_PRE_INC:
			argc = 1;
			sprintf(buffer, "INC");
			break;
		case OP_PRE_DEC:
			argc = 1;
			sprintf(buffer, "DEC");
			break;
		case OP_POST_INC:
			argc = 1;
			sprintf(buffer, "POSTINC");
			break;
		case OP_POST_DEC:
			argc = 1;
			sprintf(buffer, "POSTDEC");
			break;
		case OP_PRE_INC_AT:
			sprintf(buffer, "INC@");
			break;
		case OP_PRE_DEC_AT:
			sprintf(buffer, "DEC@");
			break;
		case OP_POST_INC_AT:
			sprintf(buffer, "POSTINC@");
			break;
		case OP_POST_DEC_AT:
			sprintf(buffer, "POSTDEC@");
			break;
		case OP_PRE_INC_R:
			argc = 1;
			sprintf(buffer, "INCf");
			break;
		case OP_PRE_DEC_R:
			argc = 1;
			sprintf(buffer, "DECf");
			break;
		case OP_POST_INC_R:
			argc = 1;
			sprintf(buffer, "POSTINCf");
			break;
		case OP_POST_DEC_R:
			argc = 1;
			sprintf(buffer, "POSTDECf");
			break;
		case OP_PRE_INC_AT_R:
			sprintf(buffer, "INC@f");
			break;
		case OP_PRE_DEC_AT_R:
			sprintf(buffer, "DEC@f");
			break;
		case OP_POST_INC_AT_R:
			sprintf(buffer, "POSTINC@f");
			break;
		case OP_POST_DEC_AT_R:
			sprintf(buffer, "POSTDEC@f");
			break;
		case OP_PRE_INC_V:
			argc = 1;
			sprintf(buffer, "INCV");
			break;
		case OP_PRE_DEC_V:
			argc = 1;
			sprintf(buffer, "DECV");
			break;
		case OP_POST_INC_V:
			argc = 1;
			sprintf(buffer, "POSTINCV");
			break;
		case OP_POST_DEC_V:
			argc = 1;
			sprintf(buffer, "POSTDECV");
			break;
		case OP_PRE_INC_AT_V:
			sprintf(buffer, "INC@V");
			break;
		case OP_PRE_DEC_AT_V:
			sprintf(buffer, "DEC@V");
			break;
		case OP_POST_INC_AT_V:
			sprintf(buffer, "POSTINC@V");
			break;
		case OP_POST_DEC_AT_V:
			sprintf(buffer, "POSTDEC@V");
			break;
		case OP_PRE_INC_R_V:
			argc = 1;
			sprintf(buffer, "INCfV");
			break;
		case OP_PRE_DEC_R_V:
			argc = 1;
			sprintf(buffer, "DECfV");
			break;
		case OP_POST_INC_R_V:
			argc = 1;
			sprintf(buffer, "POSTINCfV");
			break;
		case OP_POST_DEC_R_V:
			argc = 1;
			sprintf(buffer, "POSTDECfV");
			break;
		case OP_PRE_INC_AT_R_V:
			sprintf(buffer, "INC@fV");
			break;
		case OP_PRE_DEC_AT_R_V:
			sprintf(buffer, "DEC@fV");
			break;
		case OP_POST_INC_AT_R_V:
			sprintf(buffer, "POSTINC@fV");
			break;
		case OP_POST_DEC_AT_R_V:
			sprintf(buffer, "POSTDEC@fV");
			break;

		case OP_NOT:
			sprintf(buffer, "BITNOT");
			break;
		case OP_LOG_NOT:
			sprintf(buffer, "NOT");
			break;
		case OP_UNMINUS:
			sprintf(buffer, "UNMINUS");
			break;
		case OP_UNMINUS_R:
			sprintf(buffer, "UNMINUSf");
			break;

		case IC_FUNC_BEG:
			argc = 2;
			was_switch = true;
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

	if (type == OP_CONST || type == IC_LID)
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

	if (type == OP_STRING)
	{
		const size_t n = (size_t)vector_get(table, i - 1);
		for (size_t j = 0; j < n; j++)
		{
			uni_printf(io, "%" PRIitem "\n", vector_get(table, i++));
		}
	}
	else if (type == OP_STRING_D)
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

	if (type == OP_CONST_D || type == IC_LID)
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

		if ((node_get_arg(nd, i) != ITEM_MAX && node_get_type(nd) != OP_STRING && node_get_type(nd) != OP_STRING_D)
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
