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

#include "operations.h"
#include "errors.h"


static const size_t DISPL_TO_FLOAT = 50;
static const size_t DISPL_TO_VOID = 200;


operation_t token_to_binary(const token_t token)
{
	switch (token)
	{
		case TK_PLUS:					return OP_ADD;
		case TK_MINUS:					return OP_SUB;
		case TK_STAR:					return OP_MUL;
		case TK_SLASH:					return OP_DIV;
		case TK_PERCENT:				return OP_REM;
		case TK_CARET:					return OP_XOR;
		case TK_PIPE:					return OP_OR;
		case TK_AMP:					return OP_AND;
		case TK_EQUAL:					return OP_ASSIGN;
		case TK_LESS:					return OP_LT;
		case TK_GREATER:				return OP_GT;
		case TK_PLUS_EQUAL:				return OP_ADD_ASSIGN;
		case TK_MINUS_EQUAL:			return OP_SUB_ASSIGN;
		case TK_STAR_EQUAL:				return OP_MUL_ASSIGN;
		case TK_SLASH_EQUAL:			return OP_DIV_ASSIGN;
		case TK_PERCENT_EQUAL:			return OP_REM_ASSIGN;
		case TK_EXCLAIM_EQUAL:			return OP_NE;
		case TK_CARET_EQUAL:			return OP_XOR_ASSIGN;
		case TK_PIPE_EQUAL:				return OP_OR_ASSIGN;
		case TK_PIPE_PIPE:				return OP_LOG_OR;
		case TK_AMP_EQUAL:				return OP_AND_ASSIGN;
		case TK_AMP_AMP:				return OP_LOG_AND;
		case TK_EQUAL_EQUAL:			return OP_EQ;
		case TK_LESS_EQUAL:				return OP_LE;
		case TK_LESS_LESS:				return OP_SHL;
		case TK_GREATER_EQUAL:			return OP_GE;
		case TK_GREATER_GREATER:		return OP_SHR;
		case TK_LESS_LESS_EQUAL:		return OP_SHL_ASSIGN;
		case TK_GREATER_GREATER_EQUAL:	return OP_SHR_ASSIGN;

		default:
			system_error(node_unexpected);
			return 0;
	}
}

operation_t token_to_unary(const token_t token)
{
	switch (token)
	{
		case TK_TILDE:					return OP_NOT;
		case TK_EXCLAIM:				return OP_LOG_NOT;
		case TK_PLUS_PLUS:				return OP_PRE_INC;
		case TK_MINUS_MINUS:			return OP_PRE_DEC;

		default:
			system_error(node_unexpected);
			return 0;
	}
}

instruction_t builtin_to_instruction(const builtin_t builtin)
{
	switch (builtin)
	{
		case BI_SQRT:					return IC_SQRT;
		case BI_EXP:					return IC_EXP;
		case BI_SIN:					return IC_SIN;
		case BI_COS:					return IC_COS;
		case BI_LOG:					return IC_LOG;
		case BI_LOG10:					return IC_LOG10;
		case BI_ASIN:					return IC_ASIN;
		case BI_RAND:					return IC_RAND;
		case BI_ROUND:					return IC_ROUND;
		case BI_STRCPY:					return IC_STRCPY;
		case BI_STRNCPY:				return IC_STRNCPY;
		case BI_STRCAT:					return IC_STRCAT;
		case BI_STRNCAT:				return IC_STRNCAT;
		case BI_STRCMP:					return IC_STRCMP;
		case BI_STRNCMP:				return IC_STRNCMP;
		case BI_STRSTR:					return IC_STRSTR;
		case BI_STRLEN:					return IC_STRLEN;
		case BI_ASSERT:					return IC_ASSERT;
		case BI_MSG_SEND:				return IC_MSG_SEND;
		case BI_MSG_RECEIVE:			return IC_MSG_RECEIVE;
		case BI_JOIN:					return IC_JOIN;
		case BI_SLEEP:					return IC_SLEEP;
		case BI_SEM_CREATE:				return IC_SEM_CREATE;
		case BI_SEM_WAIT:				return IC_SEM_WAIT;
		case BI_SEM_POST:				return IC_SEM_POST;
		case BI_CREATE:					return IC_CREATE;
		case BI_INIT:					return IC_INIT;
		case BI_DESTROY:				return IC_DESTROY;
		case BI_EXIT:					return IC_EXIT;
		case BI_GETNUM:					return IC_GETNUM;
		case BI_ROBOT_SEND_INT:			return IC_ROBOT_SEND_INT;
		case BI_ROBOT_SEND_FLOAT:		return IC_ROBOT_SEND_FLOAT;
		case BI_ROBOT_SEND_STRING:		return IC_ROBOT_SEND_STRING;
		case BI_ROBOT_RECEIVE_INT:		return IC_ROBOT_RECEIVE_INT;
		case BI_ROBOT_RECEIVE_FLOAT:	return IC_ROBOT_RECEIVE_FLOAT;
		case BI_ROBOT_RECEIVE_STRING:	return IC_ROBOT_RECEIVE_STRING;

		default:
			system_error(node_unexpected);
			return 0;
	}
}

operation_t operation_to_address_ver(const operation_t operation)
{
	switch (operation)
	{
		case OP_PRE_INC:	return OP_PRE_INC_AT;
		case OP_PRE_DEC:	return OP_PRE_DEC_AT;
		case OP_POST_INC:	return OP_POST_INC_AT;
		case OP_POST_DEC:	return OP_POST_DEC_AT;
		case OP_REM_ASSIGN:	return OP_REM_ASSIGN_AT;
		case OP_SHL_ASSIGN:	return OP_SHL_ASSIGN_AT;
		case OP_SHR_ASSIGN:	return OP_SHR_ASSIGN_AT;
		case OP_AND_ASSIGN:	return OP_AND_ASSIGN_AT;
		case OP_XOR_ASSIGN:	return OP_XOR_ASSIGN_AT;
		case OP_OR_ASSIGN:	return OP_OR_ASSIGN_AT;
		case OP_ASSIGN:		return OP_ASSIGN_AT;
		case OP_ADD_ASSIGN:	return OP_ADD_ASSIGN_AT;
		case OP_SUB_ASSIGN:	return OP_SUB_ASSIGN_AT;
		case OP_MUL_ASSIGN:	return OP_MUL_ASSIGN_AT;
		case OP_DIV_ASSIGN:	return OP_DIV_ASSIGN_AT;

		default:
			return operation;
	}
}

operation_t operation_to_void_ver(const operation_t operation)
{
	return (operation >= OP_ASSIGN && operation <= OP_DIV_ASSIGN_AT)
		|| (operation >= OP_POST_INC && operation <= OP_PRE_DEC_AT)
		|| (operation >= OP_ASSIGN_R && operation <= OP_DIV_ASSIGN_AT_R)
		|| (operation >= OP_POST_INC_R && operation <= OP_PRE_DEC_AT_R)
			? operation + DISPL_TO_VOID
			: operation;
}

operation_t operation_to_float_ver(const operation_t operation)
{
	switch (operation)
	{
		case OP_STRING:
			return OP_STRING_D;
		case OP_IDENT_TO_VAL:
			return OP_IDENT_TO_VAL_D;
		case OP_ADDR_TO_VAL:
			return OP_ADDR_TO_VAL_D;
		case OP_CONST:
			return OP_CONST_D;

		default:
			return (operation >= OP_ASSIGN && operation <= OP_DIV_ASSIGN)
				|| (operation >= OP_ASSIGN_AT && operation <= OP_DIV_ASSIGN_AT)
				|| (operation >= OP_EQ && operation <= OP_UNMINUS)
					? operation + DISPL_TO_FLOAT
					: operation;
	}
}

bool operation_is_assignment(const operation_t op)
{
	return (op >= OP_REM_ASSIGN && op <= OP_DIV_ASSIGN)
		|| (op >= OP_REM_ASSIGN_V && op <= OP_DIV_ASSIGN_V)
		|| (op >= OP_ASSIGN_R && op <= OP_DIV_ASSIGN_R)
		|| (op >= OP_ASSIGN_R_V && op <= OP_DIV_ASSIGN_R_V)
		|| (op >= OP_POST_INC && op <= OP_PRE_DEC)
		|| (op >= OP_POST_INC_V && op <= OP_PRE_DEC_V)
		|| (op >= OP_POST_INC_R && op <= OP_PRE_DEC_R)
		|| (op >= OP_POST_INC_R_V && op <= OP_PRE_DEC_R_V);
}
