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

typedef enum NODE
{
	BEGIN_FINAL_OPERATION,

	ND_AD_LOG_AND 			= 7000,					/**< 'ADLOGAND' node */
	ND_AD_LOG_OR,									/**< 'ADLOGOR' node */
	ND_NULL					= IC_NOP,				/**< Empty node */
	ND_WIDEN1				= IC_WIDEN1,			/**< 'WIDEN1' node */
	ND_WIDEN				= IC_WIDEN,				/**< 'WIDEN' node */
	ND_STRINGINIT			= IC_STRINGINIT,		/**< 'STRINGINIT' node */
	ND_ROWING				= IC_ROWING,			/**< 'ROWING' node */
	ND_ROWINGD				= IC_ROWINGD,			/**< 'ROWINGD' node */
	ND_ASSERT				= IC_ASSERT,			/**< 'ASSERT' node */
	ND_UPB					= IC_UPB,				/**< 'UPB' node */

	// Struct copying functions
	ND_COPY00				= IC_COPY00,			/**< 'COPY00' node */
	ND_COPY01				= IC_COPY01,			/**< 'COPY01' node */
	ND_COPY10				= IC_COPY10,			/**< 'COPY10' node */
	ND_COPY11				= IC_COPY11,			/**< 'COPY11' node */
	ND_COPY0ST				= IC_COPY0ST,			/**< 'COPY0ST' node */
	ND_COPY1ST				= IC_COPY1ST,			/**< 'COPY1ST' node */
	ND_COPY0ST_ASSIGN		= IC_COPY0STASS,		/**< 'COPY0ST_ASSIGN' node */
	ND_COPY1ST_ASSIGN		= IC_COPY1STASS,		/**< 'COPY1ST_ASSIGN' node */
	ND_COPYST				= IC_COPYST,			/**< 'COPYST' node */

	// Math functions
	ND_ABSI					= IC_ABSI,				/**< 'ABSI' node */
	ND_ABS					= IC_ABS,				/**< 'ABS' node */
	ND_SQRT					= IC_SQRT,				/**< 'SQRT' node */
	ND_EXP					= IC_EXP,				/**< 'EXP' node */
	ND_SIN					= IC_SIN,				/**< 'SIN' node */
	ND_COS					= IC_COS,				/**< 'COS' node */
	ND_LOG					= IC_LOG,				/**< 'LOG' node */
	ND_LOG10				= IC_LOG10,				/**< 'LOG10' node */
	ND_ASIN					= IC_ASIN,				/**< 'ASIN' node */
	ND_RAND					= IC_RAND,				/**< 'RAND' node */
	ND_ROUND				= IC_ROUND,				/**< 'ROUND' node */

	// String functions
	ND_STRCPY				= IC_STRCPY,			/**< 'STRCPY' node */
	ND_STRNCPY				= IC_STRNCPY,			/**< 'STRNCPY' node */
	ND_STRCAT				= IC_STRCAT,			/**< 'STRCAT' node */
	ND_STRNCAT				= IC_STRNCAT,			/**< 'STRNCAT' node */
	ND_STRCMP				= IC_STRCMP,			/**< 'STRCMP' node */
	ND_STRNCMP				= IC_STRNCMP,			/**< 'STRNCMP' node */
	ND_STRSTR				= IC_STRSTR,			/**< 'STRSTR' node */
	ND_STRLEN				= IC_STRLEN,			/**< 'STRLEN' node */

	// Thread functions
	ND_MSG_SEND				= IC_MSG_SEND,			/**< 'MSG_SEND' node */
	ND_MSG_RECEIVE			= IC_MSG_RECEIVE,		/**< 'MSG_RECEIVE' node */
	ND_JOIN					= IC_JOIN,				/**< 'JOIN' node */
	ND_SLEEP				= IC_SLEEP,				/**< 'SLEEP' node */
	ND_SEM_CREATE			= IC_SEMCREATE,			/**< 'SEM_CREATE' node */
	ND_SEM_WAIT				= IC_SEMWAIT,			/**< 'SEM_WAIT' node */
	ND_SEM_POST				= IC_SEMPOST,			/**< 'SEM_POST' node */
	ND_CREATE				= IC_CREATE,			/**< 'CREATE' node */
	ND_INIT					= IC_INIT,				/**< 'INIT' node */
	ND_DESTROY				= IC_DESTROY,			/**< 'DESTROY' node */
	ND_EXIT					= IC_EXIT,				/**< 'EXIT' node */
	ND_GETNUM				= IC_GETNUM,			/**< 'GETNUM' node */

	// Robot functions
	ND_SEND_INT				= IC_SEND_INT,			/**< 'SEND_INT' node */
	ND_SEND_FLOAT			= IC_SEND_FLOAT,		/**< 'SEND_FLOAT' node */
	ND_SEND_STRING			= IC_SEND_STRING,		/**< 'SEND_STRING' node */
	ND_RECEIVE_INT			= IC_RECEIVE_INT,		/**< 'RECEIVE_INT' node */
	ND_RECEIVE_FLOAT		= IC_RECEIVE_FLOAT,		/**< 'RECEIVE_FLOAT' node */
	ND_RECEIVE_STRING		= IC_RECEIVE_STRING,	/**< 'RECEIVE_STRING' node */

	// Unary operators
	ND_NOT					= IC_NOT,				/**< 'BITNOT' node */
	ND_LOG_NOT				= IC_LOGNOT,			/**< 'NOT' node */
	ND_UNMINUS				= IC_UNMINUS,			/**< 'UNMINUS' node */
	ND_UNMINUS_R			= IC_UNMINUSR,			/**< 'UNIMINUSf' node */

	ND_PREINC				= IC_INC,				/**< 'INC' node */
	ND_PREINC_R				= IC_INCR,				/**< 'INCf' node */
	ND_PREINC_AT			= IC_INCAT,				/**< 'INC@' node */
	ND_PREINC_AT_R			= IC_INCATR,			/**< 'PREINC@f' node */
	ND_PREINC_V				= IC_INCV,				/**< 'PREINCV' node */
	ND_PREINC_R_V			= IC_INCRV,				/**< 'PREINCfV' node */
	ND_PREINC_AT_V			= IC_INCATV,			/**< 'PREINC@V' node */
	ND_PREINC_AT_R_V		= IC_INCATRV,			/**< 'PREINC@fV' node */

	ND_PREDEC				= IC_DEC,				/**< 'DEC' node */
	ND_PREDEC_R				= IC_DECR,				/**< 'DECf' node */
	ND_PREDEC_AT			= IC_DECAT,				/**< 'DEC@' node */
	ND_PREDEC_AT_R			= IC_DECATR,			/**< 'PREDEC@f' node */
	ND_PREDEC_V				= IC_DECV,				/**< 'PREDECV' node */
	ND_PREDEC_R_V			= IC_DECRV,				/**< 'PREDECfV' node */
	ND_PREDEC_AT_V			= IC_DECATV,			/**< 'PREDEC@V' node */
	ND_PREDEC_AT_R_V		= IC_DECATRV,			/**< 'PREDEC@fV' node */

	ND_POSTINC				= IC_POSTINC,			/**< 'POSTINC' node */
	ND_POSTINC_R			= IC_POSTINCR,			/**< 'POSTINCf' node */
	ND_POSTINC_AT			= IC_POSTINCAT,			/**< 'POSTINC@' node */
	ND_POSTINC_AT_R			= IC_POSTINCATR,		/**< 'POSTINC@f' node */
	ND_POSTINC_V			= IC_POSTINCV,			/**< 'POSTINCV' node */
	ND_POSTINC_R_V			= IC_POSTINCRV,			/**< 'POSTINCfV' node */
	ND_POSTINC_AT_V			= IC_POSTINCATV,		/**< 'POSTINC@V' node */
	ND_POSTINC_AT_R_V		= IC_POSTINCATRV,		/**< 'POSTINC@fV' node */

	ND_POSTDEC				= IC_POSTDEC,			/**< 'POSTDEC' node */
	ND_POSTDEC_R			= IC_POSTDECR,			/**< 'POSTDECf' node */
	ND_POSTDEC_AT			= IC_POSTDECAT,			/**< 'POSTDEC@' node */
	ND_POSTDEC_AT_R			= IC_POSTDECATR,		/**< 'POSTDEC@f' node */
	ND_POSTDEC_V			= IC_POSTDECV,			/**< 'POSTDECV' node */
	ND_POSTDEC_R_V			= IC_POSTDECRV,			/**< 'POSTDECfV' node */
	ND_POSTDEC_AT_V			= IC_POSTDECATV,		/**< 'POSTDEC@V' node */
	ND_POSTDEC_AT_R_V		= IC_POSTDECATRV,		/**< 'POSTDEC@fV' node */

	// Binary operators
	ND_MUL					= IC_MULT,				/**< '*' node */
	ND_MUL_R				= IC_MULTR,				/**< '*f' node */

	ND_DIV					= IC_DIV,				/**< '/' node */
	ND_DIV_R				= IC_DIVR,				/**< '/f' node */

	ND_ADD					= IC_PLUS,				/**< '+' node */
	ND_ADD_R				= IC_PLUSR,				/**< '+f' node */

	ND_SUB					= IC_MINUS,				/**< '-' node */
	ND_SUB_R				= IC_MINUSR,			/**< '-f' node */

	ND_LT					= IC_LT,				/**< '<' node */
	ND_LT_R					= IC_LTR,				/**< '<f' node */

	ND_GT					= IC_GT,				/**< '>' node */
	ND_GT_R					= IC_GTR,				/**< '>f' node */

	ND_LE					= IC_LE,				/**< '<=' node */
	ND_LE_R					= IC_LER,				/**< '<=f' node */

	ND_GE					= IC_GE,				/**< '>=' node */
	ND_GE_R					= IC_GER,				/**< '>=f' node */

	ND_EQ					= IC_EQEQ,				/**< '==' node */
	ND_EQ_R					= IC_EQEQR,				/**< '==f' node */

	ND_NE					= IC_NOTEQ,				/**< '!=' node */
	ND_NE_R					= IC_NOTEQR,			/**< '!=f' node */

	ND_SHL					= IC_SHL,				/**< '<<' node */
	ND_SHR					= IC_SHR,				/**< '>>' node */
	ND_REM					= IC_REM,				/**< '%' node */
	ND_AND					= IC_AND,				/**< '&' node */
	ND_XOR					= IC_EXOR,				/**< '^' node */
	ND_OR					= IC_OR,				/**< '|' node */
	ND_LOG_AND				= IC_LOGAND,			/**< '&&' node */
	ND_LOG_OR				= IC_LOGOR,				/**< '||' node */

	ND_ASSIGN				= IC_ASS,				/**< '=' node */
	ND_ASSIGN_R				= IC_ASSR,				/**< '=f' node */
	ND_ASSIGN_AT			= IC_ASSAT,				/**< '=@' node */
	ND_ASSIGN_AT_R			= IC_ASSATR,			/**< '=@f' node */
	ND_ASSIGN_V				= IC_ASSV,				/**< '=V' node */
	ND_ASSIGN_R_V			= IC_ASSRV,				/**< '=fV' node */
	ND_ASSIGN_AT_V			= IC_ASSATV,			/**< '=@V' node */
	ND_ASSIGN_AT_R_V		= IC_ASSATRV,			/**< '=@fV' node */

	ND_MUL_ASSIGN			= IC_MULTASS,			/**< '*=' node */
	ND_MUL_ASSIGN_R			= IC_MULTASSR,			/**< '*=f' node */
	ND_MUL_ASSIGN_AT		= IC_MULTASSAT,			/**< '*=@' node */
	ND_MUL_ASSIGN_AT_R		= IC_MULTASSATR,		/**< '*=@f' node */
	ND_MUL_ASSIGN_V			= IC_MULTASSV,			/**< '*=V' node */
	ND_MUL_ASSIGN_R_V		= IC_MULTASSRV,			/**< '*=fV' node */
	ND_MUL_ASSIGN_AT_V		= IC_MULTASSATV,		/**< '*=@V' node */
	ND_MUL_ASSIGN_AT_R_V	= IC_MULTASSATRV,		/**< '*=@fV' node */

	ND_DIV_ASSIGN			= IC_DIVASS,			/**< '/=' node */
	ND_DIV_ASSIGN_R			= IC_DIVASSR,			/**< '/=f' node */
	ND_DIV_ASSIGN_AT		= IC_DIVASSAT,			/**< '/=@' node */
	ND_DIV_ASSIGN_AT_R		= IC_DIVASSATR,			/**< '/=@f' node */
	ND_DIV_ASSIGN_V			= IC_DIVASSV,			/**< '/=V' node */
	ND_DIV_ASSIGN_R_V		= IC_DIVASSRV,			/**< '/=fV' node */
	ND_DIV_ASSIGN_AT_V		= IC_DIVASSATV,			/**< '/=@V' node */
	ND_DIV_ASSIGN_AT_R_V	= IC_DIVASSATRV,		/**< '/=@fV' node */

	ND_ADD_ASSIGN			= IC_PLUSASS,			/**< '+=' node */
	ND_ADD_ASSIGN_R			= IC_PLUSASSR,			/**< '+=f' node */
	ND_ADD_ASSIGN_AT		= IC_PLUSASSAT,			/**< '+=@' node */
	ND_ADD_ASSIGN_AT_R		= IC_PLUSASSATR,		/**< '+=@f' node */
	ND_ADD_ASSIGN_V			= IC_PLUSASSV,			/**< '+=V' node */
	ND_ADD_ASSIGN_R_V		= IC_PLUSASSRV,			/**< '+=fV' node */
	ND_ADD_ASSIGN_AT_V		= IC_PLUSASSATV,		/**< '+=@V' node */
	ND_ADD_ASSIGN_AT_R_V	= IC_PLUSASSATRV,		/**< '+=@fV' node */

	ND_SUB_ASSIGN			= IC_MINUSASS,			/**< '-=' node */
	ND_SUB_ASSIGN_R			= IC_MINUSASSR,			/**< '-=f' node */
	ND_SUB_ASSIGN_AT		= IC_MINUSASSAT,		/**< '-=@' node */
	ND_SUB_ASSIGN_AT_R		= IC_MINUSASSATR,		/**< '-=@f' node */
	ND_SUB_ASSIGN_V			= IC_MINUSASSV,			/**< '-=V' node */
	ND_SUB_ASSIGN_R_V		= IC_MINUSASSRV,		/**< '-=fV' node */
	ND_SUB_ASSIGN_AT_V		= IC_MINUSASSATV,		/**< '-=@V' node */
	ND_SUB_ASSIGN_AT_R_V	= IC_MINUSASSATRV,		/**< '-=@fV' node */

	ND_REM_ASSIGN			= IC_REMASS,			/**< '%=' node */
	ND_REM_ASSIGN_AT		= IC_REMASSAT,			/**< '%=@' node */
	ND_REM_ASSIGN_V			= IC_REMASSV,			/**< '%=V' node */
	ND_REM_ASSIGN_AT_V		= IC_REMASSATV,			/**< '%=@V' node */

	ND_SHL_ASSIGN			= IC_SHLASS,			/**< '<<=' node */
	ND_SHL_ASSIGN_AT		= IC_SHLASSAT,			/**< '<<=@' node */
	ND_SHL_ASSIGN_V			= IC_SHLASSV,			/**< '<<=V' node */
	ND_SHL_ASSIGN_AT_V		= IC_SHLASSATV,			/**< '<<=@V' node */

	ND_SHR_ASSIGN			= IC_SHRASS,			/**< '>>=' node */
	ND_SHR_ASSIGN_AT		= IC_SHRASSAT,			/**< '>>=@' node */
	ND_SHR_ASSIGN_V			= IC_SHRASSV,			/**< '>>=V' node */
	ND_SHR_ASSIGN_AT_V		= IC_SHRASSATV,			/**< '>>=@V' node */

	ND_AND_ASSIGN			= IC_ANDASS,			/**< '&=' node */
	ND_AND_ASSIGN_AT		= IC_ANDASSAT,			/**< '&=@' node */
	ND_AND_ASSIGN_V			= IC_ANDASSV,			/**< '&=V' node */
	ND_AND_ASSIGN_AT_V		= IC_ANDASSATV,			/**< '&=@V' node */

	ND_XOR_ASSIGN			= IC_EXORASS,			/**< '^=' node */
	ND_XOR_ASSIGN_AT		= IC_EXORASSAT,			/**< '^=@' node */
	ND_XOR_ASSIGN_V			= IC_EXORASSV,			/**< '^=V' node */
	ND_XOR_ASSIGN_AT_V		= IC_EXORASSATV,		/**< '^=@V' node */

	ND_OR_ASSIGN			= IC_ORASS,				/**< '|=' node */
	ND_OR_ASSIGN_AT			= IC_ORASSAT,			/**< '|=@' node */
	ND_OR_ASSIGN_V			= IC_ORASSV,			/**< '|=V' node */
	ND_OR_ASSIGN_AT_V		= IC_ORASSATV,			/**< '|=@V' node */

	END_FINAL_OPERATION = MAX_INSTRUCTION_CODE,

	// Statement
	ND_LABEL,				/**< Label statement node */
	ND_CASE,				/**< Case statement node */
	ND_DEFAULT,				/**< Default statement node */
	ND_BLOCK,				/**< Compound statement node */
	ND_IF,					/**< If statement node */
	ND_SWITCH,				/**< Switch statement node */
	ND_WHILE,				/**< While statement node */
	ND_DO,					/**< Do statement node */
	ND_FOR,					/**< For statement node */
	ND_GOTO,				/**< Goto statement node */
	ND_CONTINUE,			/**< Continue statement node */
	ND_BREAK,				/**< Break statement node */
	ND_RETURN_VOID,			/**< Void return statement node */
	ND_RETURN_VAL,			/**< Valued return statement node */
	ND_CREATE_DIRECT,		/**< Create direct thread node */
	ND_EXIT_DIRECT,			/**< Exit direct thread node */
	ND_PRINTID,				/**< Printid statement node */
	ND_PRINT,				/**< Print statement node */
	ND_GETID,				/**< Getid statement node */
	ND_PRINTF,				/**< Printf statement node */

	// Declarations
	ND_DECL_ID,				/**< Identifier declaration node */
	ND_DECL_ARR,			/**< Array declaration node */
	ND_DECL_STRUCT,			/**< Struct declaration node */
	ND_FUNC_DEF,			/**< Function definition node */
	ND_ARRAY_INIT,			/**< Array inition node */
	ND_STRUCT_INIT,			/**< Struct inition node */

	// End nodes
	ND_BLOCK_END,			/**< End of block node */
	ND_DECL_STRUCT_END,		/**< End of struct declaration node */
	ND_EXPR_END,			/**< End of expression node */

	// Expressions
	ND_CONDITIONAL,			/**< Ternary operator node */
	ND_IDENT,				/**< Identifier node */
	ND_IDENT_TO_ADDR,		/**< Identifier to address node */
	ND_SLICE,				/**< Slice node */
	ND_SLICE_IDENT,			/**< Slice from identifier node */
	ND_SELECT,				/**< Select node */
	ND_CALL1,				/**< 'Call1' node */
	ND_CALL2,				/**< 'Call2' node */
	ND_STRING,				/**< String literal node */
	ND_IDENT_TO_VAL,		/**< Value of integer variable node */
	ND_ADDR_TO_VAL,			/**< Address to integer value node */
	ND_CONST,				/**< Integer constant node */

	ND_STRING_D			= ND_STRING + DISPL_TO_FLOAT,		/**< Row of doubles node */
	ND_IDENT_TO_VAL_D	= ND_IDENT_TO_VAL + DISPL_TO_FLOAT,	/**< Value of double variable node */
	ND_ADDR_TO_VAL_D	= ND_ADDR_TO_VAL + DISPL_TO_FLOAT,	/**< Address to double value node */
	ND_CONST_D			= ND_CONST + DISPL_TO_FLOAT,		/**< Double constant node node */
} node_t;


/**
 *	Convert token type to corresponding node type
 *
 *	@param	token	Token type
 *
 *	@return	Node type
 */
node_t token_to_node(const token_t token);

/**
 *	Convert operator to corresponding address version
 *
 *	@param	node	Operator
 *
 *	@return	Address version operator
 */
node_t node_to_address_ver(const node_t node);

/**
 *	Convert operator to corresponding void version
 *
 *	@param	node	Operator
 *
 *	@return	Void version operator
 */
node_t node_to_void_ver(const node_t node);

/**
 *	Convert operator to corresponding float version
 *
 *	@param	node	Operator
 *
 *	@return	Float version operator
 */
node_t node_to_float_ver(const node_t node);

/**
 *	Check if node type is assignment operator
 *
 *	@param	node	Node type
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int node_is_assignment(const node_t node);

#ifdef __cplusplus
} /* extern "C" */
#endif
