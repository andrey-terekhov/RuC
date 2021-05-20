/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#pragma once

#include "defs.h"
#include "instructions.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum NODE
{
	// Statements
	ND_LABEL			= TLabel,			/**< Label statement node */
	ND_CASE				= TCase,			/**< Case statement node */
	ND_DEFAULT			= TDefault,			/**< Default statement node */
	ND_BLOCK			= TBegin,			/**< Compound statement node */
	ND_NULL				= NOP,				/**< Null statement node */
	ND_IF				= TIf,				/**< If statement node */
	ND_SWITCH			= TSwitch,			/**< Switch statement node */
	ND_WHILE			= TWhile,			/**< While statement node */
	ND_DO				= TDo,				/**< Do statement node */
	ND_FOR				= TFor,				/**< For statement node */
	ND_GOTO				= TGoto,			/**< Goto statement node */
	ND_CONTINUE			= TContinue,		/**< Continue statement node */
	ND_BREAK			= TBreak,			/**< Break statement node */
	ND_RETURN_VOID		= TReturnvoid,		/**< Void return statement node */
	ND_RETURN_VAL		= TReturnval,		/**< Valued return statement node */
	ND_CREATEDIRECT		= CREATEDIRECTC,	/**< Create direct statement node */
	ND_EXITDIRECT		= EXITDIRECTC,		/**< Exit direct statement node */
	ND_PRINTID			= TPrintid,			/**< Printid statement node */
	ND_PRINT			= TPrint,			/**< Print statement node */
	ND_GETID			= TGetid,			/**< Getid statement node */
	ND_PRINTF			= TPrintf,			/**< Printf statement node */

	// Declarations
	ND_DECL_ID			= TDeclid,			/**< Identifier declaration node */
	ND_DECL_ARR			= TDeclarr,			/**< Array declaration node */
	ND_DECL_STRUCT		= TStructbeg,		/**< Struct declaration node */
	ND_FUNC_DEF			= TFuncdef,			/**< Function definition node */
	ND_ARRAY_INIT		= TBeginit,			/**< Array inition node */
	ND_STRUCT_INIT		= TStructinit,		/**< Struct inition node */

	// End nodes
	ND_BLOCK_END		= TEnd,
	ND_DECL_STRUCT_END	= TStructend,
	ND_EXPRESSION_END	= TExprend,

	// Expressions
	ND_CONDITIONAL		= TCondexpr,
	ND_WIDEN			= WIDEN,			/**< "WIDEN" node */
	ND_WIDEN1			= WIDEN1,			/**< "WIDEN1" node */
	ND_STRINGINIT		= STRINGINIT,		/**< "STRINGINIT" node */
	ND_STRING			= TString,
	ND_STRINGD			= TStringd,
	ND_ADLOGOR			= ADLOGOR,
	ND_ADLOGAND			= ADLOGAND,
	ND_IDENT			= TIdent,
	ND_IDENTTOADDR		= TIdenttoaddr,
	ND_IDENTTOVAL		= TIdenttoval,
	ND_IDENTTOVALD		= TIdenttovald,
	ND_ADDRTOVAL		= TAddrtoval,
	ND_ADDRTOVALD		= TAddrtovald,
	ND_CONST			= TConst,
	ND_CONSTD			= TConstd,
	ND_SLICE			= TSlice,
	ND_SLICEIDENT		= TSliceident,
	ND_SELECT			= TSelect,
	ND_CALL1			= TCall1,
	ND_CALL2			= TCall2,
	ND_ROWING			= ROWING,			/**< "ROWING" node */
	ND_ROWINGD			= ROWINGD,			/**< "ROWINGD" node */

	// Standard functions
	ND_COPY00			= COPY00,			/**< "COPY00" node */
	ND_COPY01			= COPY01,			/**< "COPY01" node */
	ND_COPY10			= COPY10,			/**< "COPY10" node */
	ND_COPY11			= COPY11,			/**< "COPY11" node */
	ND_COPY0ST			= COPY0ST,			/**< "COPY0ST" node */
	ND_COPY1ST			= COPY1ST,			/**< "COPY1ST" node */
	ND_COPY0STASS		= COPY0STASS,		/**< "COPY0STASS" node */
	ND_COPY1STASS		= COPY1STASS,		/**< "COPY1STASS" node */
	ND_COPYST			= COPYST,			/**< "COPYST" node */

	ND_ABSI				= ABSIC,			/**< "ABSI" node */
	ND_ABS				= ABSC,				/**< "ABS" node */
	ND_SQRT				= SQRTC,			/**< "SQRT" node */
	ND_EXP				= EXPC,				/**< "EXP" node */
	ND_SIN				= SINC,				/**< "SIN" node */
	ND_COS				= COSC,				/**< "COS" node */
	ND_LOG				= LOGC,				/**< "LOG" node */
	ND_LOG10			= LOG10C,			/**< "LOG10" node */
	ND_ASIN				= ASINC,			/**< "ASIN" node */
	ND_RAND				= RANDC,			/**< "RAND" node */
	ND_ROUND			= ROUNDC,			/**< "ROUND" node */

	ND_STRCPY			= STRCPYC,			/**< "STRCPY" node */
	ND_STRNCPY			= STRNCPYC,			/**< "STRNCPY" node */
	ND_STRCAT			= STRCATC,			/**< "STRCAT" node */
	ND_STRNCAT			= STRNCATC,			/**< "STRNCAT" node */
	ND_STRCMP			= STRCMPC,			/**< "STRCMP" node */
	ND_STRNCMP			= STRNCMPC,			/**< "STRNCMP" node */
	ND_STRSTR			= STRSTRC,			/**< "STRSTR" node */
	ND_STRLEN			= STRLENC,			/**< "STRLEN" node */

	ND_MSG_SEND			= MSGSENDC,			/**< "MSG_SEND" node */
	ND_MSG_RECEIVE		= MSGRECEIVEC,		/**< "MSG_RECEIVE" node */
	ND_JOIN				= JOINC,			/**< "JOIN" node */
	ND_SLEEP			= SLEEPC,			/**< "SLEEP" node */
	ND_SEMCREATE		= SEMCREATEC,		/**< "SEMCREATE" node */
	ND_SEMWAIT			= SEMWAITC,			/**< "SEMWAIT" node */
	ND_SEMPOST			= SEMPOSTC,			/**< "SEMPOST" node */
	ND_CREATE			= CREATEC,			/**< "CREATE" node */
	ND_INIT				= INITC,			/**< "INIT" node */
	ND_DESTROY			= DESTROYC,			/**< "DESTROY" node */
	ND_EXIT				= EXITC,			/**< "EXIT" node */
	ND_GETNUM			= GETNUMC,			/**< "GETNUM" node */

	ND_UPB				= UPBC,				/**< "UPB" node */
	ND_SEND_INT			= SEND_INTC,		/**< "SEND_INT" node */
	ND_SEND_FLOAT		= SEND_FLOATC,		/**< "SEND_FLOAT" node */
	ND_SEND_STRING		= SEND_STRINGC,		/**< "SEND_STRING" node */
	ND_RECEIVE_INT		= RECEIVE_INTC,		/**< "RECEIVE_INT" node */
	ND_RECEIVE_FLOAT	= RECEIVE_FLOATC,	/**< "RECEIVE_FLOAT" node */
	ND_RECEIVE_STRING	= RECEIVE_STRINGC,	/**< "RECEIVE_STRING" node */
	ND_ASSERT			= ASSERTC,			/**< "ASSERT" node */

	// Operators
	ND_REMASS			= REMASS,			/**< "%=" node */
	ND_SHLASS			= SHLASS,			/**< "<<=" node */
	ND_SHRASS			= SHRASS,			/**< ">>=" node */
	ND_ANDASS			= ANDASS,			/**< "&=" node */
	ND_EXORASS			= EXORASS,			/**< "^=" node */
	ND_ORASS			= ORASS,			/**< "|=" node */
	ND_ASS				= ASS,				/**< "=" node */
	ND_PLUSASS			= PLUSASS,			/**< "+=" node */
	ND_MINUSASS			= MINUSASS,			/**< "-=" node */
	ND_MULTASS			= MULTASS,			/**< "*=" node */
	ND_DIVASS			= DIVASS,			/**< "/=" node */
	ND_REMASSAT			= REMASSAT,			/**< "%=@" node */
	ND_SHLASSAT			= SHLASSAT,			/**< "<<=@" node */
	ND_SHRASSAT			= SHRASSAT,			/**< ">>=@" node */
	ND_ANDASSAT			= ANDASSAT,			/**< "&=@" node */
	ND_EXORASSAT		= EXORASSAT,		/**< "^=@" node */
	ND_ORASSAT			= ORASSAT,			/**< "|=@" node */
	ND_ASSAT			= ASSAT,			/**< "=@" node */
	ND_PLUSASSAT		= PLUSASSAT,		/**< "+=@" node */
	ND_MINUSASSAT		= MINUSASSAT,		/**< "-=@" node */
	ND_MULTASSAT		= MULTASSAT,		/**< "*=@" node */
	ND_DIVASSAT			= DIVASSAT,			/**< "/=@" node */
	ND_REM				= LREM,				/**< "%" node */
	ND_SHL				= LSHL,				/**< "<<" node */
	ND_SHR				= LSHR,				/**< ">>" node */
	ND_AND				= LAND,				/**< "&" node */
	ND_EXOR				= LEXOR,			/**< "^" node */
	ND_OR				= LOR,				/**< "|" node */
	ND_LOGAND			= LOGAND,			/**< "&&" node */
	ND_LOGOR			= LOGOR,			/**< "||" node */
	ND_EQEQ				= EQEQ,				/**< "==" node */
	ND_NOTEQ			= NOTEQ,			/**< "!=" node */
	ND_LT				= LLT,				/**< "<" node */
	ND_GT				= LGT,				/**< ">" node */
	ND_LE				= LLE,				/**< "<=" node */
	ND_GE				= LGE,				/**< ">=" node */
	ND_PLUS				= LPLUS,			/**< "+" node */
	ND_MINUS			= LMINUS,			/**< "-" node */
	ND_MULT				= LMULT,			/**< "*" node */
	ND_DIV				= LDIV,				/**< "/" node */
	ND_POSTINC			= POSTINC,			/**< "POSTINC" node */
	ND_POSTDEC			= POSTDEC,			/**< "POSTDEC" node */
	ND_INC				= INC,				/**< "INC" node */
	ND_DEC				= DEC,				/**< "DEC" node */
	ND_POSTINCAT		= POSTINCAT,		/**< "POSTINC@" node */
	ND_POSTDECAT		= POSTDECAT,		/**< "POSTDEC@" node */
	ND_INCAT			= INCAT,			/**< "INC@" node */
	ND_DECAT			= DECAT,			/**< "DEC@" node */
	ND_UNMINUS			= UNMINUS,			/**< "UNMINUS" node */
	ND_NOT				= LNOT,				/**< "BITNOT" node */
	ND_LOGNOT			= LOGNOT,			/**< "NOT" node */

	ND_ASSR				= ASSR,				/**< "=f" node */
	ND_PLUSASSR			= PLUSASSR,			/**< "+=f" node */
	ND_MINUSASSR		= MINUSASSR,		/**< "-=f" node */
	ND_MULTASSR			= MULTASSR,			/**< "*=f" node */
	ND_DIVASSR			= DIVASSR,			/**< "/=f" node */
	ND_ASSATR			= ASSATR ,			/**< "=@f" node */
	ND_PLUSASSATR		= PLUSASSATR,		/**< "+=@f" node */
	ND_MINUSASSATR		= MINUSASSATR,		/**< "-=@f" node */
	ND_MULTASSATR		= MULTASSATR,		/**< "*=@f" node */
	ND_DIVASSATR		= DIVASSATR,		/**< "/=@f" node */

	ND_EQEQR			= EQEQR,			/**< "==f" node */
	ND_NOTEQR			= NOTEQR,			/**< "!=f" node */
	ND_LTR				= LLTR,				/**< "<f" node */
	ND_GTR				= LGTR,				/**< ">f" node */
	ND_LER				= LLER,				/**< "<=f" node */
	ND_GER				= LGER,				/**< ">=f" node */
	ND_PLUSR			= LPLUSR,			/**< "+f" node */
	ND_MINUSR			= LMINUSR,			/**< "-f" node */
	ND_MULTR			= LMULTR,			/**< "*f" node */
	ND_DIVR				= LDIVR,			/**< "/f" node */
	ND_POSTINCR			= POSTINCR,			/**< "POSTINCf" node */
	ND_POSTDECR			= POSTDECR,			/**< "POSTDECf" node */
	ND_INCR				= INCR,				/**< "INCf" node */
	ND_DECR				= DECR,				/**< "DECf" node */
	ND_POSTINCATR		= POSTINCATR,		/**< "POSTINC@f" node */
	ND_POSTDECATR		= POSTDECATR,		/**< "POSTDEC@f" node */
	ND_INCATR			= INCATR,			/**< "INC@f" node */
	ND_DECATR			= DECATR,			/**< "DEC@f" node */
	ND_UNMINUSR			= UNMINUSR,			/**< "UNIMINUSf" node */

	ND_REMASSV			= REMASSV,			/**< "%=V" node */
	ND_SHLASSV			= SHLASSV,			/**< "<<=V" node */
	ND_SHRASSV			= SHRASSV,			/**< ">>=V" node */
	ND_ANDASSV			= ANDASSV,			/**< "&=V" node */
	ND_EXORASSV			= EXORASSV,			/**< "^=V" node */
	ND_ORASSV			= ORASSV,			/**< "|=V" node */
	ND_ASSV				= ASSV,				/**< "=V" node */
	ND_PLUSASSV			= PLUSASSV,			/**< "+=V" node */
	ND_MINUSASSV		= MINUSASSV,		/**< "-=V" node */
	ND_MULTASSV			= MULTASSV,			/**< "*=V" node */
	ND_DIVASSV			= DIVASSV,			/**< "/=V" node */
	ND_REMASSATV		= REMASSATV,		/**< "%=@V" node */
	ND_SHLASSATV		= SHLASSATV,		/**< "<<=@V" node */
	ND_SHRASSATV		= SHRASSATV,		/**< ">>=@V" node */
	ND_ANDASSATV		= ANDASSATV,		/**< "&=@V" node */
	ND_EXORASSATV		= EXORASSATV,		/**< "^=@V" node */
	ND_ORASSATV			= ORASSATV,			/**< "|=@V" node */
	ND_ASSATV			= ASSATV,			/**< "=@V" node */
	ND_PLUSASSATV		= PLUSASSATV,		/**< "+=@V" node */
	ND_MINUSASSATV		= MINUSASSATV,		/**< "-=@V" node */
	ND_MULTASSATV		= MULTASSATV,		/**< "*=@V" node */
	ND_DIVASSATV		= DIVASSATV,		/**< "/=@V" node */

	ND_ASSRV			= ASSRV,			/**< "=fV" node */
	ND_PLUSASSRV		= PLUSASSRV,		/**< "+=fV" node */
	ND_MINUSASSRV		= MINUSASSRV,		/**< "-=fV" node */
	ND_MULTASSRV		= MULTASSRV,		/**< *=fV" node */
	ND_DIVASSRV			= DIVASSRV,			/**< "/=fV" node */
	ND_ASSATRV			= ASSATRV,			/**< "=@fV" node */
	ND_PLUSASSATRV		= PLUSASSATRV,		/**< "+=@fV" node */
	ND_MINUSASSATRV		= MINUSASSATRV,		/**< "-=@fV" node */
	ND_MULTASSATRV		= MULTASSATRV,		/**< *=@fV" node */
	ND_DIVASSATRV		= DIVASSATRV,		/**< "/=@fV" node */

	ND_POSTINCV			= POSTINCV,			/**< "POSTINCV" node */
	ND_POSTDECV			= POSTDECV,			/**< "POSTDECV" node */
	ND_INCV				= INCV,				/**< "INCV" node */
	ND_DECV				= DECV,				/**< "DECV" node */
	ND_POSTINCATV		= POSTINCATV,		/**< "POSTINC@V" node */
	ND_POSTDECATV		= POSTDECATV,		/**< "POSTDEC@V" node */
	ND_INCATV			= INCATV,			/**< "INC@V" node */
	ND_DECATV			= DECATV,			/**< "DEC@V" node */

	ND_POSTINCRV		= POSTINCRV,		/**< "POSTINCfV" node */
	ND_POSTDECRV		= POSTDECRV,		/**< "POSTDECfV" node */
	ND_INCRV			= INCRV,			/**< "INCfV" node */
	ND_DECRV			= DECRV,			/**< "DECfV" node */
	ND_POSTINCATRV		= POSTINCATRV,		/**< "POSTINC@fV" node */
	ND_POSTDECATRV		= POSTDECATRV,		/**< "POSTDEC@fV" node */
	ND_INCATRV			= INCATRV,			/**< "INC@fV" node */
	ND_DECATRV			= DECATRV,			/**< "DEC@fV" node */
} node_t;


instruction_t node_to_instruction(const node_t node);

int node_is_assignment_operator(const node_t node);

#ifdef __cplusplus
} /* extern "C" */
#endif
