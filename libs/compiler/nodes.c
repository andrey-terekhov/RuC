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

node_t token_to_node(const token_t token)
{
	switch (token)
	{
		case TK_TILDE:					return ND_NOT;
		case TK_PLUS:					return ND_ADD;
		case TK_MINUS:					return ND_SUB;
		case TK_STAR:					return ND_MUL;
		case TK_SLASH:					return ND_DIV;
		case TK_PERCENT:				return ND_REM;
		case TK_EXCLAIM:				return ND_LOG_NOT;
		case TK_CARET:					return ND_XOR;
		case TK_PIPE:					return ND_OR;
		case TK_AMP:					return ND_AND;
		case TK_EQUAL:					return ND_ASSIGN;
		case TK_LESS:					return ND_LT;
		case TK_GREATER:				return ND_GT;
		case TK_PLUS_EQUAL:				return ND_ADD_ASSIGN;
		case TK_PLUS_PLUS:				return ND_PREINC;
		case TK_MINUS_EQUAL:			return ND_SUB_ASSIGN;
		case TK_MINUS_MINUS:			return ND_PREDEC;
		case TK_STAR_EQUAL:				return ND_MUL_ASSIGN;
		case TK_SLASH_EQUAL:			return ND_DIV_ASSIGN;
		case TK_PERCENT_EQUAL:			return ND_REM_ASSIGN;
		case TK_EXCLAIM_EQUAL:			return ND_NE;
		case TK_CARET_EQUAL:			return ND_XOR_ASSIGN;
		case TK_PIPE_EQUAL:				return ND_OR_ASSIGN;
		case TK_PIPE_PIPE:				return ND_LOG_OR;
		case TK_AMP_EQUAL:				return ND_AND_ASSIGN;
		case TK_AMP_AMP:				return ND_LOG_AND;
		case TK_EQUAL_EQUAL:			return ND_EQ;
		case TK_LESS_EQUAL:				return ND_LE;
		case TK_LESS_LESS:				return ND_SHL;
		case TK_GREATER_EQUAL:			return ND_GE;
		case TK_GREATER_GREATER:		return ND_SHR;
		case TK_LESS_LESS_EQUAL:		return ND_SHL_ASSIGN;
		case TK_GREATER_GREATER_EQUAL:	return ND_SHR_ASSIGN;

		case TK_ABS:					return ND_ABS;
		case TK_SQRT:					return ND_SQRT;
		case TK_EXP:					return ND_EXP;
		case TK_SIN:					return ND_SIN;
		case TK_COS:					return ND_COS;
		case TK_LOG:					return ND_LOG;
		case TK_LOG10:					return ND_LOG10;
		case TK_ASIN:					return ND_ASIN;
		case TK_RAND:					return ND_RAND;
		case TK_ROUND:					return ND_ROUND;
		case TK_STRCPY:					return ND_STRCPY;
		case TK_STRNCPY:				return ND_STRNCPY;
		case TK_STRCAT:					return ND_STRCAT;
		case TK_STRNCAT:				return ND_STRNCAT;
		case TK_STRCMP:					return ND_STRCMP;
		case TK_STRNCMP:				return ND_STRNCMP;
		case TK_STRSTR:					return ND_STRSTR;
		case TK_STRLEN:					return ND_STRLEN;
		case TK_ASSERT:					return ND_ASSERT;
		case TK_UPB:					return ND_UPB;
		case TK_CREATE_DIRECT:			return ND_CREATE_DIRECT;
		case TK_EXIT_DIRECT:			return ND_EXIT_DIRECT;
		case TK_MSG_SEND:				return ND_MSG_SEND;
		case TK_MSG_RECEIVE:			return ND_MSG_RECEIVE;
		case TK_JOIN:					return ND_JOIN;
		case TK_SLEEP:					return ND_SLEEP;
		case TK_SEM_CREATE:				return ND_SEM_CREATE;
		case TK_SEM_WAIT:				return ND_SEM_WAIT;
		case TK_SEM_POST:				return ND_SEM_POST;
		case TK_CREATE:					return ND_CREATE;
		case TK_INIT:					return ND_INIT;
		case TK_DESTROY:				return ND_DESTROY;
		case TK_EXIT:					return ND_EXIT;
		case TK_GETNUM:					return ND_GETNUM;
		case TK_ROBOT_SEND_INT:			return ND_SEND_INT;
		case TK_ROBOT_SEND_FLOAT:		return ND_SEND_FLOAT;
		case TK_ROBOT_SEND_STRING:		return ND_SEND_STRING;
		case TK_ROBOT_RECEIVE_INT:		return ND_RECEIVE_INT;
		case TK_ROBOT_RECEIVE_FLOAT:	return ND_RECEIVE_FLOAT;
		case TK_ROBOT_RECEIVE_STRING:	return ND_RECEIVE_STRING;
		default:						return 0;
	}
}

node_t node_to_address_ver(const node_t node)
{
	switch (node)
	{
		case ND_PREINC:		return ND_PREINC_AT;
		case ND_PREDEC:		return ND_PREDEC_AT;
		case ND_POSTINC:	return ND_POSTINC_AT;
		case ND_POSTDEC:	return ND_POSTDEC_AT;
		case ND_REM_ASSIGN:	return ND_REM_ASSIGN_AT;
		case ND_SHL_ASSIGN:	return ND_SHL_ASSIGN_AT;
		case ND_SHR_ASSIGN:	return ND_SHR_ASSIGN_AT;
		case ND_AND_ASSIGN:	return ND_AND_ASSIGN_AT;
		case ND_XOR_ASSIGN:	return ND_XOR_ASSIGN_AT;
		case ND_OR_ASSIGN:	return ND_OR_ASSIGN_AT;
		case ND_ASSIGN:		return ND_ASSIGN_AT;
		case ND_ADD_ASSIGN:	return ND_ADD_ASSIGN_AT;
		case ND_SUB_ASSIGN:	return ND_SUB_ASSIGN_AT;
		case ND_MUL_ASSIGN:	return ND_MUL_ASSIGN_AT;
		case ND_DIV_ASSIGN:	return ND_DIV_ASSIGN_AT;

		default:
			return node;
	}
}

node_t node_to_void_ver(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIV_ASSIGN_AT)
		|| (node >= ND_POSTINC && node <= ND_PREDEC_AT)
		|| (node >= ND_ASSIGN_R && node <= ND_DIV_ASSIGN_AT_R)
		|| (node >= ND_POSTINC_R && node <= ND_PREDEC_AT_R))
	{
		return node + DISPL_TO_VOID;
	}
	else
	{
		return node;
	}
}

node_t node_to_float_ver(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIV_ASSIGN)
		|| (node >= ND_ASSIGN_AT && node <= ND_DIV_ASSIGN_AT)
		|| (node >= ND_EQ && node <= ND_UNMINUS)
		|| node == ND_STRING || node == ND_IDENT_TO_VAL || node == ND_ADDR_TO_VAL || node == ND_CONST)
	{
		return node + DISPL_TO_FLOAT;
	}

	return node;
}

int node_is_assignment(const node_t op)
{
	return ((op >= ND_REM_ASSIGN && op <= ND_DIV_ASSIGN) || (op >= ND_REM_ASSIGN_V && op <= ND_DIV_ASSIGN_V)
		|| (op >= ND_ASSIGN_R && op <= ND_DIV_ASSIGN_R) || (op >= ND_ASSIGN_R_V && op <= ND_DIV_ASSIGN_R_V)
		|| (op >= ND_POSTINC && op <= ND_PREDEC) || (op >= ND_POSTINC_V && op <= ND_PREDEC_V)
		|| (op >= ND_POSTINC_R && op <= ND_PREDEC_R) || (op >= ND_POSTINC_R_V && op <= ND_PREDEC_R_V));
}
