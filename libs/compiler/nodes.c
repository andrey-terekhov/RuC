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
#include "stdio.h"


node_t token_to_node(const token_t token)
{
	switch (token)
	{
		case TOK_TILDE:					return ND_NOT;
		case TOK_PLUS:					return ND_ADD;
		case TOK_MINUS:					return ND_SUB;
		case TOK_STAR:					return ND_MUL;
		case TOK_SLASH:					return ND_DIV;
		case TOK_PERCENT:				return ND_REM;
		case TOK_EXCLAIM:				return ND_LOGNOT;
		case TOK_CARET:					return ND_XOR;
		case TOK_PIPE:					return ND_OR;
		case TOK_AMP:					return ND_AND;
		case TOK_EQUAL:					return ND_ASSIGN;
		case TOK_LESS:					return ND_LT;
		case TOK_GREATER:				return ND_GT;
		case TOK_PLUSEQUAL:				return ND_ADDASSIGN;
		case TOK_PLUSPLUS:				return ND_PREINC;
		case TOK_MINUSEQUAL:			return ND_SUBASSIGN;
		case TOK_MINUSMINUS:			return ND_PREDEC;
		case TOK_STAREQUAL:				return ND_MULASSIGN;
		case TOK_SLASHEQUAL:			return ND_DIVASSIGN;
		case TOK_PERCENTEQUAL:			return ND_REMASSIGN;
		case TOK_EXCLAIMEQUAL:			return ND_NE;
		case TOK_CARETEQUAL:			return ND_XORASSIGN;
		case TOK_PIPEEQUAL:				return ND_ORASSIGN;
		case TOK_PIPEPIPE:				return ND_LOGOR;
		case TOK_AMPEQUAL:				return ND_ANDASSIGN;
		case TOK_AMPAMP:				return ND_LOGAND;
		case TOK_EQUALEQUAL:			return ND_EQ;
		case TOK_LESSEQUAL:				return ND_LE;
		case TOK_LESSLESS:				return ND_SHL;
		case TOK_GREATEREQUAL:			return ND_GE;
		case TOK_GREATERGREATER:		return ND_SHR;
		case TOK_LESSLESSEQUAL:			return ND_SHLASSIGN;
		case TOK_GREATERGREATEREQUAL:	return ND_SHRASSIGN;

		case TOK_ABS:					return ND_ABS;
		case TOK_SQRT:					return ND_SQRT;
		case TOK_EXP:					return ND_EXP;
		case TOK_SIN:					return ND_SIN;
		case TOK_COS:					return ND_COS;
		case TOK_LOG:					return ND_LOG;
		case TOK_LOG10:					return ND_LOG10;
		case TOK_ASIN:					return ND_ASIN;
		case TOK_RAND:					return ND_RAND;
		case TOK_ROUND:					return ND_ROUND;
		case TOK_STRCPY:				return ND_STRCPY;
		case TOK_STRNCPY:				return ND_STRNCPY;
		case TOK_STRCAT:				return ND_STRCAT;
		case TOK_STRNCAT:				return ND_STRNCAT;
		case TOK_STRCMP:				return ND_STRCMP;
		case TOK_STRNCMP:				return ND_STRNCMP;
		case TOK_STRSTR:				return ND_STRSTR;
		case TOK_STRLEN:				return ND_STRLEN;
		case TOK_ASSERT:				return ND_ASSERT;
		case TOK_UPB:					return ND_UPB;
		case TOK_CREATEDIRECT:			return ND_CREATEDIRECT;
		case TOK_EXITDIRECT:			return ND_EXITDIRECT;
		case TOK_MSG_SEND:				return ND_MSG_SEND;
		case TOK_MSG_RECEIVE:			return ND_MSG_RECEIVE;
		case TOK_JOIN:					return ND_JOIN;
		case TOK_SLEEP:					return ND_SLEEP;
		case TOK_SEMCREATE:				return ND_SEMCREATE;
		case TOK_SEMWAIT:				return ND_SEMWAIT;
		case TOK_SEMPOST:				return ND_SEMPOST;
		case TOK_CREATE:				return ND_CREATE;
		case TOK_INIT:					return ND_INIT;
		case TOK_DESTROY:				return ND_DESTROY;
		case TOK_EXIT:					return ND_EXIT;
		case TOK_GETNUM:				return ND_GETNUM;
		case TOK_SEND_INT:				return ND_SEND_INT;
		case TOK_SEND_FLOAT:			return ND_SEND_FLOAT;
		case TOK_SEND_STRING:			return ND_SEND_STRING;
		case TOK_RECEIVE_INT:			return ND_RECEIVE_INT;
		case TOK_RECEIVE_FLOAT:			return ND_RECEIVE_FLOAT;
		case TOK_RECEIVE_STRING:		return ND_RECEIVE_STRING;
		default:						return 0;
	}
}

node_t node_at_operator(const node_t node)
{
	switch (node)
	{
		case ND_PREINC:		return ND_PREINCAT;
		case ND_PREDEC:		return ND_PREDECAT;
		case ND_POSTINC:	return ND_POSTINCAT;
		case ND_POSTDEC:	return ND_POSTDECAT;

		case ND_REMASSIGN:	return ND_REMASSIGNAT;
		case ND_SHLASSIGN:	return ND_SHLASSIGNAT;
		case ND_SHRASSIGN:	return ND_SHRASSIGNAT;
		case ND_ANDASSIGN:	return ND_ANDASSIGNAT;
		case ND_XORASSIGN:	return ND_XORASSIGNAT;
		case ND_ORASSIGN:	return ND_ORASSIGNAT;
		case ND_ASSIGN:		return ND_ASSIGNAT;
		case ND_ADDASSIGN:	return ND_ADDASSIGNAT;
		case ND_SUBASSIGN:	return ND_SUBASSIGNAT;
		case ND_MULASSIGN:	return ND_MULASSIGNAT;
		case ND_DIVASSIGN:	return ND_DIVASSIGNAT;

		default: 			return 0;
	}
}

node_t node_void_operator(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIVASSIGNAT)
		|| (node >= ND_POSTINC && node <= ND_PREDECAT)
		|| (node >= ND_ASSIGNR && node <= ND_DIVASSIGNATR)
		|| (node >= ND_POSTINCR && node <= ND_PREDECATR))
	{
		return node + 200;
	}
	else
	{
		return node;
	}
}

node_t node_float_operator(const node_t node)
{
	if ((node >= ND_ASSIGN && node <= ND_DIVASSIGN)
		|| (node >= ND_ASSIGNAT && node <= ND_DIVASSIGNAT)
		|| (node >= ND_EQ && node <= ND_UNMINUS))
	{
		return node + 50;
	}
	else
	{
		return node;
	}
}

int node_is_assignment_operator(const node_t op)
{
	if ((op >= ND_REMASSIGN && op <= ND_DIVASSIGN) || (op >= ND_REMASSIGNV && op <= ND_DIVASSIGNV)
		|| (op >= ND_ASSIGNR && op <= ND_DIVASSIGNR) || (op >= ND_ASSIGNRV && op <= ND_DIVASSIGNRV)
		|| (op >= ND_POSTINC && op <= ND_PREDEC) || (op >= ND_POSTINCV && op <= ND_PREDECV)
		|| (op >= ND_POSTINCR && op <= ND_PREDECR) || (op >= ND_POSTINCRV && op <= ND_PREDECRV))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
