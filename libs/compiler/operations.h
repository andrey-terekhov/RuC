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

#include <stdbool.h>
#include <stddef.h>
#include "instructions.h"
#include "tokens.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum OPERATION
{
	BEGIN_OP_FINAL			= 8000,

	OP_NOP					= IC_NOP,					/**< Empty node */
	OP_WIDEN1				= IC_WIDEN1,				/**< 'WIDEN1' node */
	OP_WIDEN				= IC_WIDEN,					/**< 'WIDEN' node */
	OP_ROWING				= IC_ROWING,				/**< 'ROWING' node */
	OP_ROWING_D				= IC_ROWING_D,				/**< 'ROWINGD' node */
	OP_UPB					= IC_UPB,					/**< 'UPB' node */

	// Struct copying functions
	OP_COPY00				= IC_COPY00,				/**< 'COPY00' node */
	OP_COPY01				= IC_COPY01,				/**< 'COPY01' node */
	OP_COPY10				= IC_COPY10,				/**< 'COPY10' node */
	OP_COPY11				= IC_COPY11,				/**< 'COPY11' node */
	OP_COPY0ST				= IC_COPY0ST,				/**< 'COPY0ST' node */
	OP_COPY1ST				= IC_COPY1ST,				/**< 'COPY1ST' node */
	OP_COPY0ST_ASSIGN		= IC_COPY0ST_ASSIGN,		/**< 'COPY0ST_ASSIGN' node */
	OP_COPY1ST_ASSIGN		= IC_COPY1ST_ASSIGN,		/**< 'COPY1ST_ASSIGN' node */
	OP_COPYST				= IC_COPYST,				/**< 'COPYST' node */

	// Math functions
	OP_ABSI					= IC_ABSI,					/**< 'ABSI' node */
	OP_ABS					= IC_ABS,					/**< 'ABS' node */

	// Unary operators
	OP_NOT					= IC_NOT,					/**< 'BITNOT' node */
	OP_LOG_NOT				= IC_LOG_NOT,				/**< 'NOT' node */
	OP_UNMINUS				= IC_UNMINUS,				/**< 'UNMINUS' node */
	OP_UNMINUS_R			= IC_UNMINUS_R,				/**< 'UNIMINUSf' node */

	OP_PRE_INC				= IC_PRE_INC,				/**< 'INC' node */
	OP_PRE_INC_R			= IC_PRE_INC_R,				/**< 'INCf' node */
	OP_PRE_INC_AT			= IC_PRE_INC_AT,			/**< 'INC@' node */
	OP_PRE_INC_AT_R			= IC_PRE_INC_AT_R,			/**< 'PREINC@f' node */
	OP_PRE_INC_V			= IC_PRE_INC_V,				/**< 'PREINCV' node */
	OP_PRE_INC_R_V			= IC_PRE_INC_R_V,			/**< 'PREINCfV' node */
	OP_PRE_INC_AT_V			= IC_PRE_INC_AT_V,			/**< 'PREINC@V' node */
	OP_PRE_INC_AT_R_V		= IC_PRE_INC_AT_R_V,		/**< 'PREINC@fV' node */

	OP_PRE_DEC				= IC_PRE_DEC,				/**< 'DEC' node */
	OP_PRE_DEC_R			= IC_PRE_DEC_R,				/**< 'DECf' node */
	OP_PRE_DEC_AT			= IC_PRE_DEC_AT,			/**< 'DEC@' node */
	OP_PRE_DEC_AT_R			= IC_PRE_DEC_AT_R,			/**< 'PREDEC@f' node */
	OP_PRE_DEC_V			= IC_PRE_DEC_V,				/**< 'PREDECV' node */
	OP_PRE_DEC_R_V			= IC_PRE_DEC_R_V,			/**< 'PREDECfV' node */
	OP_PRE_DEC_AT_V			= IC_PRE_DEC_AT_V,			/**< 'PREDEC@V' node */
	OP_PRE_DEC_AT_R_V		= IC_PRE_DEC_AT_R_V,		/**< 'PREDEC@fV' node */

	OP_POST_INC				= IC_POST_INC,				/**< 'POSTINC' node */
	OP_POST_INC_R			= IC_POST_INC_R,			/**< 'POSTINCf' node */
	OP_POST_INC_AT			= IC_POST_INC_AT,			/**< 'POSTINC@' node */
	OP_POST_INC_AT_R		= IC_POST_INC_AT_R,			/**< 'POSTINC@f' node */
	OP_POST_INC_V			= IC_POST_INC_V,			/**< 'POSTINCV' node */
	OP_POST_INC_R_V			= IC_POST_INC_R_V,			/**< 'POSTINCfV' node */
	OP_POST_INC_AT_V		= IC_POST_INC_AT_V,			/**< 'POSTINC@V' node */
	OP_POST_INC_AT_R_V		= IC_POST_INC_AT_R_V,		/**< 'POSTINC@fV' node */

	OP_POST_DEC				= IC_POST_DEC,				/**< 'POSTDEC' node */
	OP_POST_DEC_R			= IC_POST_DEC_R,			/**< 'POSTDECf' node */
	OP_POST_DEC_AT			= IC_POST_DEC_AT,			/**< 'POSTDEC@' node */
	OP_POST_DEC_AT_R		= IC_POST_DEC_AT_R,			/**< 'POSTDEC@f' node */
	OP_POST_DEC_V			= IC_POST_DEC_V,			/**< 'POSTDECV' node */
	OP_POST_DEC_R_V			= IC_POST_DEC_R_V,			/**< 'POSTDECfV' node */
	OP_POST_DEC_AT_V		= IC_POST_DEC_AT_V,			/**< 'POSTDEC@V' node */
	OP_POST_DEC_AT_R_V		= IC_POST_DEC_AT_R_V,		/**< 'POSTDEC@fV' node */

	// Binary operators
	OP_MUL					= IC_MUL,					/**< '*' node */
	OP_MUL_R				= IC_MUL_R,					/**< '*f' node */

	OP_DIV					= IC_DIV,					/**< '/' node */
	OP_DIV_R				= IC_DIV_R,					/**< '/f' node */

	OP_ADD					= IC_ADD,					/**< '+' node */
	OP_ADD_R				= IC_ADD_R,					/**< '+f' node */

	OP_SUB					= IC_SUB,					/**< '-' node */
	OP_SUB_R				= IC_SUB_R,					/**< '-f' node */

	OP_LT					= IC_LT,					/**< '<' node */
	OP_LT_R					= IC_LT_R,					/**< '<f' node */

	OP_GT					= IC_GT,					/**< '>' node */
	OP_GT_R					= IC_GT_R,					/**< '>f' node */

	OP_LE					= IC_LE,					/**< '<=' node */
	OP_LE_R					= IC_LE_R,					/**< '<=f' node */

	OP_GE					= IC_GE,					/**< '>=' node */
	OP_GE_R					= IC_GE_R,					/**< '>=f' node */

	OP_EQ					= IC_EQ,					/**< '==' node */
	OP_EQ_R					= IC_EQ_R,					/**< '==f' node */

	OP_NE					= IC_NE,					/**< '!=' node */
	OP_NE_R					= IC_NE_R,					/**< '!=f' node */

	OP_SHL					= IC_SHL,					/**< '<<' node */
	OP_SHR					= IC_SHR,					/**< '>>' node */
	OP_REM					= IC_REM,					/**< '%' node */
	OP_AND					= IC_AND,					/**< '&' node */
	OP_XOR					= IC_XOR,					/**< '^' node */
	OP_OR					= IC_OR,					/**< '|' node */
	OP_LOG_AND				= IC_LOG_AND,				/**< '&&' node */
	OP_LOG_OR				= IC_LOG_OR,				/**< '||' node */

	OP_ASSIGN				= IC_ASSIGN,				/**< '=' node */
	OP_ASSIGN_R				= IC_ASSIGN_R,				/**< '=f' node */
	OP_ASSIGN_AT			= IC_ASSIGN_AT,				/**< '=@' node */
	OP_ASSIGN_AT_R			= IC_ASSIGN_AT_R,			/**< '=@f' node */
	OP_ASSIGN_V				= IC_ASSIGN_V,				/**< '=V' node */
	OP_ASSIGN_R_V			= IC_ASSIGN_R_V,			/**< '=fV' node */
	OP_ASSIGN_AT_V			= IC_ASSIGN_AT_V,			/**< '=@V' node */
	OP_ASSIGN_AT_R_V		= IC_ASSIGN_AT_R_V,			/**< '=@fV' node */

	OP_MUL_ASSIGN			= IC_MUL_ASSIGN,			/**< '*=' node */
	OP_MUL_ASSIGN_R			= IC_MUL_ASSIGN_R,			/**< '*=f' node */
	OP_MUL_ASSIGN_AT		= IC_MUL_ASSIGN_AT,			/**< '*=@' node */
	OP_MUL_ASSIGN_AT_R		= IC_MUL_ASSIGN_AT_R,		/**< '*=@f' node */
	OP_MUL_ASSIGN_V			= IC_MUL_ASSIGN_V,			/**< '*=V' node */
	OP_MUL_ASSIGN_R_V		= IC_MUL_ASSIGN_R_V,		/**< '*=fV' node */
	OP_MUL_ASSIGN_AT_V		= IC_MUL_ASSIGN_AT_V,		/**< '*=@V' node */
	OP_MUL_ASSIGN_AT_R_V	= IC_MUL_ASSIGN_AT_R_V,		/**< '*=@fV' node */

	OP_DIV_ASSIGN			= IC_DIV_ASSIGN,			/**< '/=' node */
	OP_DIV_ASSIGN_R			= IC_DIV_ASSIGN_R,			/**< '/=f' node */
	OP_DIV_ASSIGN_AT		= IC_DIV_ASSIGN_AT,			/**< '/=@' node */
	OP_DIV_ASSIGN_AT_R		= IC_DIV_ASSIGN_AT_R,		/**< '/=@f' node */
	OP_DIV_ASSIGN_V			= IC_DIV_ASSIGN_V,			/**< '/=V' node */
	OP_DIV_ASSIGN_R_V		= IC_DIV_ASSIGN_R_V,		/**< '/=fV' node */
	OP_DIV_ASSIGN_AT_V		= IC_DIV_ASSIGN_AT_V,		/**< '/=@V' node */
	OP_DIV_ASSIGN_AT_R_V	= IC_DIV_ASSIGN_AT_R_V,		/**< '/=@fV' node */

	OP_ADD_ASSIGN			= IC_ADD_ASSIGN,			/**< '+=' node */
	OP_ADD_ASSIGN_R			= IC_ADD_ASSIGN_R,			/**< '+=f' node */
	OP_ADD_ASSIGN_AT		= IC_ADD_ASSIGN_AT,			/**< '+=@' node */
	OP_ADD_ASSIGN_AT_R		= IC_ADD_ASSIGN_AT_R,		/**< '+=@f' node */
	OP_ADD_ASSIGN_V			= IC_ADD_ASSIGN_V,			/**< '+=V' node */
	OP_ADD_ASSIGN_R_V		= IC_ADD_ASSIGN_R_V,		/**< '+=fV' node */
	OP_ADD_ASSIGN_AT_V		= IC_ADD_ASSIGN_AT_V,		/**< '+=@V' node */
	OP_ADD_ASSIGN_AT_R_V	= IC_ADD_ASSIGN_AT_R_V,		/**< '+=@fV' node */

	OP_SUB_ASSIGN			= IC_SUB_ASSIGN,			/**< '-=' node */
	OP_SUB_ASSIGN_R			= IC_SUB_ASSIGN_R,			/**< '-=f' node */
	OP_SUB_ASSIGN_AT		= IC_SUB_ASSIGN_AT,			/**< '-=@' node */
	OP_SUB_ASSIGN_AT_R		= IC_SUB_ASSIGN_AT_R,		/**< '-=@f' node */
	OP_SUB_ASSIGN_V			= IC_SUB_ASSIGN_V,			/**< '-=V' node */
	OP_SUB_ASSIGN_R_V		= IC_SUB_ASSIGN_R_V,		/**< '-=fV' node */
	OP_SUB_ASSIGN_AT_V		= IC_SUB_ASSIGN_AT_V,		/**< '-=@V' node */
	OP_SUB_ASSIGN_AT_R_V	= IC_SUB_ASSIGN_AT_R_V,		/**< '-=@fV' node */

	OP_REM_ASSIGN			= IC_REM_ASSIGN,			/**< '%=' node */
	OP_REM_ASSIGN_AT		= IC_REM_ASSIGN_AT,			/**< '%=@' node */
	OP_REM_ASSIGN_V			= IC_REM_ASSIGN_V,			/**< '%=V' node */
	OP_REM_ASSIGN_AT_V		= IC_REM_ASSIGN_AT_V,		/**< '%=@V' node */

	OP_SHL_ASSIGN			= IC_SHL_ASSIGN,			/**< '<<=' node */
	OP_SHL_ASSIGN_AT		= IC_SHL_ASSIGN_AT,			/**< '<<=@' node */
	OP_SHL_ASSIGN_V			= IC_SHL_ASSIGN_V,			/**< '<<=V' node */
	OP_SHL_ASSIGN_AT_V		= IC_SHL_ASSIGN_AT_V,		/**< '<<=@V' node */

	OP_SHR_ASSIGN			= IC_SHR_ASSIGN,			/**< '>>=' node */
	OP_SHR_ASSIGN_AT		= IC_SHR_ASSIGN_AT,			/**< '>>=@' node */
	OP_SHR_ASSIGN_V			= IC_SHR_ASSIGN_V,			/**< '>>=V' node */
	OP_SHR_ASSIGN_AT_V		= IC_SHR_ASSIGN_AT_V,		/**< '>>=@V' node */

	OP_AND_ASSIGN			= IC_AND_ASSIGN,			/**< '&=' node */
	OP_AND_ASSIGN_AT		= IC_AND_ASSIGN_AT,			/**< '&=@' node */
	OP_AND_ASSIGN_V			= IC_AND_ASSIGN_V,			/**< '&=V' node */
	OP_AND_ASSIGN_AT_V		= IC_AND_ASSIGN_AT_V,		/**< '&=@V' node */

	OP_XOR_ASSIGN			= IC_XOR_ASSIGN,			/**< '^=' node */
	OP_XOR_ASSIGN_AT		= IC_XOR_ASSIGN_AT,			/**< '^=@' node */
	OP_XOR_ASSIGN_V			= IC_XOR_ASSIGN_V,			/**< '^=V' node */
	OP_XOR_ASSIGN_AT_V		= IC_XOR_ASSIGN_AT_V,		/**< '^=@V' node */

	OP_OR_ASSIGN			= IC_OR_ASSIGN,				/**< '|=' node */
	OP_OR_ASSIGN_AT			= IC_OR_ASSIGN_AT,			/**< '|=@' node */
	OP_OR_ASSIGN_V			= IC_OR_ASSIGN_V,			/**< '|=V' node */
	OP_OR_ASSIGN_AT_V		= IC_OR_ASSIGN_AT_V,		/**< '|=@V' node */

	OP_AD_LOG_AND			= MAX_INSTRUCTION_CODE,		/**< 'ADLOGAND' node */
	OP_AD_LOG_OR,										/**< 'ADLOGOR' node */

	END_OP_FINAL,

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

	OP_FREAD,				/**< 'fread' node */
} operation_t;


typedef enum builtin
{
	// Diagnostics functions
	BI_ASSERT				= 2,

	// Math functions
	BI_ASIN					= 6,
	BI_COS					= 10,
	BI_SIN					= 14,
	BI_EXP					= 18,
	BI_LOG					= 22,
	BI_LOG10				= 26,
	BI_SQRT					= 30,
	BI_RAND					= 34,
	BI_ROUND				= 38,

	// String functions
	BI_STRCPY				= 42,
	BI_STRNCPY				= 46,
	BI_STRCAT				= 50,
	BI_STRNCAT				= 54,
	BI_STRCMP				= 58,
	BI_STRNCMP				= 62,
	BI_STRSTR				= 66,
	BI_STRLEN				= 70,

	// Robot functions
	BI_ROBOT_SEND_INT		= 74,
	BI_ROBOT_SEND_FLOAT		= 78,
	BI_ROBOT_SEND_STRING	= 82,
	BI_ROBOT_RECEIVE_INT	= 86,
	BI_ROBOT_RECEIVE_FLOAT	= 90,
	BI_ROBOT_RECEIVE_STRING	= 94,

	// Thread functions
	BI_CREATE				= 98,
	BI_GETNUM				= 102,
	BI_SLEEP				= 106,
	BI_JOIN					= 110,
	BI_EXIT					= 114,
	BI_INIT					= 118,
	BI_DESTROY				= 122,

	BI_SEM_CREATE			= 126,
	BI_SEM_WAIT				= 130,
	BI_SEM_POST				= 134,

	BI_MSG_SEND				= 138,
	BI_MSG_RECEIVE			= 142,

	BI_FOPEN				= 146,
	BI_FGETC				= 150,
	BI_FPUTC				= 154,
	BI_FCLOSE				= 158,

	BEGIN_USER_FUNC			= 162,
} builtin_t;


/**
 *	Convert token to corresponding binary operation
 *
 *	@param	token		Token
 *
 *	@return	Binary operation
 */
operation_t token_to_binary(const token_t token);

/**
 *	Convert token to corresponding unary operation
 *
 *	@param	token		Token
 *
 *	@return	Unary operation
 */
operation_t token_to_unary(const token_t token);

/**
 *	Convert standard function id to corresponding function instruction
 *
 *	@param	func		Function id
 *
 *	@return	Function instruction
 */
instruction_t builtin_to_instruction(const builtin_t func);

/**
 *	Convert to corresponding address version of operation
 *
 *	@param	operation	Operation
 *
 *	@return	Address version of operation
 */
operation_t operation_to_address_ver(const operation_t operation);

/**
 *	Convert to corresponding void version of operation
 *
 *	@param	operation	Operation
 *
 *	@return	Void version of operation
 */
operation_t operation_to_void_ver(const operation_t operation);

/**
 *	Convert to corresponding float version of operation
 *
 *	@param	operation	Operation
 *
 *	@return	Float version of operation
 */
operation_t operation_to_float_ver(const operation_t operation);

/**
 *	Check if operation type is assignment
 *
 *	@param	operation	Operation
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool operation_is_assignment(const operation_t operation);

#ifdef __cplusplus
} /* extern "C" */
#endif
