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
	// Statement
	ND_LABEL = 7000,	/**< Label statement node */
	ND_CASE,			/**< Case statement node */
	ND_DEFAULT,			/**< Default statement node */
	ND_BLOCK,			/**< Compound statement node */
	ND_IF,				/**< If statement node */
	ND_SWITCH,			/**< Switch statement node */
	ND_WHILE,			/**< While statement node */
	ND_DO,				/**< Do statement node */
	ND_FOR,				/**< For statement node */
	ND_GOTO,			/**< Goto statement node */
	ND_CONTINUE,		/**< Continue statement node */
	ND_BREAK,			/**< Break statement node */
	ND_RETURN_VOID,		/**< Void return statement node */
	ND_RETURN_VAL,		/**< Valued return statement node */
	ND_CREATEDIRECT,	/**< Create direct statement node */
	ND_EXITDIRECT,		/**< Exit direct statement node */
	ND_PRINTID,			/**< Printid statement node */
	ND_PRINT,			/**< Print statement node */
	ND_GETID,			/**< Getid statement node */
	ND_PRINTF,			/**< Printf statement node */

	// Declarations
	ND_DECL_ID,			/**< Identifier declaration node */
	ND_DECL_ARR,		/**< Array declaration node */
	ND_DECL_STRUCT,		/**< Struct declaration node */
	ND_FUNC_DEF,		/**< Function definition node */
	ND_ARRAY_INIT,		/**< Array inition node */
	ND_STRUCT_INIT,		/**< Struct inition node */

	// End nodes
	ND_BLOCK_END,		/**< End of block node */
	ND_DECL_STRUCT_END,	/**< End of struct declaration node */
	ND_EXPRESSION_END,	/**< End of expression node */

	// Expressions
	ND_CONDITIONAL,		/**< Ternary operator node */
	ND_STRING,			/**< String literal node */
	ND_STRINGD,			/**< Row of doubles node */
	ND_IDENT,			/**< Identifier node */
	ND_IDENTTOADDR,		/**< Identifier to address node */
	ND_IDENTTOVAL,		/**< Value of integer variable node */
	ND_IDENTTOVALD,		/**< Value of double variable node */
	ND_ADDRTOVAL,		/**< Address to integer value node */
	ND_ADDRTOVALD,		/**< Address to double value node */
	ND_CONST,			/**< Integer constant node */
	ND_CONSTD,			/**< Double constant node node */
	ND_SLICE,			/**< Slice node */
	ND_SLICEIDENT,		/**< Slice from identifier node */
	ND_SELECT,			/**< Select node */
	ND_CALL1,			/**< "Call1" node */
	ND_CALL2,			/**< "Call2" node */

	// Operators
	ND_REMASS = 8001,	/**< "%=" node */
	ND_SHLASS,			/**< "<<=" node */
	ND_SHRASS,			/**< ">>=" node */
	ND_ANDASS,			/**< "&=" node */
	ND_EXORASS,			/**< "^=" node */
	ND_ORASS,			/**< "|=" node */
	ND_ASS,				/**< "=" node */
	ND_PLUSASS,			/**< "+=" node */
	ND_MINUSASS,		/**< "-=" node */
	ND_MULTASS,			/**< "*=" node */
	ND_DIVASS,			/**< "/=" node */
	ND_REMASSAT,		/**< "%=@" node */
	ND_SHLASSAT,		/**< "<<=@" node */
	ND_SHRASSAT,		/**< ">>=@" node */
	ND_ANDASSAT,		/**< "&=@" node */
	ND_EXORASSAT,		/**< "^=@" node */
	ND_ORASSAT,			/**< "|=@" node */
	ND_ASSAT,			/**< "=@" node */
	ND_PLUSASSAT,		/**< "+=@" node */
	ND_MINUSASSAT,		/**< "-=@" node */
	ND_MULTASSAT,		/**< "*=@" node */
	ND_DIVASSAT,		/**< "/=@" node */
	ND_REM,				/**< "%" node */
	ND_SHL,				/**< "<<" node */
	ND_SHR,				/**< ">>" node */
	ND_AND,				/**< "&" node */
	ND_EXOR,			/**< "^" node */
	ND_OR,				/**< "|" node */
	ND_LOGAND,			/**< "&&" node */
	ND_LOGOR,			/**< "||" node */
	ND_EQEQ,			/**< "==" node */
	ND_NOTEQ,			/**< "!=" node */
	ND_LT,				/**< "<" node */
	ND_GT,				/**< ">" node */
	ND_LE,				/**< "<=" node */
	ND_GE,				/**< ">=" node */
	ND_PLUS,			/**< "+" node */
	ND_MINUS,			/**< "-" node */
	ND_MULT,			/**< "*" node */
	ND_DIV,				/**< "/" node */
	ND_POSTINC,			/**< "POSTINC" node */
	ND_POSTDEC,			/**< "POSTDEC" node */
	ND_INC,				/**< "INC" node */
	ND_DEC,				/**< "DEC" node */
	ND_POSTINCAT,		/**< "POSTINC@" node */
	ND_POSTDECAT,		/**< "POSTDEC@" node */
	ND_INCAT,			/**< "INC@" node */
	ND_DECAT,			/**< "DEC@" node */
	ND_UNMINUS,			/**< "UNMINUS" node */
	ND_NOT = 8052,		/**< "BITNOT" node */
	ND_LOGNOT,			/**< "NOT" node */

	ND_ASSR = 8057,		/**< "=f" node */
	ND_PLUSASSR,		/**< "+=f" node */
	ND_MINUSASSR,		/**< "-=f" node */
	ND_MULTASSR,		/**< "*=f" node */
	ND_DIVASSR,			/**< "/=f" node */
	ND_ASSATR = 8068,	/**< "=@f" node */
	ND_PLUSASSATR,		/**< "+=@f" node */
	ND_MINUSASSATR,		/**< "-=@f" node */
	ND_MULTASSATR,		/**< "*=@f" node */
	ND_DIVASSATR,		/**< "/=@f" node */

	ND_EQEQR = 8081,	/**< "==f" node */
	ND_NOTEQR,			/**< "!=f" node */
	ND_LTR,				/**< "<f" node */
	ND_GTR,				/**< ">f" node */
	ND_LER,				/**< "<=f" node */
	ND_GER,				/**< ">=f" node */
	ND_PLUSR,			/**< "+f" node */
	ND_MINUSR,			/**< "-f" node */
	ND_MULTR,			/**< "*f" node */
	ND_DIVR,			/**< "/f" node */
	ND_POSTINCR,		/**< "POSTINCf" node */
	ND_POSTDECR,		/**< "POSTDECf" node */
	ND_INCR,			/**< "INCf" node */
	ND_DECR,			/**< "DECf" node */
	ND_POSTINCATR,		/**< "POSTINC@f" node */
	ND_POSTDECATR,		/**< "POSTDEC@f" node */
	ND_INCATR,			/**< "INC@f" node */
	ND_DECATR,			/**< "DEC@f" node */
	ND_UNMINUSR,		/**< "UNIMINUSf" node */

	ND_REMASSV = 8201,	/**< "%=V" node */
	ND_SHLASSV,			/**< "<<=V" node */
	ND_SHRASSV,			/**< ">>=V" node */
	ND_ANDASSV,			/**< "&=V" node */
	ND_EXORASSV,		/**< "^=V" node */
	ND_ORASSV,			/**< "|=V" node */
	ND_ASSV,			/**< "=V" node */
	ND_PLUSASSV,		/**< "+=V" node */
	ND_MINUSASSV,		/**< "-=V" node */
	ND_MULTASSV,		/**< "*=V" node */
	ND_DIVASSV,			/**< "/=V" node */
	ND_REMASSATV,		/**< "%=@V" node */
	ND_SHLASSATV,		/**< "<<=@V" node */
	ND_SHRASSATV,		/**< ">>=@V" node */
	ND_ANDASSATV,		/**< "&=@V" node */
	ND_EXORASSATV,		/**< "^=@V" node */
	ND_ORASSATV,		/**< "|=@V" node */
	ND_ASSATV,			/**< "=@V" node */
	ND_PLUSASSATV,		/**< "+=@V" node */
	ND_MINUSASSATV,		/**< "-=@V" node */
	ND_MULTASSATV,		/**< "*=@V" node */
	ND_DIVASSATV,		/**< "/=@V" node */

	ND_ASSRV = 8257,	/**< "=fV" node */
	ND_PLUSASSRV,		/**< "+=fV" node */
	ND_MINUSASSRV,		/**< "-=fV" node */
	ND_MULTASSRV,		/**< *=fV" node */
	ND_DIVASSRV,		/**< "/=fV" node */
	ND_ASSATRV = 8268,	/**< "=@fV" node */
	ND_PLUSASSATRV,		/**< "+=@fV" node */
	ND_MINUSASSATRV,	/**< "-=@fV" node */
	ND_MULTASSATRV,		/**< *=@fV" node */
	ND_DIVASSATRV,		/**< "/=@fV" node */

	ND_POSTINCV = 8241,	/**< "POSTINCV" node */
	ND_POSTDECV,		/**< "POSTDECV" node */
	ND_INCV,			/**< "INCV" node */
	ND_DECV,			/**< "DECV" node */
	ND_POSTINCATV,		/**< "POSTINC@V" node */
	ND_POSTDECATV,		/**< "POSTDEC@V" node */
	ND_INCATV,			/**< "INC@V" node */
	ND_DECATV,			/**< "DEC@V" node */

	ND_POSTINCRV = 8291,/**< "POSTINCfV" node */
	ND_POSTDECRV,		/**< "POSTDECfV" node */
	ND_INCRV,			/**< "INCfV" node */
	ND_DECRV,			/**< "DECfV" node */
	ND_POSTINCATRV,		/**< "POSTINC@fV" node */
	ND_POSTDECATRV,		/**< "POSTDEC@fV" node */
	ND_INCATRV,			/**< "INC@fV" node */
	ND_DECATRV,			/**< "DEC@fV" node */

	ND_NULL,			/**< Empty node */
	ND_WIDEN,			/**< "WIDEN" node */
	ND_WIDEN1,			/**< "WIDEN1" node */
	ND_STRINGINIT,		/**< "STRINGINIT" node */
	ND_ADLOGOR,			/**< "ADLOGOR" node */
	ND_ADLOGAND,		/**< "ADLOGAND" node */
	ND_ROWING,			/**< "ROWING" node */
	ND_ROWINGD,			/**< "ROWINGD" node */

	// Standard functions
	ND_COPY00,			/**< "COPY00" node */
	ND_COPY01,			/**< "COPY01" node */
	ND_COPY10,			/**< "COPY10" node */
	ND_COPY11,			/**< "COPY11" node */
	ND_COPY0ST,			/**< "COPY0ST" node */
	ND_COPY1ST,			/**< "COPY1ST" node */
	ND_COPY0STASS,		/**< "COPY0STASS" node */
	ND_COPY1STASS,		/**< "COPY1STASS" node */
	ND_COPYST,			/**< "COPYST" node */

	ND_ABSI,			/**< "ABSI" node */
	ND_ABS,				/**< "ABS" node */
	ND_SQRT,			/**< "SQRT" node */
	ND_EXP,				/**< "EXP" node */
	ND_SIN,				/**< "SIN" node */
	ND_COS,				/**< "COS" node */
	ND_LOG,				/**< "LOG" node */
	ND_LOG10,			/**< "LOG10" node */
	ND_ASIN,			/**< "ASIN" node */
	ND_RAND,			/**< "RAND" node */
	ND_ROUND,			/**< "ROUND" node */

	ND_STRCPY,			/**< "STRCPY" node */
	ND_STRNCPY,			/**< "STRNCPY" node */
	ND_STRCAT,			/**< "STRCAT" node */
	ND_STRNCAT,			/**< "STRNCAT" node */
	ND_STRCMP,			/**< "STRCMP" node */
	ND_STRNCMP,			/**< "STRNCMP" node */
	ND_STRSTR,			/**< "STRSTR" node */
	ND_STRLEN,			/**< "STRLEN" node */

	ND_MSG_SEND,		/**< "MSG_SEND" node */
	ND_MSG_RECEIVE,		/**< "MSG_RECEIVE" node */
	ND_JOIN,			/**< "JOIN" node */
	ND_SLEEP,			/**< "SLEEP" node */
	ND_SEMCREATE,		/**< "SEMCREATE" node */
	ND_SEMWAIT,			/**< "SEMWAIT" node */
	ND_SEMPOST,			/**< "SEMPOST" node */
	ND_CREATE,			/**< "CREATE" node */
	ND_INIT,			/**< "INIT" node */
	ND_DESTROY,			/**< "DESTROY" node */
	ND_EXIT,			/**< "EXIT" node */
	ND_GETNUM,			/**< "GETNUM" node */

	ND_UPB,				/**< "UPB" node */
	ND_SEND_INT,		/**< "SEND_INT" node */
	ND_SEND_FLOAT,		/**< "SEND_FLOAT" node */
	ND_SEND_STRING,		/**< "SEND_STRING" node */
	ND_RECEIVE_INT,		/**< "RECEIVE_INT" node */
	ND_RECEIVE_FLOAT,	/**< "RECEIVE_FLOAT" node */
	ND_RECEIVE_STRING,	/**< "RECEIVE_STRING" node */
	ND_ASSERT,			/**< "ASSERT" node */
} node_t;


/**
 *	Convert node type to corresponding instruction code
 *
 *	@param	node	Node type
 *
 *	@return	Instruction code
 */
instruction_t node_to_instruction(const node_t node);

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
