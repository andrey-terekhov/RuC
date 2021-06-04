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

#include "nodes.h"


static const size_t DISPL_TO_VOID = 200;

operation_t token_to_node(const token_t token)
{
	switch (token)
	{
		case TK_TILDE:					return OP_NOT;
		case TK_PLUS:					return OP_ADD;
		case TK_MINUS:					return OP_SUB;
		case TK_STAR:					return OP_MUL;
		case TK_SLASH:					return OP_DIV;
		case TK_PERCENT:				return OP_REM;
		case TK_EXCLAIM:				return OP_LOG_NOT;
		case TK_CARET:					return OP_XOR;
		case TK_PIPE:					return OP_OR;
		case TK_AMP:					return OP_AND;
		case TK_EQUAL:					return OP_ASSIGN;
		case TK_LESS:					return OP_LT;
		case TK_GREATER:				return OP_GT;
		case TK_PLUS_EQUAL:				return OP_ADD_ASSIGN;
		case TK_PLUS_PLUS:				return OP_PREINC;
		case TK_MINUS_EQUAL:			return OP_SUB_ASSIGN;
		case TK_MINUS_MINUS:			return OP_PREDEC;
		case TK_STAR_EQUAL:				return OP_MUL_ASSIGN;
		case TK_SLASH_EQUAL:			return OP_DIV_ASSIGN;
		case TK_PERCENT_EQUAL:			return OP_REM_ASSIGN;
		case TK_EXCLAIM_EQUAL:			return OP_NE;
		case TK_CARET_EQUAL:			return OP_XOR_ASSIGN;
		case TK_PIPE_EQUAL:				return OP_OR_ASSIGN;
		case TK_PIPE_PIPE:				return OP_LOG_OR;
		case TK_AMP_EQUAL:				return OP_AOP_ASSIGN;
		case TK_AMP_AMP:				return OP_LOG_AND;
		case TK_EQUAL_EQUAL:			return OP_EQ;
		case TK_LESS_EQUAL:				return OP_LE;
		case TK_LESS_LESS:				return OP_SHL;
		case TK_GREATER_EQUAL:			return OP_GE;
		case TK_GREATER_GREATER:		return OP_SHR;
		case TK_LESS_LESS_EQUAL:		return OP_SHL_ASSIGN;
		case TK_GREATER_GREATER_EQUAL:	return OP_SHR_ASSIGN;

		case TK_ABS:					return OP_ABS;
		case TK_SQRT:					return OP_SQRT;
		case TK_EXP:					return OP_EXP;
		case TK_SIN:					return OP_SIN;
		case TK_COS:					return OP_COS;
		case TK_LOG:					return OP_LOG;
		case TK_LOG10:					return OP_LOG10;
		case TK_ASIN:					return OP_ASIN;
		case TK_RAND:					return OP_RAND;
		case TK_ROUND:					return OP_ROUND;
		case TK_STRCPY:					return OP_STRCPY;
		case TK_STRNCPY:				return OP_STRNCPY;
		case TK_STRCAT:					return OP_STRCAT;
		case TK_STRNCAT:				return OP_STRNCAT;
		case TK_STRCMP:					return OP_STRCMP;
		case TK_STRNCMP:				return OP_STRNCMP;
		case TK_STRSTR:					return OP_STRSTR;
		case TK_STRLEN:					return OP_STRLEN;
		case TK_ASSERT:					return OP_ASSERT;
		case TK_UPB:					return OP_UPB;
		case TK_CREATE_DIRECT:			return OP_CREATE_DIRECT;
		case TK_EXIT_DIRECT:			return OP_EXIT_DIRECT;
		case TK_MSG_SEND:				return OP_MSG_SEND;
		case TK_MSG_RECEIVE:			return OP_MSG_RECEIVE;
		case TK_JOIN:					return OP_JOIN;
		case TK_SLEEP:					return OP_SLEEP;
		case TK_SEM_CREATE:				return OP_SEM_CREATE;
		case TK_SEM_WAIT:				return OP_SEM_WAIT;
		case TK_SEM_POST:				return OP_SEM_POST;
		case TK_CREATE:					return OP_CREATE;
		case TK_INIT:					return OP_INIT;
		case TK_DESTROY:				return OP_DESTROY;
		case TK_EXIT:					return OP_EXIT;
		case TK_GETNUM:					return OP_GETNUM;
		case TK_ROBOT_SEND_INT:			return OP_SEND_INT;
		case TK_ROBOT_SEND_FLOAT:		return OP_SEND_FLOAT;
		case TK_ROBOT_SEND_STRING:		return OP_SEND_STRING;
		case TK_ROBOT_RECEIVE_INT:		return OP_RECEIVE_INT;
		case TK_ROBOT_RECEIVE_FLOAT:	return OP_RECEIVE_FLOAT;
		case TK_ROBOT_RECEIVE_STRING:	return OP_RECEIVE_STRING;
		default:						return 0;
	}
}

operation_t node_to_address_ver(const operation_t node)
{
	switch (node)
	{
		case OP_PREINC:		return OP_PREINC_AT;
		case OP_PREDEC:		return OP_PREDEC_AT;
		case OP_POSTINC:	return OP_POSTINC_AT;
		case OP_POSTDEC:	return OP_POSTDEC_AT;
		case OP_REM_ASSIGN:	return OP_REM_ASSIGN_AT;
		case OP_SHL_ASSIGN:	return OP_SHL_ASSIGN_AT;
		case OP_SHR_ASSIGN:	return OP_SHR_ASSIGN_AT;
		case OP_AOP_ASSIGN:	return OP_AOP_ASSIGN_AT;
		case OP_XOR_ASSIGN:	return OP_XOR_ASSIGN_AT;
		case OP_OR_ASSIGN:	return OP_OR_ASSIGN_AT;
		case OP_ASSIGN:		return OP_ASSIGN_AT;
		case OP_ADD_ASSIGN:	return OP_ADD_ASSIGN_AT;
		case OP_SUB_ASSIGN:	return OP_SUB_ASSIGN_AT;
		case OP_MUL_ASSIGN:	return OP_MUL_ASSIGN_AT;
		case OP_DIV_ASSIGN:	return OP_DIV_ASSIGN_AT;

		default:
			return node;
	}
}

operation_t node_to_void_ver(const operation_t node)
{
	if ((node >= OP_ASSIGN && node <= OP_DIV_ASSIGN_AT)
		|| (node >= OP_POSTINC && node <= OP_PREDEC_AT)
		|| (node >= OP_ASSIGN_R && node <= OP_DIV_ASSIGN_AT_R)
		|| (node >= OP_POSTINC_R && node <= OP_PREDEC_AT_R))
	{
		return node + DISPL_TO_VOID;
	}

	return node;
}

operation_t node_to_float_ver(const operation_t node)
{
	if ((node >= OP_ASSIGN && node <= OP_DIV_ASSIGN)
		|| (node >= OP_ASSIGN_AT && node <= OP_DIV_ASSIGN_AT)
		|| (node >= OP_EQ && node <= OP_UNMINUS))
	{
		return node + DISPL_TO_FLOAT;
	}
	else
	{
		switch (node)
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
				return node;
		}
	}
}

int node_is_assignment(const operation_t op)
{
	return ((op >= OP_REM_ASSIGN && op <= OP_DIV_ASSIGN) || (op >= OP_REM_ASSIGN_V && op <= OP_DIV_ASSIGN_V)
		|| (op >= OP_ASSIGN_R && op <= OP_DIV_ASSIGN_R) || (op >= OP_ASSIGN_R_V && op <= OP_DIV_ASSIGN_R_V)
		|| (op >= OP_POSTINC && op <= OP_PREDEC) || (op >= OP_POSTINC_V && op <= OP_PREDEC_V)
		|| (op >= OP_POSTINC_R && op <= OP_PREDEC_R) || (op >= OP_POSTINC_R_V && op <= OP_PREDEC_R_V));
}
