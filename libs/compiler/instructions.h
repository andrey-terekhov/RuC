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

#include "operations.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum INSTURCTION
{
	IC_GETID = -27,				/**< 'GETID' instruction code */
	IC_SCANF,					/**< 'SCANF' instruction code */
	IC_PRINTF,					/**< 'PRINTF' instruction code */
	IC_PRINT,					/**< 'PRINT' instruction code */
	IC_PRINTID,					/**< 'PRINTID' instruction code */

	MIN_INSTRUCTION_CODE = 9000,

	IC_REM_ASSIGN,				/**< '%=' instruction code */
	IC_SHL_ASSIGN,				/**< '<<=' instruction code */
	IC_SHR_ASSIGN,				/**< '>>=' instruction code */
	IC_AND_ASSIGN,				/**< '&=' instruction code */
	IC_XOR_ASSIGN,				/**< '^=' instruction code */
	IC_OR_ASSIGN,				/**< '|=' instruction code */
	IC_ASSIGN,					/**< '=' instruction code */
	IC_ADD_ASSIGN,				/**< '+=' instruction code */
	IC_SUB_ASSIGN,				/**< '-=' instruction code */
	IC_MUL_ASSIGN,				/**< '*=' instruction code */
	IC_DIV_ASSIGN,				/**< '/=' instruction code */
	IC_REM_ASSIGN_AT,			/**< '%=@' instruction code */
	IC_SHL_ASSIGN_AT,			/**< '<<=@' instruction code */
	IC_SHR_ASSIGN_AT,			/**< '>>=@' instruction code */
	IC_AND_ASSIGN_AT,			/**< '&=@' instruction code */
	IC_XOR_ASSIGN_AT,			/**< '^=@' instruction code */
	IC_OR_ASSIGN_AT,			/**< '|=@' instruction code */
	IC_ASSIGN_AT,				/**< '=@' instruction code */
	IC_ADD_ASSIGN_AT,			/**< '+=@' instruction code */
	IC_SUB_ASSIGN_AT,			/**< '-=@' instruction code */
	IC_MUL_ASSIGN_AT,			/**< '*=@' instruction code */
	IC_DIV_ASSIGN_AT,			/**< '/=@' instruction code */
	IC_REM,						/**< '%' instruction code */
	IC_SHL,						/**< '<<' instruction code */
	IC_SHR,						/**< '>>' instruction code */
	IC_AND,						/**< '&' instruction code */
	IC_XOR,						/**< '^' instruction code */
	IC_OR,						/**< '|' instruction code */
	IC_LOG_AND,					/**< '&&' instruction code */
	IC_LOG_OR,					/**< '||' instruction code */
	IC_EQ,						/**< '==' instruction code */
	IC_NE,						/**< '!=' instruction code */
	IC_LT,						/**< '<' instruction code */
	IC_GT,						/**< '>' instruction code */
	IC_LE,						/**< '<=' instruction code */
	IC_GE,						/**< '>=' instruction code */
	IC_ADD,						/**< '+' instruction code */
	IC_SUB,						/**< '-' instruction code */
	IC_MUL,						/**< '*' instruction code */
	IC_DIV,						/**< '/' instruction code */
	IC_POST_INC,				/**< 'POSTINC' instruction code */
	IC_POST_DEC,				/**< 'POSTDEC' instruction code */
	IC_PRE_INC,					/**< 'INC' instruction code */
	IC_PRE_DEC,					/**< 'DEC' instruction code */
	IC_POST_INC_AT,				/**< 'POSTINC@' instruction code */
	IC_POST_DEC_AT,				/**< 'POSTDEC@' instruction code */
	IC_PRE_INC_AT,				/**< 'INC@' instruction code */
	IC_PRE_DEC_AT,				/**< 'DEC@' instruction code */
	IC_UNMINUS,					/**< 'UNMINUS' instruction code */
	IC_NOT = 9052,				/**< 'BITNOT' instruction code */
	IC_LOG_NOT,					/**< 'NOT' instruction code */

	IC_ASSIGN_R = 9057,			/**< '=f' instruction code */
	IC_ADD_ASSIGN_R,			/**< '+=f' instruction code */
	IC_SUB_ASSIGN_R,			/**< '-=f' instruction code */
	IC_MUL_ASSIGN_R,			/**< '*=f' instruction code */
	IC_DIV_ASSIGN_R,			/**< '/=f' instruction code */
	IC_ASSIGN_AT_R = 9068,		/**< '=@f' instruction code */
	IC_ADD_ASSIGN_AT_R,			/**< '+=@f' instruction code */
	IC_SUB_ASSIGN_AT_R,			/**< '-=@f' instruction code */
	IC_MUL_ASSIGN_AT_R,			/**< '*=@f' instruction code */
	IC_DIV_ASSIGN_AT_R,			/**< '/=@f' instruction code */

	IC_EQ_R = 9081,				/**< '==f' instruction code */
	IC_NE_R,					/**< '!=f' instruction code */
	IC_LT_R,					/**< '<f' instruction code */
	IC_GT_R,					/**< '>f' instruction code */
	IC_LE_R,					/**< '<=f' instruction code */
	IC_GE_R,					/**< '>=f' instruction code */
	IC_ADD_R,					/**< '+f' instruction code */
	IC_SUB_R,					/**< '-f' instruction code */
	IC_MUL_R,					/**< '*f' instruction code */
	IC_DIV_R,					/**< '/f' instruction code */
	IC_POST_INC_R,				/**< 'POSTINCf' instruction code */
	IC_POST_DEC_R,				/**< 'POSTDECf' instruction code */
	IC_PRE_INC_R,				/**< 'INCf' instruction code */
	IC_PRE_DEC_R,				/**< 'DECf' instruction code */
	IC_POST_INC_AT_R,			/**< 'POSTINC@f' instruction code */
	IC_POST_DEC_AT_R,			/**< 'POSTDEC@f' instruction code */
	IC_PRE_INC_AT_R,			/**< 'INC@f' instruction code */
	IC_PRE_DEC_AT_R,			/**< 'DEC@f' instruction code */
	IC_UNMINUS_R,				/**< 'UNIMINUSf' instruction code */

	IC_REM_ASSIGN_V = 9201,		/**< '%=V' instruction code */
	IC_SHL_ASSIGN_V,			/**< '<<=V' instruction code */
	IC_SHR_ASSIGN_V,			/**< '>>=V' instruction code */
	IC_AND_ASSIGN_V,			/**< '&=V' instruction code */
	IC_XOR_ASSIGN_V,			/**< '^=V' instruction code */
	IC_OR_ASSIGN_V,				/**< '|=V' instruction code */
	IC_ASSIGN_V,				/**< '=V' instruction code */
	IC_ADD_ASSIGN_V,			/**< '+=V' instruction code */
	IC_SUB_ASSIGN_V,			/**< '-=V' instruction code */
	IC_MUL_ASSIGN_V,			/**< '*=V' instruction code */
	IC_DIV_ASSIGN_V,			/**< '/=V' instruction code */
	IC_REM_ASSIGN_AT_V,			/**< '%=@V' instruction code */
	IC_SHL_ASSIGN_AT_V,			/**< '<<=@V' instruction code */
	IC_SHR_ASSIGN_AT_V,			/**< '>>=@V' instruction code */
	IC_AND_ASSIGN_AT_V,			/**< '&=@V' instruction code */
	IC_XOR_ASSIGN_AT_V,			/**< '^=@V' instruction code */
	IC_OR_ASSIGN_AT_V,			/**< '|=@V' instruction code */
	IC_ASSIGN_AT_V,				/**< '=@V' instruction code */
	IC_ADD_ASSIGN_AT_V,			/**< '+=@V' instruction code */
	IC_SUB_ASSIGN_AT_V,			/**< '-=@V' instruction code */
	IC_MUL_ASSIGN_AT_V,			/**< '*=@V' instruction code */
	IC_DIV_ASSIGN_AT_V,			/**< '/=@V' instruction code */

	IC_ASSIGN_R_V = 9257,		/**< '=fV' instruction code */
	IC_ADD_ASSIGN_R_V,			/**< '+=fV' instruction code */
	IC_SUB_ASSIGN_R_V,			/**< '-=fV' instruction code */
	IC_MUL_ASSIGN_R_V,			/**< *=fV' instruction code */
	IC_DIV_ASSIGN_R_V,			/**< '/=fV' instruction code */
	IC_ASSIGN_AT_R_V = 9268,	/**< '=@fV' instruction code */
	IC_ADD_ASSIGN_AT_R_V,		/**< '+=@fV' instruction code */
	IC_SUB_ASSIGN_AT_R_V,		/**< '-=@fV' instruction code */
	IC_MUL_ASSIGN_AT_R_V,		/**< *=@fV' instruction code */
	IC_DIV_ASSIGN_AT_R_V,		/**< '/=@fV' instruction code */

	IC_POST_INC_V = 9241,		/**< 'POSTINCV' instruction code */
	IC_POST_DEC_V,				/**< 'POSTDECV' instruction code */
	IC_PRE_INC_V,				/**< 'INCV' instruction code */
	IC_PRE_DEC_V,				/**< 'DECV' instruction code */
	IC_POST_INC_AT_V,			/**< 'POSTINC@V' instruction code */
	IC_POST_DEC_AT_V,			/**< 'POSTDEC@V' instruction code */
	IC_PRE_INC_AT_V,			/**< 'INC@V' instruction code */
	IC_PRE_DEC_AT_V,			/**< 'DEC@V' instruction code */

	IC_POST_INC_R_V = 9291,		/**< 'POSTINCfV' instruction code */
	IC_POST_DEC_R_V,			/**< 'POSTDECfV' instruction code */
	IC_PRE_INC_R_V,				/**< 'INCfV' instruction code */
	IC_PRE_DEC_R_V,				/**< 'DECfV' instruction code */
	IC_POST_INC_AT_R_V,			/**< 'POSTINC@fV' instruction code */
	IC_POST_DEC_AT_R_V,			/**< 'POSTDEC@fV' instruction code */
	IC_PRE_INC_AT_R_V,			/**< 'INC@fV' instruction code */
	IC_PRE_DEC_AT_R_V,			/**< 'DEC@fV' instruction code */

	IC_NOP = 9453,				/**< 'NOP' instruction code */
	IC_DEFARR,					/**< 'DEFARR' instruction code */
	IC_LI,						/**< 'LI' instruction code */
	IC_LID,						/**< 'LID' instruction code */
	IC_LOAD,					/**< 'LOAD' instruction code */
	IC_LOADD,					/**< 'LOADD' instruction code */
	IC_LAT,						/**< 'L@' instruction code */
	IC_LATD,					/**< 'L@f' instruction code */
	IC_STOP,					/**< 'STOP' instruction code */
	IC_SELECT,					/**< 'SELECT' instruction code */
	IC_FUNC_BEG,				/**< 'FUNCBEG' instruction code */
	IC_LA,						/**< 'LA' instruction code */
	IC_CALL1,					/**< 'CALL1' instruction code */
	IC_CALL2,					/**< 'CALL2' instruction code */
	IC_RETURN_VAL,				/**< 'RETURNVAL' instruction code */
	IC_RETURN_VOID,				/**< 'RETURNVOID' instruction code */
	IC_B,						/**< 'B' instruction code */
	IC_BE0,						/**< 'BE0' instruction code */
	IC_BNE0,					/**< 'BNE0' instruction code */
	IC_SLICE,					/**< 'SLICE' instruction code */
	IC_WIDEN,					/**< 'WIDEN' instruction code */
	IC_WIDEN1,					/**< 'WIDEN1' instruction code */
	IC_DUPLICATE,				/**< '_DOUBLE' instruction code */
	IC_STRING_INIT,				/**< 'STRINGINIT' instruction code */
	IC_ARR_INIT,				/**< 'ARRINIT' instruction code */
	IC_STRUCT_WITH_ARR,			/**< 'STRUCTWITHARR' instruction code */

	IC_BEG_INIT = 9481,			/**< 'BEGINIT' instruction code */
	IC_ROWING,					/**< 'ROWING' instruction code */
	IC_ROWING_D,				/**< 'ROWINGD' instruction code */

	IC_COPY00 = 9300,			/**< 'COPY00' instruction code */
	IC_COPY01,					/**< 'COPY01' instruction code */
	IC_COPY10,					/**< 'COPY10' instruction code */
	IC_COPY11,					/**< 'COPY11' instruction code */
	IC_COPY0ST,					/**< 'COPY0ST' instruction code */
	IC_COPY1ST,					/**< 'COPY1ST' instruction code */
	IC_COPY2ST,                 /**< 'COPY2ST' instruction code */
	IC_COPY0ST_ASSIGN,			/**< 'COPY0STASS' instruction code */
	IC_COPY1ST_ASSIGN,			/**< 'COPY1STASS' instruction code */
	IC_COPYST,					/**< 'COPYST' instruction code */

	IC_ABS	= 9534,				/**< 'ABS' instruction code */
	IC_SQRT,					/**< 'SQRT' instruction code */
	IC_EXP,						/**< 'EXP' instruction code */
	IC_SIN,						/**< 'SIN' instruction code */
	IC_COS,						/**< 'COS' instruction code */
	IC_LOG,						/**< 'LOG' instruction code */
	IC_LOG10,					/**< 'LOG10' instruction code */
	IC_ASIN,					/**< 'ASIN' instruction code */
	IC_RAND,					/**< 'RAND' instruction code */
	IC_ROUND,					/**< 'ROUND' instruction code */

	IC_STRCPY = 9544,			/**< 'STRCPY' instruction code */
	IC_STRNCPY,					/**< 'STRNCPY' instruction code */
	IC_STRCAT,					/**< 'STRCAT' instruction code */
	IC_STRNCAT,					/**< 'STRNCAT' instruction code */
	IC_STRCMP,					/**< 'STRCMP' instruction code */
	IC_STRNCMP,					/**< 'STRNCMP' instruction code */
	IC_STRSTR,					/**< 'STRSTR' instruction code */
	IC_STRLEN,					/**< 'STRLEN' instruction code */

	IC_MSG_SEND = 9552,			/**< 'MSG_SEND' instruction code */
	IC_MSG_RECEIVE,				/**< 'MSG_RECEIVE' instruction code */
	IC_JOIN,					/**< 'JOIN' instruction code */
	IC_SLEEP,					/**< 'SLEEP' instruction code */
	IC_SEM_CREATE,				/**< 'SEMCREATE' instruction code */
	IC_SEM_WAIT,				/**< 'SEMWAIT' instruction code */
	IC_SEM_POST,				/**< 'SEMPOST' instruction code */
	IC_CREATE,					/**< 'CREATE' instruction code */
	IC_INIT,					/**< 'INIT' instruction code */
	IC_DESTROY,					/**< 'DESTROY' instruction code */
	IC_EXIT,					/**< 'EXIT' instruction code */
	IC_GETNUM,					/**< 'GETNUM' instruction code */

	IC_UPB = 9588,				/**< 'UPB' instruction code */
	IC_ROBOT_SEND_INT,			/**< 'SEND_INT' instruction code */
	IC_ROBOT_SEND_FLOAT,		/**< 'SEND_FLOAT' instruction code */
	IC_ROBOT_SEND_STRING,		/**< 'SEND_STRING' instruction code */
	IC_ROBOT_RECEIVE_INT,		/**< 'RECEIVE_INT' instruction code */
	IC_ROBOT_RECEIVE_FLOAT,		/**< 'RECEIVE_FLOAT' instruction code */
	IC_ROBOT_RECEIVE_STRING,	/**< 'RECEIVE_STRING' instruction code */
	IC_ASSERT,					/**< 'ASSERT' instruction code */
	IC_ABSI	= 9651,				/**< 'ABSI' instruction code */

	IC_FOPEN,					/**< 'FOPEN' instruction code */
	IC_FCLOSE,					/**< 'FCLOSE' instruction code */
	IC_FGETC,					/**< 'FGETC' instruction code */
	IC_FPUTC,					/**< 'FPUTC' instruction code */

	MAX_INSTRUCTION_CODE,
} instruction_t;


/**
 *	Convert standard function id to corresponding function instruction
 *
 *	@param	func			Function id
 *
 *	@return	Function instruction
 */
instruction_t builtin_to_instruction(const builtin_t func);

/**
 *	Convert unary operator to corresponding instruction
 *
 *	@param	op				Unary operator
 *
 *	@return	Instruction
 */
instruction_t unary_to_instruction(const unary_t op);

/**
 *	Convert binary operator to corresponding instruction
 *
 *	@param	op				Binary operator
 *
 *	@return	Instruction
 */
instruction_t binary_to_instruction(const binary_t op);


/**
 *	Convert instruction to corresponding address version
 *
 *	@param	instruction		Instruction
 *
 *	@return	Address version of instruction
 */
instruction_t instruction_to_address_ver(const instruction_t instruction);

/**
 *	Convert instruction to corresponding floating version
 *
 *	@param	instruction		Instruction
 *
 *	@return	Floating version of instruction
 */
instruction_t instruction_to_floating_ver(const instruction_t instruction);

/**
 *	Convert instruction to corresponding void version
 *
 *	@param	instruction		Instruction
 *
 *	@return	Void version of instruction
 */
instruction_t instruction_to_void_ver(const instruction_t instruction);

#ifdef __cplusplus
} /* extern "C" */
#endif
