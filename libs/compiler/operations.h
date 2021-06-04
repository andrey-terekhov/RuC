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

#include "tokens.h"
#include <stdio.h>
#include "instructions.h"


#ifdef __cplusplus
extern "C" {
#endif

static const size_t DISPL_TO_FLOAT = 50;

typedef enum OPERATION
{
	BEGIN_FINAL_OPERATION,

	OP_AD_LOG_AND 			= 7000,					/**< 'ADLOGAND' node */
	OP_AD_LOG_OR,									/**< 'ADLOGOR' node */
	OP_NULL					= IC_NOP,				/**< Empty node */
	OP_WIDEN1				= IC_WIDEN1,			/**< 'WIDEN1' node */
	OP_WIDEN				= IC_WIDEN,				/**< 'WIDEN' node */
	OP_STRINGINIT			= IC_STRINGINIT,		/**< 'STRINGINIT' node */
	OP_ROWING				= IC_ROWING,			/**< 'ROWING' node */
	OP_ROWINGD				= IC_ROWINGD,			/**< 'ROWINGD' node */
	OP_ASSERT				= IC_ASSERT,			/**< 'ASSERT' node */
	OP_UPB					= IC_UPB,				/**< 'UPB' node */

	// Struct copying functions
	OP_COPY00				= IC_COPY00,			/**< 'COPY00' node */
	OP_COPY01				= IC_COPY01,			/**< 'COPY01' node */
	OP_COPY10				= IC_COPY10,			/**< 'COPY10' node */
	OP_COPY11				= IC_COPY11,			/**< 'COPY11' node */
	OP_COPY0ST				= IC_COPY0ST,			/**< 'COPY0ST' node */
	OP_COPY1ST				= IC_COPY1ST,			/**< 'COPY1ST' node */
	OP_COPY0ST_ASSIGN		= IC_COPY0STASS,		/**< 'COPY0ST_ASSIGN' node */
	OP_COPY1ST_ASSIGN		= IC_COPY1STASS,		/**< 'COPY1ST_ASSIGN' node */
	OP_COPYST				= IC_COPYST,			/**< 'COPYST' node */

	// Math functions
	OP_ABSI					= IC_ABSI,				/**< 'ABSI' node */
	OP_ABS					= IC_ABS,				/**< 'ABS' node */
	OP_SQRT					= IC_SQRT,				/**< 'SQRT' node */
	OP_EXP					= IC_EXP,				/**< 'EXP' node */
	OP_SIN					= IC_SIN,				/**< 'SIN' node */
	OP_COS					= IC_COS,				/**< 'COS' node */
	OP_LOG					= IC_LOG,				/**< 'LOG' node */
	OP_LOG10				= IC_LOG10,				/**< 'LOG10' node */
	OP_ASIN					= IC_ASIN,				/**< 'ASIN' node */
	OP_RAND					= IC_RAND,				/**< 'RAND' node */
	OP_ROUND				= IC_ROUND,				/**< 'ROUND' node */

	// String functions
	OP_STRCPY				= IC_STRCPY,			/**< 'STRCPY' node */
	OP_STRNCPY				= IC_STRNCPY,			/**< 'STRNCPY' node */
	OP_STRCAT				= IC_STRCAT,			/**< 'STRCAT' node */
	OP_STRNCAT				= IC_STRNCAT,			/**< 'STRNCAT' node */
	OP_STRCMP				= IC_STRCMP,			/**< 'STRCMP' node */
	OP_STRNCMP				= IC_STRNCMP,			/**< 'STRNCMP' node */
	OP_STRSTR				= IC_STRSTR,			/**< 'STRSTR' node */
	OP_STRLEN				= IC_STRLEN,			/**< 'STRLEN' node */

	// Thread functions
	OP_MSG_SEND				= IC_MSG_SEND,			/**< 'MSG_SEND' node */
	OP_MSG_RECEIVE			= IC_MSG_RECEIVE,		/**< 'MSG_RECEIVE' node */
	OP_JOIN					= IC_JOIN,				/**< 'JOIN' node */
	OP_SLEEP				= IC_SLEEP,				/**< 'SLEEP' node */
	OP_SEM_CREATE			= IC_SEMCREATE,			/**< 'SEM_CREATE' node */
	OP_SEM_WAIT				= IC_SEMWAIT,			/**< 'SEM_WAIT' node */
	OP_SEM_POST				= IC_SEMPOST,			/**< 'SEM_POST' node */
	OP_CREATE				= IC_CREATE,			/**< 'CREATE' node */
	OP_INIT					= IC_INIT,				/**< 'INIT' node */
	OP_DESTROY				= IC_DESTROY,			/**< 'DESTROY' node */
	OP_EXIT					= IC_EXIT,				/**< 'EXIT' node */
	OP_GETNUM				= IC_GETNUM,			/**< 'GETNUM' node */

	// Robot functions
	OP_SEND_INT				= IC_SEND_INT,			/**< 'SEND_INT' node */
	OP_SEND_FLOAT			= IC_SEND_FLOAT,		/**< 'SEND_FLOAT' node */
	OP_SEND_STRING			= IC_SEND_STRING,		/**< 'SEND_STRING' node */
	OP_RECEIVE_INT			= IC_RECEIVE_INT,		/**< 'RECEIVE_INT' node */
	OP_RECEIVE_FLOAT		= IC_RECEIVE_FLOAT,		/**< 'RECEIVE_FLOAT' node */
	OP_RECEIVE_STRING		= IC_RECEIVE_STRING,	/**< 'RECEIVE_STRING' node */

	// Unary operators
	OP_NOT					= IC_NOT,				/**< 'BITNOT' node */
	OP_LOG_NOT				= IC_LOGNOT,			/**< 'NOT' node */
	OP_UNMINUS				= IC_UNMINUS,			/**< 'UNMINUS' node */
	OP_UNMINUS_R			= IC_UNMINUSR,			/**< 'UNIMINUSf' node */

	OP_PREINC				= IC_INC,				/**< 'INC' node */
	OP_PREINC_R				= IC_INCR,				/**< 'INCf' node */
	OP_PREINC_AT			= IC_INCAT,				/**< 'INC@' node */
	OP_PREINC_AT_R			= IC_INCATR,			/**< 'PREINC@f' node */
	OP_PREINC_V				= IC_INCV,				/**< 'PREINCV' node */
	OP_PREINC_R_V			= IC_INCRV,				/**< 'PREINCfV' node */
	OP_PREINC_AT_V			= IC_INCATV,			/**< 'PREINC@V' node */
	OP_PREINC_AT_R_V		= IC_INCATRV,			/**< 'PREINC@fV' node */

	OP_PREDEC				= IC_DEC,				/**< 'DEC' node */
	OP_PREDEC_R				= IC_DECR,				/**< 'DECf' node */
	OP_PREDEC_AT			= IC_DECAT,				/**< 'DEC@' node */
	OP_PREDEC_AT_R			= IC_DECATR,			/**< 'PREDEC@f' node */
	OP_PREDEC_V				= IC_DECV,				/**< 'PREDECV' node */
	OP_PREDEC_R_V			= IC_DECRV,				/**< 'PREDECfV' node */
	OP_PREDEC_AT_V			= IC_DECATV,			/**< 'PREDEC@V' node */
	OP_PREDEC_AT_R_V		= IC_DECATRV,			/**< 'PREDEC@fV' node */

	OP_POSTINC				= IC_POSTINC,			/**< 'POSTINC' node */
	OP_POSTINC_R			= IC_POSTINCR,			/**< 'POSTINCf' node */
	OP_POSTINC_AT			= IC_POSTINCAT,			/**< 'POSTINC@' node */
	OP_POSTINC_AT_R			= IC_POSTINCATR,		/**< 'POSTINC@f' node */
	OP_POSTINC_V			= IC_POSTINCV,			/**< 'POSTINCV' node */
	OP_POSTINC_R_V			= IC_POSTINCRV,			/**< 'POSTINCfV' node */
	OP_POSTINC_AT_V			= IC_POSTINCATV,		/**< 'POSTINC@V' node */
	OP_POSTINC_AT_R_V		= IC_POSTINCATRV,		/**< 'POSTINC@fV' node */

	OP_POSTDEC				= IC_POSTDEC,			/**< 'POSTDEC' node */
	OP_POSTDEC_R			= IC_POSTDECR,			/**< 'POSTDECf' node */
	OP_POSTDEC_AT			= IC_POSTDECAT,			/**< 'POSTDEC@' node */
	OP_POSTDEC_AT_R			= IC_POSTDECATR,		/**< 'POSTDEC@f' node */
	OP_POSTDEC_V			= IC_POSTDECV,			/**< 'POSTDECV' node */
	OP_POSTDEC_R_V			= IC_POSTDECRV,			/**< 'POSTDECfV' node */
	OP_POSTDEC_AT_V			= IC_POSTDECATV,		/**< 'POSTDEC@V' node */
	OP_POSTDEC_AT_R_V		= IC_POSTDECATRV,		/**< 'POSTDEC@fV' node */

	// Binary operators
	OP_MUL					= IC_MULT,				/**< '*' node */
	OP_MUL_R				= IC_MULTR,				/**< '*f' node */

	OP_DIV					= IC_DIV,				/**< '/' node */
	OP_DIV_R				= IC_DIVR,				/**< '/f' node */

	OP_ADD					= IC_PLUS,				/**< '+' node */
	OP_ADD_R				= IC_PLUSR,				/**< '+f' node */

	OP_SUB					= IC_MINUS,				/**< '-' node */
	OP_SUB_R				= IC_MINUSR,			/**< '-f' node */

	OP_LT					= IC_LT,				/**< '<' node */
	OP_LT_R					= IC_LTR,				/**< '<f' node */

	OP_GT					= IC_GT,				/**< '>' node */
	OP_GT_R					= IC_GTR,				/**< '>f' node */

	OP_LE					= IC_LE,				/**< '<=' node */
	OP_LE_R					= IC_LER,				/**< '<=f' node */

	OP_GE					= IC_GE,				/**< '>=' node */
	OP_GE_R					= IC_GER,				/**< '>=f' node */

	OP_EQ					= IC_EQEQ,				/**< '==' node */
	OP_EQ_R					= IC_EQEQR,				/**< '==f' node */

	OP_NE					= IC_NOTEQ,				/**< '!=' node */
	OP_NE_R					= IC_NOTEQR,			/**< '!=f' node */

	OP_SHL					= IC_SHL,				/**< '<<' node */
	OP_SHR					= IC_SHR,				/**< '>>' node */
	OP_REM					= IC_REM,				/**< '%' node */
	OP_AND					= IC_AND,				/**< '&' node */
	OP_XOR					= IC_EXOR,				/**< '^' node */
	OP_OR					= IC_OR,				/**< '|' node */
	OP_LOG_AND				= IC_LOGAND,			/**< '&&' node */
	OP_LOG_OR				= IC_LOGOR,				/**< '||' node */

	OP_ASSIGN				= IC_ASS,				/**< '=' node */
	OP_ASSIGN_R				= IC_ASSR,				/**< '=f' node */
	OP_ASSIGN_AT			= IC_ASSAT,				/**< '=@' node */
	OP_ASSIGN_AT_R			= IC_ASSATR,			/**< '=@f' node */
	OP_ASSIGN_V				= IC_ASSV,				/**< '=V' node */
	OP_ASSIGN_R_V			= IC_ASSRV,				/**< '=fV' node */
	OP_ASSIGN_AT_V			= IC_ASSATV,			/**< '=@V' node */
	OP_ASSIGN_AT_R_V		= IC_ASSATRV,			/**< '=@fV' node */

	OP_MUL_ASSIGN			= IC_MULTASS,			/**< '*=' node */
	OP_MUL_ASSIGN_R			= IC_MULTASSR,			/**< '*=f' node */
	OP_MUL_ASSIGN_AT		= IC_MULTASSAT,			/**< '*=@' node */
	OP_MUL_ASSIGN_AT_R		= IC_MULTASSATR,		/**< '*=@f' node */
	OP_MUL_ASSIGN_V			= IC_MULTASSV,			/**< '*=V' node */
	OP_MUL_ASSIGN_R_V		= IC_MULTASSRV,			/**< '*=fV' node */
	OP_MUL_ASSIGN_AT_V		= IC_MULTASSATV,		/**< '*=@V' node */
	OP_MUL_ASSIGN_AT_R_V	= IC_MULTASSATRV,		/**< '*=@fV' node */

	OP_DIV_ASSIGN			= IC_DIVASS,			/**< '/=' node */
	OP_DIV_ASSIGN_R			= IC_DIVASSR,			/**< '/=f' node */
	OP_DIV_ASSIGN_AT		= IC_DIVASSAT,			/**< '/=@' node */
	OP_DIV_ASSIGN_AT_R		= IC_DIVASSATR,			/**< '/=@f' node */
	OP_DIV_ASSIGN_V			= IC_DIVASSV,			/**< '/=V' node */
	OP_DIV_ASSIGN_R_V		= IC_DIVASSRV,			/**< '/=fV' node */
	OP_DIV_ASSIGN_AT_V		= IC_DIVASSATV,			/**< '/=@V' node */
	OP_DIV_ASSIGN_AT_R_V	= IC_DIVASSATRV,		/**< '/=@fV' node */

	OP_ADD_ASSIGN			= IC_PLUSASS,			/**< '+=' node */
	OP_ADD_ASSIGN_R			= IC_PLUSASSR,			/**< '+=f' node */
	OP_ADD_ASSIGN_AT		= IC_PLUSASSAT,			/**< '+=@' node */
	OP_ADD_ASSIGN_AT_R		= IC_PLUSASSATR,		/**< '+=@f' node */
	OP_ADD_ASSIGN_V			= IC_PLUSASSV,			/**< '+=V' node */
	OP_ADD_ASSIGN_R_V		= IC_PLUSASSRV,			/**< '+=fV' node */
	OP_ADD_ASSIGN_AT_V		= IC_PLUSASSATV,		/**< '+=@V' node */
	OP_ADD_ASSIGN_AT_R_V	= IC_PLUSASSATRV,		/**< '+=@fV' node */

	OP_SUB_ASSIGN			= IC_MINUSASS,			/**< '-=' node */
	OP_SUB_ASSIGN_R			= IC_MINUSASSR,			/**< '-=f' node */
	OP_SUB_ASSIGN_AT		= IC_MINUSASSAT,		/**< '-=@' node */
	OP_SUB_ASSIGN_AT_R		= IC_MINUSASSATR,		/**< '-=@f' node */
	OP_SUB_ASSIGN_V			= IC_MINUSASSV,			/**< '-=V' node */
	OP_SUB_ASSIGN_R_V		= IC_MINUSASSRV,		/**< '-=fV' node */
	OP_SUB_ASSIGN_AT_V		= IC_MINUSASSATV,		/**< '-=@V' node */
	OP_SUB_ASSIGN_AT_R_V	= IC_MINUSASSATRV,		/**< '-=@fV' node */

	OP_REM_ASSIGN			= IC_REMASS,			/**< '%=' node */
	OP_REM_ASSIGN_AT		= IC_REMASSAT,			/**< '%=@' node */
	OP_REM_ASSIGN_V			= IC_REMASSV,			/**< '%=V' node */
	OP_REM_ASSIGN_AT_V		= IC_REMASSATV,			/**< '%=@V' node */

	OP_SHL_ASSIGN			= IC_SHLASS,			/**< '<<=' node */
	OP_SHL_ASSIGN_AT		= IC_SHLASSAT,			/**< '<<=@' node */
	OP_SHL_ASSIGN_V			= IC_SHLASSV,			/**< '<<=V' node */
	OP_SHL_ASSIGN_AT_V		= IC_SHLASSATV,			/**< '<<=@V' node */

	OP_SHR_ASSIGN			= IC_SHRASS,			/**< '>>=' node */
	OP_SHR_ASSIGN_AT		= IC_SHRASSAT,			/**< '>>=@' node */
	OP_SHR_ASSIGN_V			= IC_SHRASSV,			/**< '>>=V' node */
	OP_SHR_ASSIGN_AT_V		= IC_SHRASSATV,			/**< '>>=@V' node */

	OP_AOP_ASSIGN			= IC_ANDASS,			/**< '&=' node */
	OP_AOP_ASSIGN_AT		= IC_ANDASSAT,			/**< '&=@' node */
	OP_AOP_ASSIGN_V			= IC_ANDASSV,			/**< '&=V' node */
	OP_AOP_ASSIGN_AT_V		= IC_ANDASSATV,			/**< '&=@V' node */

	OP_XOR_ASSIGN			= IC_EXORASS,			/**< '^=' node */
	OP_XOR_ASSIGN_AT		= IC_EXORASSAT,			/**< '^=@' node */
	OP_XOR_ASSIGN_V			= IC_EXORASSV,			/**< '^=V' node */
	OP_XOR_ASSIGN_AT_V		= IC_EXORASSATV,		/**< '^=@V' node */

	OP_OR_ASSIGN			= IC_ORASS,				/**< '|=' node */
	OP_OR_ASSIGN_AT			= IC_ORASSAT,			/**< '|=@' node */
	OP_OR_ASSIGN_V			= IC_ORASSV,			/**< '|=V' node */
	OP_OR_ASSIGN_AT_V		= IC_ORASSATV,			/**< '|=@V' node */

	EOP_FINAL_OPERATION = MAX_INSTRUCTION_CODE,

	// Statement
	OP_LABEL,				/**< Label statement node */
	OP_CASE,				/**< Case statement node */
	OP_DEFAULT,				/**< Default statement node */
	OP_BLOCK,				/**< Compound statement node */
	OP_IF,					/**< If statement node */
	OP_SWITCH,				/**< Switch statement node */
	OP_WHILE,				/**< While statement node */
	OP_DO,					/**< Do statement node */
	OP_FOR,					/**< For statement node */
	OP_GOTO,				/**< Goto statement node */
	OP_CONTINUE,			/**< Continue statement node */
	OP_BREAK,				/**< Break statement node */
	OP_RETURN_VOID,			/**< Void return statement node */
	OP_RETURN_VAL,			/**< Valued return statement node */
	OP_CREATE_DIRECT,		/**< Create direct thread node */
	OP_EXIT_DIRECT,			/**< Exit direct thread node */
	OP_PRINTID,				/**< Printid statement node */
	OP_PRINT,				/**< Print statement node */
	OP_GETID,				/**< Getid statement node */
	OP_PRINTF,				/**< Printf statement node */

	// Declarations
	OP_DECL_ID,				/**< Identifier declaration node */
	OP_DECL_ARR,			/**< Array declaration node */
	OP_DECL_STRUCT,			/**< Struct declaration node */
	OP_FUNC_DEF,			/**< Function definition node */
	OP_ARRAY_INIT,			/**< Array inition node */
	OP_STRUCT_INIT,			/**< Struct inition node */

	// End nodes
	OP_BLOCK_END,			/**< End of block node */
	OP_DECL_STRUCT_END,		/**< End of struct declaration node */
	OP_EXPR_END,			/**< End of expression node */

	// Expressions
	OP_CONDITIONAL,			/**< Ternary operator node */
	OP_IDENT,				/**< Identifier node */
	OP_IDENT_TO_ADDR,		/**< Identifier to address node */
	OP_SLICE,				/**< Slice node */
	OP_SLICE_IDENT,			/**< Slice from identifier node */
	OP_SELECT,				/**< Select node */
	OP_CALL1,				/**< 'Call1' node */
	OP_CALL2,				/**< 'Call2' node */
	OP_STRING,				/**< String literal node */
	OP_IDENT_TO_VAL,		/**< Value of integer variable node */
	OP_ADDR_TO_VAL,			/**< Address to integer value node */
	OP_CONST,				/**< Integer constant node */

	OP_STRING_D,			/**< Row of doubles node */
	OP_IDENT_TO_VAL_D,		/**< Value of double variable node */
	OP_ADDR_TO_VAL_D,		/**< Address to double value node */
	OP_CONST_D,				/**< Double constant node node */
} operation_t;


/**
 *	Convert token to corresponding binary operator
 *
 *	@param	token	Token
 *
 *	@return	Operation
 */
operation_t token_to_binary(const token_t token);

/**
 *	Convert token to corresponding unary operator
 *
 *	@param	token	Token
 *
 *	@return	Operation
 */
operation_t token_to_unary(const token_t token);

/**
 *	Convert token to corresponding function operator
 *
 *	@param	token	Token
 *
 *	@return	Operation
 */
operation_t token_to_function(const token_t token);

/**
 *	Convert operation to corresponding address version
 *
 *	@param	operation	Operator
 *
 *	@return	Address version operator
 */
operation_t operation_to_address_ver(const operation_t operation);

/**
 *	Convert operation to corresponding void version
 *
 *	@param	operation	Operator
 *
 *	@return	Void version operator
 */
operation_t operation_to_void_ver(const operation_t operation);

/**
 *	Convert operation to corresponding float version
 *
 *	@param	operation	Operator
 *
 *	@return	Float version operator
 */
operation_t operation_to_float_ver(const operation_t operation);

/**
 *	Check if node type is assignment operation
 *
 *	@param	operation	Operator type
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int operation_is_assignment(const operation_t operation);

#ifdef __cplusplus
} /* extern "C" */
#endif
