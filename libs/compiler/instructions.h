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


#ifdef __cplusplus
extern "C" {
#endif

typedef enum INSTURCTION
{
	IC_NOP = 9453,		/**< "NOP" instruction code */
	IC_DEFARR,			/**< "DEFARR" instruction code */
	IC_LI,				/**< "LI" instruction code */
	IC_LID,				/**< "LID" instruction code */
	IC_LOAD,			/**< "LOAD" instruction code */
	IC_LOADD,			/**< "LOADD" instruction code */
	IC_LAT,				/**< "L@" instruction code */
	IC_LATD,			/**< "L@f" instruction code */
	IC_STOP,			/**< "STOP" instruction code */
	IC_SELECT,			/**< "SELECT" instruction code */
	IC_FUNCBEG,			/**< "FUNCBEG" instruction code */
	IC_LA,				/**< "LA" instruction code */
	IC_CALL1,			/**< "CALL1" instruction code */
	IC_CALL2,			/**< "CALL2" instruction code */
	IC_RETURNVAL,		/**< "RETURNVAL" instruction code */
	IC_RETURNVOID,		/**< "RETURNVOID" instruction code */
	IC_B,				/**< "B" instruction code */
	IC_BE0,				/**< "BE0" instruction code */
	IC_BNE0,			/**< "BNE0" instruction code */
	IC_SLICE,			/**< "SLICE" instruction code */
	IC_WIDEN,			/**< "WIDEN" instruction code */
	IC_WIDEN1,			/**< "WIDEN1" instruction code */
	IC__DOUBLE,			/**< "_DOUBLE" instruction code */
	IC_STRINGINIT,		/**< "STRINGINIT" instruction code */
	IC_ARRINIT,			/**< "ARRINIT" instruction code */
	IC_STRUCTWITHARR,	/**< "STRUCTWITHARR" instruction code */

	IC_BEGINIT = 9481,	/**< "BEGINIT" instruction code */
	IC_ROWING,			/**< "ROWING" instruction code */
	IC_ROWINGD,			/**< "ROWINGD" instruction code */

	IC_COPY00 = 9300,	/**< "COPY00" instruction code */
	IC_COPY01,			/**< "COPY01" instruction code */
	IC_COPY10,			/**< "COPY10" instruction code */
	IC_COPY11,			/**< "COPY11" instruction code */
	IC_COPY0ST,			/**< "COPY0ST" instruction code */
	IC_COPY1ST,			/**< "COPY1ST" instruction code */
	IC_COPY0STASS,		/**< "COPY0STASS" instruction code */
	IC_COPY1STASS,		/**< "COPY1STASS" instruction code */
	IC_COPYST,			/**< "COPYST" instruction code */

	IC_CREATEDIRECT = 9528,	/**< "CREATEDIRECT" instruction code */
	IC_EXITDIRECT,			/**< "EXITDIRECT" instruction code */

	IC_ABSI	= 9651,		/**< "ABSI" instruction code */
	IC_ABS	= 9534,		/**< "ABS" instruction code */
	IC_SQRT,			/**< "SQRT" instruction code */
	IC_EXP,				/**< "EXP" instruction code */
	IC_SIN,				/**< "SIN" instruction code */
	IC_COS,				/**< "COS" instruction code */
	IC_LOG,				/**< "LOG" instruction code */
	IC_LOG10,			/**< "LOG10" instruction code */
	IC_ASIN,			/**< "ASIN" instruction code */
	IC_RAND,			/**< "RAND" instruction code */
	IC_ROUND,			/**< "ROUND" instruction code */

	IC_STRCPY = 9544,	/**< "STRCPY" instruction code */
	IC_STRNCPY,			/**< "STRNCPY" instruction code */
	IC_STRCAT,			/**< "STRCAT" instruction code */
	IC_STRNCAT,			/**< "STRNCAT" instruction code */
	IC_STRCMP,			/**< "STRCMP" instruction code */
	IC_STRNCMP,			/**< "STRNCMP" instruction code */
	IC_STRSTR,			/**< "STRSTR" instruction code */
	IC_STRLEN,			/**< "STRLEN" instruction code */

	IC_MSG_SEND = 9552,	/**< "MSG_SEND" instruction code */
	IC_MSG_RECEIVE,		/**< "MSG_RECEIVE" instruction code */
	IC_JOIN,			/**< "JOIN" instruction code */
	IC_SLEEP,			/**< "SLEEP" instruction code */
	IC_SEMCREATE,		/**< "SEMCREATE" instruction code */
	IC_SEMWAIT,			/**< "SEMWAIT" instruction code */
	IC_SEMPOST,			/**< "SEMPOST" instruction code */
	IC_CREATE,			/**< "CREATE" instruction code */
	IC_INIT,			/**< "INIT" instruction code */
	IC_DESTROY,			/**< "DESTROY" instruction code */
	IC_EXIT,			/**< "EXIT" instruction code */
	IC_GETNUM,			/**< "GETNUM" instruction code */

	IC_UPB = 9588,		/**< "UPB" instruction code */
	IC_SEND_INT,		/**< "SEND_INT" instruction code */
	IC_SEND_FLOAT,		/**< "SEND_FLOAT" instruction code */
	IC_SEND_STRING,		/**< "SEND_STRING" instruction code */
	IC_RECEIVE_INT,		/**< "RECEIVE_INT" instruction code */
	IC_RECEIVE_FLOAT,	/**< "RECEIVE_FLOAT" instruction code */
	IC_RECEIVE_STRING,	/**< "RECEIVE_STRING" instruction code */
	IC_ASSERT,			/**< "ASSERT" instruction code */

	IC_GETID = -27,		/**< "GETID" instruction code */
	IC_SCANF,			/**< "SCANF" instruction code */
	IC_PRINTF,			/**< "PRINTF" instruction code */
	IC_PRINT,			/**< "PRINT" instruction code */
	IC_PRINTID,			/**< "PRINTID" instruction code */

	IC_REMASS = 9001,	/**< "%=" instruction code */
	IC_SHLASS,			/**< "<<=" instruction code */
	IC_SHRASS,			/**< ">>=" instruction code */
	IC_ANDASS,			/**< "&=" instruction code */
	IC_EXORASS,			/**< "^=" instruction code */
	IC_ORASS,			/**< "|=" instruction code */
	IC_ASS,				/**< "=" instruction code */
	IC_PLUSASS,			/**< "+=" instruction code */
	IC_MINUSASS,		/**< "-=" instruction code */
	IC_MULTASS,			/**< "*=" instruction code */
	IC_DIVASS,			/**< "/=" instruction code */
	IC_REMASSAT,		/**< "%=@" instruction code */
	IC_SHLASSAT,		/**< "<<=@" instruction code */
	IC_SHRASSAT,		/**< ">>=@" instruction code */
	IC_ANDASSAT,		/**< "&=@" instruction code */
	IC_EXORASSAT,		/**< "^=@" instruction code */
	IC_ORASSAT,			/**< "|=@" instruction code */
	IC_ASSAT,			/**< "=@" instruction code */
	IC_PLUSASSAT,		/**< "+=@" instruction code */
	IC_MINUSASSAT,		/**< "-=@" instruction code */
	IC_MULTASSAT,		/**< "*=@" instruction code */
	IC_DIVASSAT,		/**< "/=@" instruction code */
	IC_REM,				/**< "%" instruction code */
	IC_SHL,				/**< "<<" instruction code */
	IC_SHR,				/**< ">>" instruction code */
	IC_AND,				/**< "&" instruction code */
	IC_EXOR,			/**< "^" instruction code */
	IC_OR,				/**< "|" instruction code */
	IC_LOGAND,			/**< "&&" instruction code */
	IC_LOGOR,			/**< "||" instruction code */
	IC_EQEQ,			/**< "==" instruction code */
	IC_NOTEQ,			/**< "!=" instruction code */
	IC_LT,				/**< "<" instruction code */
	IC_GT,				/**< ">" instruction code */
	IC_LE,				/**< "<=" instruction code */
	IC_GE,				/**< ">=" instruction code */
	IC_PLUS,			/**< "+" instruction code */
	IC_MINUS,			/**< "-" instruction code */
	IC_MULT,			/**< "*" instruction code */
	IC_DIV,				/**< "/" instruction code */
	IC_POSTINC,			/**< "POSTINC" instruction code */
	IC_POSTDEC,			/**< "POSTDEC" instruction code */
	IC_INC,				/**< "INC" instruction code */
	IC_DEC,				/**< "DEC" instruction code */
	IC_POSTINCAT,		/**< "POSTINC@" instruction code */
	IC_POSTDECAT,		/**< "POSTDEC@" instruction code */
	IC_INCAT,			/**< "INC@" instruction code */
	IC_DECAT,			/**< "DEC@" instruction code */
	IC_UNMINUS,			/**< "UNMINUS" instruction code */
	IC_NOT = 9052,		/**< "BITNOT" instruction code */
	IC_LOGNOT,			/**< "NOT" instruction code */

	IC_ASSR = 9057,		/**< "=f" instruction code */
	IC_PLUSASSR,		/**< "+=f" instruction code */
	IC_MINUSASSR,		/**< "-=f" instruction code */
	IC_MULTASSR,		/**< "*=f" instruction code */
	IC_DIVASSR,			/**< "/=f" instruction code */
	IC_ASSATR = 9068,	/**< "=@f" instruction code */
	IC_PLUSASSATR,		/**< "+=@f" instruction code */
	IC_MINUSASSATR,		/**< "-=@f" instruction code */
	IC_MULTASSATR,		/**< "*=@f" instruction code */
	IC_DIVASSATR,		/**< "/=@f" instruction code */

	IC_EQEQR = 9081,	/**< "==f" instruction code */
	IC_NOTEQR,			/**< "!=f" instruction code */
	IC_LTR,				/**< "<f" instruction code */
	IC_GTR,				/**< ">f" instruction code */
	IC_LER,				/**< "<=f" instruction code */
	IC_GER,				/**< ">=f" instruction code */
	IC_PLUSR,			/**< "+f" instruction code */
	IC_MINUSR,			/**< "-f" instruction code */
	IC_MULTR,			/**< "*f" instruction code */
	IC_DIVR,			/**< "/f" instruction code */
	IC_POSTINCR,		/**< "POSTINCf" instruction code */
	IC_POSTDECR,		/**< "POSTDECf" instruction code */
	IC_INCR,			/**< "INCf" instruction code */
	IC_DECR,			/**< "DECf" instruction code */
	IC_POSTINCATR,		/**< "POSTINC@f" instruction code */
	IC_POSTDECATR,		/**< "POSTDEC@f" instruction code */
	IC_INCATR,			/**< "INC@f" instruction code */
	IC_DECATR,			/**< "DEC@f" instruction code */
	IC_UNMINUSR,		/**< "UNIMINUSf" instruction code */

	IC_REMASSV = 9201,	/**< "%=V" instruction code */
	IC_SHLASSV,			/**< "<<=V" instruction code */
	IC_SHRASSV,			/**< ">>=V" instruction code */
	IC_ANDASSV,			/**< "&=V" instruction code */
	IC_EXORASSV,		/**< "^=V" instruction code */
	IC_ORASSV,			/**< "|=V" instruction code */
	IC_ASSV,			/**< "=V" instruction code */
	IC_PLUSASSV,		/**< "+=V" instruction code */
	IC_MINUSASSV,		/**< "-=V" instruction code */
	IC_MULTASSV,		/**< "*=V" instruction code */
	IC_DIVASSV,			/**< "/=V" instruction code */
	IC_REMASSATV,		/**< "%=@V" instruction code */
	IC_SHLASSATV,		/**< "<<=@V" instruction code */
	IC_SHRASSATV,		/**< ">>=@V" instruction code */
	IC_ANDASSATV,		/**< "&=@V" instruction code */
	IC_EXORASSATV,		/**< "^=@V" instruction code */
	IC_ORASSATV,		/**< "|=@V" instruction code */
	IC_ASSATV,			/**< "=@V" instruction code */
	IC_PLUSASSATV,		/**< "+=@V" instruction code */
	IC_MINUSASSATV,		/**< "-=@V" instruction code */
	IC_MULTASSATV,		/**< "*=@V" instruction code */
	IC_DIVASSATV,		/**< "/=@V" instruction code */

	IC_ASSRV = 9257,	/**< "=fV" instruction code */
	IC_PLUSASSRV,		/**< "+=fV" instruction code */
	IC_MINUSASSRV,		/**< "-=fV" instruction code */
	IC_MULTASSRV,		/**< *=fV" instruction code */
	IC_DIVASSRV,		/**< "/=fV" instruction code */
	IC_ASSATRV = 9268,	/**< "=@fV" instruction code */
	IC_PLUSASSATRV,		/**< "+=@fV" instruction code */
	IC_MINUSASSATRV,	/**< "-=@fV" instruction code */
	IC_MULTASSATRV,		/**< *=@fV" instruction code */
	IC_DIVASSATRV,		/**< "/=@fV" instruction code */

	IC_POSTINCV = 9241,	/**< "POSTINCV" instruction code */
	IC_POSTDECV,		/**< "POSTDECV" instruction code */
	IC_INCV,			/**< "INCV" instruction code */
	IC_DECV,			/**< "DECV" instruction code */
	IC_POSTINCATV,		/**< "POSTINC@V" instruction code */
	IC_POSTDECATV,		/**< "POSTDEC@V" instruction code */
	IC_INCATV,			/**< "INC@V" instruction code */
	IC_DECATV,			/**< "DEC@V" instruction code */

	IC_POSTINCRV = 9291,/**< "POSTINCfV" instruction code */
	IC_POSTDECRV,		/**< "POSTDECfV" instruction code */
	IC_INCRV,			/**< "INCfV" instruction code */
	IC_DECRV,			/**< "DECfV" instruction code */
	IC_POSTINCATRV,		/**< "POSTINC@fV" instruction code */
	IC_POSTDECATRV,		/**< "POSTDEC@fV" instruction code */
	IC_INCATRV,			/**< "INC@fV" instruction code */
	IC_DECATRV,			/**< "DEC@fV" instruction code */
} instruction_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
