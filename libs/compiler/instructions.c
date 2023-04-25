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

#include "instructions.h"
#include "errors.h"


static const size_t DISPL_TO_FLOAT = 50;
static const size_t DISPL_TO_VOID = 200;


instruction_t builtin_to_instruction(const builtin_t func)
{
    switch (func)
    {
        case BI_SQRT:
            return IC_SQRT;
        case BI_EXP:
            return IC_EXP;
        case BI_SIN:
            return IC_SIN;
        case BI_COS:
            return IC_COS;
        case BI_LOG:
            return IC_LOG;
        case BI_LOG10:
            return IC_LOG10;
        case BI_ASIN:
            return IC_ASIN;
        case BI_RAND:
            return IC_RAND;
        case BI_ROUND:
            return IC_ROUND;
        case BI_STRCPY:
            return IC_STRCPY;
        case BI_STRNCPY:
            return IC_STRNCPY;
        case BI_STRCAT:
            return IC_STRCAT;
        case BI_STRNCAT:
            return IC_STRNCAT;
        case BI_STRCMP:
            return IC_STRCMP;
        case BI_STRNCMP:
            return IC_STRNCMP;
        case BI_STRSTR:
            return IC_STRSTR;
        case BI_STRLEN:
            return IC_STRLEN;
        case BI_ASSERT:
            return IC_ASSERT;
        case BI_MSG_SEND:
            return IC_MSG_SEND;
        case BI_MSG_RECEIVE:
            return IC_MSG_RECEIVE;
        case BI_T_JOIN:
            return IC_JOIN;
        case BI_T_SLEEP:
            return IC_SLEEP;
        case BI_SEM_CREATE:
            return IC_SEM_CREATE;
        case BI_SEM_WAIT:
            return IC_SEM_WAIT;
        case BI_SEM_POST:
            return IC_SEM_POST;
        case BI_T_CREATE:
            return IC_CREATE;
        case BI_T_INIT:
            return IC_INIT;
        case BI_T_DESTROY:
            return IC_DESTROY;
        case BI_T_EXIT:
            return IC_EXIT;
        case BI_T_GETNUM:
            return IC_GETNUM;
        case BI_ROBOT_SEND_INT:
            return IC_ROBOT_SEND_INT;
        case BI_ROBOT_SEND_FLOAT:
            return IC_ROBOT_SEND_FLOAT;
        case BI_ROBOT_SEND_STRING:
            return IC_ROBOT_SEND_STRING;
        case BI_ROBOT_RECEIVE_INT:
            return IC_ROBOT_RECEIVE_INT;
        case BI_ROBOT_RECEIVE_FLOAT:
            return IC_ROBOT_RECEIVE_FLOAT;
        case BI_ROBOT_RECEIVE_STRING:
            return IC_ROBOT_RECEIVE_STRING;
        case BI_FOPEN:
            return IC_FOPEN;
        case BI_FGETC:
            return IC_FGETC;
        case BI_FPUTC:
            return IC_FPUTC;
        case BI_FCLOSE:
            return IC_FCLOSE;

        default:
            system_error(node_unexpected);
            return IC_NOP;
    }
}

instruction_t unary_to_instruction(const unary_t op)
{
    switch (op)
    {
        case UN_POSTINC:
            return IC_POST_INC;
        case UN_POSTDEC:
            return IC_POST_DEC;
        case UN_PREINC:
            return IC_PRE_INC;
        case UN_PREDEC:
            return IC_PRE_DEC;
        case UN_ADDRESS:
            return IC_LA;
        case UN_INDIRECTION:
            return IC_LOAD;
        case UN_MINUS:
            return IC_UNMINUS;
        case UN_NOT:
            return IC_NOT;
        case UN_LOGNOT:
            return IC_LOG_NOT;
        case UN_ABS:
            return IC_ABSI;

        default:
            return IC_NOP;
    }
}

instruction_t binary_to_instruction(const binary_t op)
{
    switch (op)
    {
        case BIN_MUL:
            return IC_MUL;
        case BIN_DIV:
            return IC_DIV;
        case BIN_REM:
            return IC_REM;
        case BIN_ADD:
            return IC_ADD;
        case BIN_SUB:
            return IC_SUB;
        case BIN_SHL:
            return IC_SHL;
        case BIN_SHR:
            return IC_SHR;
        case BIN_LT:
            return IC_LT;
        case BIN_GT:
            return IC_GT;
        case BIN_LE:
            return IC_LE;
        case BIN_GE:
            return IC_GE;
        case BIN_EQ:
            return IC_EQ;
        case BIN_NE:
            return IC_NE;
        case BIN_AND:
            return IC_AND;
        case BIN_XOR:
            return IC_XOR;
        case BIN_OR:
            return IC_OR;
        case BIN_LOG_AND:
            return IC_LOG_AND;
        case BIN_LOG_OR:
            return IC_LOG_OR;
        case BIN_ASSIGN:
            return IC_ASSIGN;
        case BIN_MUL_ASSIGN:
            return IC_MUL_ASSIGN;
        case BIN_DIV_ASSIGN:
            return IC_DIV_ASSIGN;
        case BIN_REM_ASSIGN:
            return IC_REM_ASSIGN;
        case BIN_ADD_ASSIGN:
            return IC_ADD_ASSIGN;
        case BIN_SUB_ASSIGN:
            return IC_SUB_ASSIGN;
        case BIN_SHL_ASSIGN:
            return IC_SHL_ASSIGN;
        case BIN_SHR_ASSIGN:
            return IC_SHR_ASSIGN;
        case BIN_AND_ASSIGN:
            return IC_AND_ASSIGN;
        case BIN_XOR_ASSIGN:
            return IC_XOR_ASSIGN;
        case BIN_OR_ASSIGN:
            return IC_OR_ASSIGN;

        default:
            return IC_NOP;
    }
}


instruction_t instruction_to_address_ver(const instruction_t instruction)
{
    switch (instruction)
    {
        case IC_PRE_INC:
            return IC_PRE_INC_AT;
        case IC_PRE_DEC:
            return IC_PRE_DEC_AT;
        case IC_POST_INC:
            return IC_POST_INC_AT;
        case IC_POST_DEC:
            return IC_POST_DEC_AT;
        case IC_REM_ASSIGN:
            return IC_REM_ASSIGN_AT;
        case IC_SHL_ASSIGN:
            return IC_SHL_ASSIGN_AT;
        case IC_SHR_ASSIGN:
            return IC_SHR_ASSIGN_AT;
        case IC_AND_ASSIGN:
            return IC_AND_ASSIGN_AT;
        case IC_XOR_ASSIGN:
            return IC_XOR_ASSIGN_AT;
        case IC_OR_ASSIGN:
            return IC_OR_ASSIGN_AT;
        case IC_ASSIGN:
            return IC_ASSIGN_AT;
        case IC_ADD_ASSIGN:
            return IC_ADD_ASSIGN_AT;
        case IC_SUB_ASSIGN:
            return IC_SUB_ASSIGN_AT;
        case IC_MUL_ASSIGN:
            return IC_MUL_ASSIGN_AT;
        case IC_DIV_ASSIGN:
            return IC_DIV_ASSIGN_AT;

        default:
            return instruction;
    }
}

instruction_t instruction_to_floating_ver(const instruction_t instruction)
{
    return (instruction >= IC_ASSIGN && instruction <= IC_DIV_ASSIGN) ||
                   (instruction >= IC_ASSIGN_AT && instruction <= IC_DIV_ASSIGN_AT) ||
                   (instruction >= IC_EQ && instruction <= IC_UNMINUS)
               ? (instruction_t)((size_t)instruction + DISPL_TO_FLOAT)
               : instruction;
}

instruction_t instruction_to_void_ver(const instruction_t instruction)
{
    return (instruction >= IC_ASSIGN && instruction <= IC_DIV_ASSIGN_AT) ||
                   (instruction >= IC_POST_INC && instruction <= IC_PRE_DEC_AT) ||
                   (instruction >= IC_ASSIGN_R && instruction <= IC_DIV_ASSIGN_AT_R) ||
                   (instruction >= IC_POST_INC_R && instruction <= IC_PRE_DEC_AT_R)
               ? (instruction_t)((size_t)instruction + DISPL_TO_VOID)
               : instruction;
}
