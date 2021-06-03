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
#include "instructions.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum NODE
{
	BEGIN_FINAL_OPERATION,

	ND_ADLOGAND = 7000,			/**< "ADLOGAND" node */
	ND_ADLOGOR,				/**< "ADLOGOR" node */
	ND_NULL				= IC_NOP,				/**< Empty node */
	ND_WIDEN1			= IC_WIDEN1,			/**< "WIDEN1" node */
	ND_WIDEN			= IC_WIDEN,				/**< "WIDEN" node */
	ND_STRINGINIT		= IC_STRINGINIT,		/**< "STRINGINIT" node */
	ND_ROWING			= IC_ROWING,			/**< "ROWING" node */
	ND_ROWINGD			= IC_ROWINGD,			/**< "ROWINGD" node */
	ND_ASSERT			= IC_ASSERT,			/**< "ASSERT" node */
	ND_UPB				= IC_UPB,				/**< "UPB" node */

	// Struct copying functions
	ND_COPY00			= IC_COPY00,			/**< "COPY00" node */
	ND_COPY01			= IC_COPY01,			/**< "COPY01" node */
	ND_COPY10			= IC_COPY10,			/**< "COPY10" node */
	ND_COPY11			= IC_COPY11,			/**< "COPY11" node */
	ND_COPY0ST			= IC_COPY0ST,			/**< "COPY0ST" node */
	ND_COPY1ST			= IC_COPY1ST,			/**< "COPY1ST" node */
	ND_COPY0STASSIGN	= IC_COPY0STASS,		/**< "COPY0STASSIGN" node */
	ND_COPY1STASSIGN	= IC_COPY1STASS,		/**< "COPY1STASSIGN" node */
	ND_COPYST			= IC_COPYST,			/**< "COPYST" node */

	// Math functions
	ND_ABSI				= IC_ABSI,				/**< "ABSI" node */
	ND_ABS				= IC_ABS,				/**< "ABS" node */
	ND_SQRT				= IC_SQRT,				/**< "SQRT" node */
	ND_EXP				= IC_EXP,				/**< "EXP" node */
	ND_SIN				= IC_SIN,				/**< "SIN" node */
	ND_COS				= IC_COS,				/**< "COS" node */
	ND_LOG				= IC_LOG,				/**< "LOG" node */
	ND_LOG10			= IC_LOG10,				/**< "LOG10" node */
	ND_ASIN				= IC_ASIN,				/**< "ASIN" node */
	ND_RAND				= IC_RAND,				/**< "RAND" node */
	ND_ROUND			= IC_ROUND,				/**< "ROUND" node */

	// String functions
	ND_STRCPY			= IC_STRCPY,			/**< "STRCPY" node */
	ND_STRNCPY			= IC_STRNCPY,			/**< "STRNCPY" node */
	ND_STRCAT			= IC_STRCAT,			/**< "STRCAT" node */
	ND_STRNCAT			= IC_STRNCAT,			/**< "STRNCAT" node */
	ND_STRCMP			= IC_STRCMP,			/**< "STRCMP" node */
	ND_STRNCMP			= IC_STRNCMP,			/**< "STRNCMP" node */
	ND_STRSTR			= IC_STRSTR,			/**< "STRSTR" node */
	ND_STRLEN			= IC_STRLEN,			/**< "STRLEN" node */

	// Thread functions
	ND_MSG_SEND			= IC_MSG_SEND,			/**< "MSG_SEND" node */
	ND_MSG_RECEIVE		= IC_MSG_RECEIVE,		/**< "MSG_RECEIVE" node */
	ND_JOIN				= IC_JOIN,				/**< "JOIN" node */
	ND_SLEEP			= IC_SLEEP,				/**< "SLEEP" node */
	ND_SEMCREATE		= IC_SEMCREATE,			/**< "SEMCREATE" node */
	ND_SEMWAIT			= IC_SEMWAIT,			/**< "SEMWAIT" node */
	ND_SEMPOST			= IC_SEMPOST,			/**< "SEMPOST" node */
	ND_CREATE			= IC_CREATE,			/**< "CREATE" node */
	ND_INIT				= IC_INIT,				/**< "INIT" node */
	ND_DESTROY			= IC_DESTROY,			/**< "DESTROY" node */
	ND_EXIT				= IC_EXIT,				/**< "EXIT" node */
	ND_GETNUM			= IC_GETNUM,			/**< "GETNUM" node */

	// Robot functions
	ND_SEND_INT			= IC_SEND_INT,			/**< "SEND_INT" node */
	ND_SEND_FLOAT		= IC_SEND_FLOAT,		/**< "SEND_FLOAT" node */
	ND_SEND_STRING		= IC_SEND_STRING,		/**< "SEND_STRING" node */
	ND_RECEIVE_INT		= IC_RECEIVE_INT,		/**< "RECEIVE_INT" node */
	ND_RECEIVE_FLOAT	= IC_RECEIVE_FLOAT,		/**< "RECEIVE_FLOAT" node */
	ND_RECEIVE_STRING	= IC_RECEIVE_STRING,	/**< "RECEIVE_STRING" node */

	// Unary operators
	ND_NOT				= IC_NOT,				/**< "BITNOT" node */
	ND_LOGNOT			= IC_LOGNOT,			/**< "NOT" node */
	ND_UNMINUS			= IC_UNMINUS,			/**< "UNMINUS" node */
	ND_UNMINUSR			= IC_UNMINUSR,			/**< "UNIMINUSf" node */

	ND_PREINC			= IC_INC,				/**< "INC" node */
	ND_PREINCR			= IC_INCR,				/**< "INCf" node */
	ND_PREINCAT			= IC_INCAT,				/**< "INC@" node */
	ND_PREINCATR		= IC_INCATR,			/**< "PREINC@f" node */
	ND_PREINCV			= IC_INCV,				/**< "PREINCV" node */
	ND_PREINCRV			= IC_INCRV,				/**< "PREINCfV" node */
	ND_PREINCATV		= IC_INCATV,			/**< "PREINC@V" node */
	ND_PREINCATRV		= IC_INCATRV,			/**< "PREINC@fV" node */

	ND_PREDEC			= IC_DEC,				/**< "DEC" node */
	ND_PREDECR			= IC_DECR,				/**< "DECf" node */
	ND_PREDECAT			= IC_DECAT,				/**< "DEC@" node */
	ND_PREDECATR		= IC_DECATR,			/**< "PREDEC@f" node */
	ND_PREDECV			= IC_DECV,				/**< "PREDECV" node */
	ND_PREDECRV			= IC_DECRV,				/**< "PREDECfV" node */
	ND_PREDECATV		= IC_DECATV,			/**< "PREDEC@V" node */
	ND_PREDECATRV		= IC_DECATRV,			/**< "PREDEC@fV" node */

	ND_POSTINC			= IC_POSTINC,			/**< "POSTINC" node */
	ND_POSTINCR			= IC_POSTINCR,			/**< "POSTINCf" node */
	ND_POSTINCAT		= IC_POSTINCAT,			/**< "POSTINC@" node */
	ND_POSTINCATR		= IC_POSTINCATR,		/**< "POSTINC@f" node */
	ND_POSTINCV			= IC_POSTINCV,			/**< "POSTINCV" node */
	ND_POSTINCRV		= IC_POSTINCRV,			/**< "POSTINCfV" node */
	ND_POSTINCATV		= IC_POSTINCATV,		/**< "POSTINC@V" node */
	ND_POSTINCATRV		= IC_POSTINCATRV,		/**< "POSTINC@fV" node */

	ND_POSTDEC			= IC_POSTDEC,			/**< "POSTDEC" node */
	ND_POSTDECR			= IC_POSTDECR,			/**< "POSTDECf" node */
	ND_POSTDECAT		= IC_POSTDECAT,			/**< "POSTDEC@" node */
	ND_POSTDECATR		= IC_POSTDECATR,		/**< "POSTDEC@f" node */
	ND_POSTDECV			= IC_POSTDECV,			/**< "POSTDECV" node */
	ND_POSTDECRV		= IC_POSTDECRV,			/**< "POSTDECfV" node */
	ND_POSTDECATV		= IC_POSTDECATV,		/**< "POSTDEC@V" node */
	ND_POSTDECATRV		= IC_POSTDECATRV,		/**< "POSTDEC@fV" node */

	// Binary operators
	ND_MUL				= IC_MULT,				/**< "*" node */
	ND_MULR				= IC_MULTR,				/**< "*f" node */

	ND_DIV				= IC_DIV,				/**< "/" node */
	ND_DIVR				= IC_DIVR,				/**< "/f" node */

	ND_ADD				= IC_PLUS,				/**< "+" node */
	ND_ADDR				= IC_PLUSR,				/**< "+f" node */

	ND_SUB				= IC_MINUS,				/**< "-" node */
	ND_SUBR				= IC_MINUSR,			/**< "-f" node */

	ND_LT				= IC_LT,				/**< "<" node */
	ND_LTR				= IC_LTR,				/**< "<f" node */

	ND_GT				= IC_GT,				/**< ">" node */
	ND_GTR				= IC_GTR,				/**< ">f" node */

	ND_LE				= IC_LE,				/**< "<=" node */
	ND_LER				= IC_LER,				/**< "<=f" node */

	ND_GE				= IC_GE,				/**< ">=" node */
	ND_GER				= IC_GER,				/**< ">=f" node */

	ND_EQ				= IC_EQEQ,				/**< "==" node */
	ND_EQR				= IC_EQEQR,				/**< "==f" node */

	ND_NE				= IC_NOTEQ,				/**< "!=" node */
	ND_NER				= IC_NOTEQR,			/**< "!=f" node */

	ND_SHL				= IC_SHL,				/**< "<<" node */
	ND_SHR				= IC_SHR,				/**< ">>" node */
	ND_REM				= IC_REM,				/**< "%" node */
	ND_AND				= IC_AND,				/**< "&" node */
	ND_XOR				= IC_EXOR,				/**< "^" node */
	ND_OR				= IC_OR,				/**< "|" node */
	ND_LOGAND			= IC_LOGAND,			/**< "&&" node */
	ND_LOGOR			= IC_LOGOR,				/**< "||" node */

	ND_ASSIGN			= IC_ASS,				/**< "=" node */
	ND_ASSIGNR			= IC_ASSR,				/**< "=f" node */
	ND_ASSIGNAT			= IC_ASSAT,				/**< "=@" node */
	ND_ASSIGNATR		= IC_ASSATR,			/**< "=@f" node */
	ND_ASSIGNV			= IC_ASSV,				/**< "=V" node */
	ND_ASSIGNRV			= IC_ASSRV,				/**< "=fV" node */
	ND_ASSIGNATV		= IC_ASSATV,			/**< "=@V" node */
	ND_ASSIGNATRV		= IC_ASSATRV,			/**< "=@fV" node */

	ND_MULASSIGN		= IC_MULTASS,			/**< "*=" node */
	ND_MULASSIGNR		= IC_MULTASSR,			/**< "*=f" node */
	ND_MULASSIGNAT		= IC_MULTASSAT,			/**< "*=@" node */
	ND_MULASSIGNATR		= IC_MULTASSATR,		/**< "*=@f" node */
	ND_MULASSIGNV		= IC_MULTASSV,			/**< "*=V" node */
	ND_MULASSIGNRV		= IC_MULTASSRV,			/**< "*=fV" node */
	ND_MULASSIGNATV		= IC_MULTASSATV,		/**< "*=@V" node */
	ND_MULASSIGNATRV	= IC_MULTASSATRV,		/**< "*=@fV" node */

	ND_DIVASSIGN		= IC_DIVASS,			/**< "/=" node */
	ND_DIVASSIGNR		= IC_DIVASSR,			/**< "/=f" node */
	ND_DIVASSIGNAT		= IC_DIVASSAT,			/**< "/=@" node */
	ND_DIVASSIGNATR		= IC_DIVASSATR,			/**< "/=@f" node */
	ND_DIVASSIGNV		= IC_DIVASSV,			/**< "/=V" node */
	ND_DIVASSIGNRV		= IC_DIVASSRV,			/**< "/=fV" node */
	ND_DIVASSIGNATV		= IC_DIVASSATV,			/**< "/=@V" node */
	ND_DIVASSIGNATRV	= IC_DIVASSATRV,		/**< "/=@fV" node */

	ND_ADDASSIGN		= IC_PLUSASS,			/**< "+=" node */
	ND_ADDASSIGNR		= IC_PLUSASSR,			/**< "+=f" node */
	ND_ADDASSIGNAT		= IC_PLUSASSAT,			/**< "+=@" node */
	ND_ADDASSIGNATR		= IC_PLUSASSATR,		/**< "+=@f" node */
	ND_ADDASSIGNV		= IC_PLUSASSV,			/**< "+=V" node */
	ND_ADDASSIGNRV		= IC_PLUSASSRV,			/**< "+=fV" node */
	ND_ADDASSIGNATV		= IC_PLUSASSATV,		/**< "+=@V" node */
	ND_ADDASSIGNATRV	= IC_PLUSASSATRV,		/**< "+=@fV" node */

	ND_SUBASSIGN		= IC_MINUSASS,			/**< "-=" node */
	ND_SUBASSIGNR		= IC_MINUSASSR,			/**< "-=f" node */
	ND_SUBASSIGNAT		= IC_MINUSASSAT,		/**< "-=@" node */
	ND_SUBASSIGNATR		= IC_MINUSASSATR,		/**< "-=@f" node */
	ND_SUBASSIGNV		= IC_MINUSASSV,			/**< "-=V" node */
	ND_SUBASSIGNRV		= IC_MINUSASSRV,		/**< "-=fV" node */
	ND_SUBASSIGNATV		= IC_MINUSASSATV,		/**< "-=@V" node */
	ND_SUBASSIGNATRV	= IC_MINUSASSATRV,		/**< "-=@fV" node */

	ND_REMASSIGN		= IC_REMASS,			/**< "%=" node */
	ND_REMASSIGNAT		= IC_REMASSAT,			/**< "%=@" node */
	ND_REMASSIGNV		= IC_REMASSV,			/**< "%=V" node */
	ND_REMASSIGNATV		= IC_REMASSATV,			/**< "%=@V" node */

	ND_SHLASSIGN		= IC_SHLASS,			/**< "<<=" node */
	ND_SHLASSIGNAT		= IC_SHLASSAT,			/**< "<<=@" node */
	ND_SHLASSIGNV		= IC_SHLASSV,			/**< "<<=V" node */
	ND_SHLASSIGNATV		= IC_SHLASSATV,			/**< "<<=@V" node */

	ND_SHRASSIGN		= IC_SHRASS,			/**< ">>=" node */
	ND_SHRASSIGNAT		= IC_SHRASSAT,			/**< ">>=@" node */
	ND_SHRASSIGNV		= IC_SHRASSV,			/**< ">>=V" node */
	ND_SHRASSIGNATV		= IC_SHRASSATV,			/**< ">>=@V" node */

	ND_ANDASSIGN		= IC_ANDASS,			/**< "&=" node */
	ND_ANDASSIGNAT		= IC_ANDASSAT,			/**< "&=@" node */
	ND_ANDASSIGNV		= IC_ANDASSV,			/**< "&=V" node */
	ND_ANDASSIGNATV		= IC_ANDASSATV,			/**< "&=@V" node */

	ND_XORASSIGN		= IC_EXORASS,			/**< "^=" node */
	ND_XORASSIGNAT		= IC_EXORASSAT,			/**< "^=@" node */
	ND_XORASSIGNV		= IC_EXORASSV,			/**< "^=V" node */
	ND_XORASSIGNATV		= IC_EXORASSATV,		/**< "^=@V" node */

	ND_ORASSIGN			= IC_ORASS,				/**< "|=" node */
	ND_ORASSIGNAT		= IC_ORASSAT,			/**< "|=@" node */
	ND_ORASSIGNV		= IC_ORASSV,			/**< "|=V" node */
	ND_ORASSIGNATV		= IC_ORASSATV,			/**< "|=@V" node */

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
	ND_CREATEDIRECT,		/**< Create direct statement node */
	ND_EXITDIRECT,			/**< Exit direct statement node */
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
	ND_EXPRESSION_END,		/**< End of expression node */

	// Expressions
	ND_CONDITIONAL,			/**< Ternary operator node */
	ND_STRING,				/**< String literal node */
	ND_STRINGD,				/**< Row of doubles node */
	ND_IDENT,				/**< Identifier node */
	ND_IDENTTOADDR,			/**< Identifier to address node */
	ND_IDENTTOVAL,			/**< Value of integer variable node */
	ND_IDENTTOVALD,			/**< Value of double variable node */
	ND_ADDRTOVAL,			/**< Address to integer value node */
	ND_ADDRTOVALD,			/**< Address to double value node */
	ND_CONST,				/**< Integer constant node */
	ND_CONSTD,				/**< Double constant node node */
	ND_SLICE,				/**< Slice node */
	ND_SLICEIDENT,			/**< Slice from identifier node */
	ND_SELECT,				/**< Select node */
	ND_CALL1,				/**< "Call1" node */
	ND_CALL2,				/**< "Call2" node */
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
node_t node_at_operator(const node_t node);

/**
 *	Convert operator to corresponding void version
 *
 *	@param	node	Operator
 *
 *	@return	Void version operator
 */
node_t node_void_operator(const node_t node);

/**
 *	Convert operator to corresponding float version
 *
 *	@param	node	Operator
 *
 *	@return	Float version operator
 */
node_t node_float_operator(const node_t node);

/**
 *	Check if node type is assignment operator
 *
 *	@param	node	Node type
 *
 *	@return	@c 1 on assignment operator
 */
int node_is_assignment_operator(const node_t node);

#ifdef __cplusplus
} /* extern "C" */
#endif
