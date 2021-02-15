/*
 *	Copyright 2021 Andrey Terekhov, Ivan S. Arkhipov
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

#include "llvmgen.h"
#include "tree.h"
#include "uniprinter.h"


enum ANSWER 
{
    AREG,                           /**< Ответ находится в регистре */
    ACONST,                         /**< Ответ является константой */
};

enum LOCATION 
{
    LREG,                           /**< Переменная находится в регистре */
    LMEM,                           /**< Переменная находится в памяти */
};

typedef struct info
{
    int string_num;                 /**< Номер строки */
    int register_num;               /**< Номер регистра */
    int variable_location;          /**< Расположение переменной */
    int request_reg;                /**< Регистр на запрос */
    int answer_reg;                 /**< Регистр с ответом */
    int answer_num;                 /**< Константа с ответом */
    int answer_type;                /**< Тип ответа */
} info;


static void expression(universal_io *const io, syntax *const sx, node *const nd, info *const context);

static void block(universal_io *const io, syntax *const sx, node *const nd, info *const context);

static void operand(universal_io *const io, syntax *const sx, node *const nd, info *const context)
{
    if (node_get_type(nd) == NOP || node_get_type(nd) == ADLOGOR || node_get_type(nd) == ADLOGAND)
        node_set_next(nd);
    switch (node_get_type(nd))
    {
        case TIdent:
        case TSelect:
        case TIdenttovald:
        case TIdenttoaddr:
        case TConstd:
            node_set_next(nd);
            break;
        case TIdenttoval:
        {
            int displ = node_get_arg(nd, 0);

            uni_printf(io, " %%.%i = load i32, i32* %%var.%i, align 4\n", context->register_num, displ);
            context->answer_reg = context->register_num++;
            context->answer_type = AREG;
            node_set_next(nd);
        }
        break;
        case TConst:
        {
            int num = node_get_arg(nd, 0);

            if (context->variable_location == LMEM)
            {
                uni_printf(io, " store i32 %i, i32* %%var.%i, align 4\n", num, context->request_reg);
                context->answer_type = AREG;
            }
            else
            {
                context->answer_type = ACONST;
                context->answer_num = num;
            }           

            node_set_next(nd);
        }
        break;
        case TString:
            node_set_next(nd);
            break;
        case TSliceident:
        {
            node_set_next(nd);
            expression(io, sx, nd, context);
            while (node_get_type(nd) == TSlice)
            {
                node_set_next(nd);
                expression(io, sx, nd, context);
            }
        }
        break;
        case TCall1:
        {
            int npar = node_get_arg(nd, 0);

            node_set_next(nd);
            for (int i = 0; i < npar; i++)
                expression(io, sx, nd, context);
            node_set_next(nd); // TCall2
        }
        break;
        case TBeginit:
        {
            // здесь будет печать llvm с инициализацией массивов
            int n = node_get_arg(nd, 0);

            node_set_next(nd);
            for (int i = 0; i < n; i++)
                expression(io, sx, nd, context);
        }
        break;
        case TStructinit:
        {
            // здесь будет печать llvm с инициализацией структур
            int n = node_get_arg(nd, 0);

            node_set_next(nd);
            for (int i = 0; i < n; i++)
                expression(io, sx, nd, context);
        }
        break;
        default:
        // отладочная печать, потом здесь будет инициализация какой-нибудь ошибки
            printf("Ooops, something wrong, %li\n", nd->argv);
    }
}

static void expression(universal_io *const io, syntax *const sx, node *const nd, info *const context)
{
    switch (node_get_type(nd))
    {
        // бинарные операции
        // пока не все, будут вводиться по мере тестирования
        case LPLUS:
        case LMINUS:
        case LMULT:
        case LDIV:
        {
            node_set_next(nd);
            expression(io, sx, nd, context);
            expression(io, sx, nd, context);
        }
        break;
        // унарные операции
        // пока не все, будут вводиться по мере тестирования
        case ASS:
        case PLUSASS:
        case MINUSASS:
        case MULTASS:
        case DIVASS:

        case ASSV:
        case PLUSASSV:
        case MINUSASSV:
        case MULTASSV:
        case DIVASSV:
        {
            node_set_next(nd);
            expression(io, sx, nd, context);
        }
        break;
        default:
            operand(io, sx, nd, context);
    }
    if (node_get_type(nd) == TExprend)
        node_set_next(nd);
}

static void statement(universal_io *const io, syntax *const sx, node *const nd, info *const context)
{
    switch (node_get_type(nd))
    {
        case TBegin:
        {
            node_set_next(nd);
            block(io, sx, nd, context);
        }
        break;
        case TIf:
        {
            int ref_else = node_get_arg(nd, 0);

            node_set_next(nd);
            expression(io, sx, nd, context);
            statement(io, sx, nd, context);
            if (ref_else)
                statement(io, sx, nd, context);
        }
        break;
        case TSwitch:
        case TCase:
        case TDefault:
        case TWhile:
        {
            node_set_next(nd);
            expression(io, sx, nd, context);
            statement(io, sx, nd, context);
        }
        break;
        case TDo:
        {
            node_set_next(nd);
            statement(io, sx, nd, context);
            expression(io, sx, nd, context);
        }
        break;
        case TFor:
        {
            const int ref_from = node_get_arg(nd, 0);
			const int ref_cond = node_get_arg(nd, 1);
			const int ref_incr = node_get_arg(nd, 2);

            node_set_next(nd);
            if (ref_from)
                expression(io, sx, nd, context);
            if (ref_cond)
                expression(io, sx, nd, context);
            if (ref_incr)
                expression(io, sx, nd, context);
            statement(io, sx, nd, context);
        }
        break;
        case TLabel:
        {
            node_set_next(nd);
            statement(io, sx, nd, context);
        }
        break;
        case TBreak:
        case TContinue:
        case TReturnvoid:
        case TGoto:
            node_set_next(nd);
            break;
        case TReturnval:
        {
            node_set_next(nd);
            context->variable_location = LREG;
            expression(io, sx, nd, context);
            if (context->answer_type == ACONST)
                uni_printf(io, " ret i32 %i\n", context->answer_num);
        }
        break;
        case TGetid:
            // здесь будет печать llvm для чтения
            node_set_next(nd);
            break;
        case TPrintid:
            // здесь будет печать llvm для вывода
            node_set_next(nd);
            break;
        case TPrintf:
        {
            int n = node_get_arg(nd, 0);
            int args[100];

            node_set_next(nd);
            int string_length = node_get_arg(nd, 0);
            node_set_next(nd); // TString
            node_set_next(nd); // TExprend
            for (int i = 0; i < n; i++)
            {
                context->variable_location = LREG;
                expression(io, sx, nd, context);
                args[i] = context->answer_reg;
            }
            uni_printf(io, " %%.%i = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
            		"([%i x i8], [%i x i8]* @.str%i, i32 0, i32 0)", context->register_num++, string_length+1, string_length+1, context->string_num++);
            for (int i = 0; i < n; i++)
                uni_printf(io, ", i32 signext %%.%i", args[i]);
            uni_printf(io, ")\n");
        }
        break;
        // todo обсудить добавление TScanf
        default:
            expression(io, sx, nd, context);
    }
}

static void init(universal_io *const io, syntax *const sx, node *const nd, info *const context)
{
    switch (node_get_type(nd))
    {
        case TBeginit:
        {
            // здесь будет печать llvm с инициализацией массивов
            int n = node_get_arg(nd, 0);

            node_set_next(nd);
            for (int i = 0; i < n; i++)
                expression(io, sx, nd, context);
        }
        break;
        case TStructinit:
        {
            // здесь будет печать llvm с инициализацией структур
            int n = node_get_arg(nd, 0);

            node_set_next(nd);
            for (int i = 0; i < n; i++)
                expression(io, sx, nd, context);
        }
        break;
        default:
            expression(io, sx, nd, context);
    }
}

static void block(universal_io *const io, syntax *const sx, node *const nd, info *const context)
{
    do
    {
        switch (node_get_type(nd))
        {
            case TFuncdef:
            {
                int ref_ident = node_get_arg(nd, 0) / 4;

                if (ident_get_mode(sx, ref_ident) == LMAIN)
                	uni_printf(io, "define i32 @main(");
                uni_printf(io, ") {\n");

                node_set_next(nd); // TBegin
                node_set_next(nd);
                block(io, sx, nd, context);
                uni_printf(io, "}\n\n");
            }
            break;
            case TDeclarr:
            {
                int n = node_get_arg(nd, 0);

                node_set_next(nd);
                for (int i = 0; i < n; i++)
                    expression(io, sx, nd, context);
            }
            break;
            case TDeclid:
            {
                int displ = node_get_arg(nd, 0), eltype = node_get_arg(nd, 1), 
                    N = node_get_arg(nd, 2), all = node_get_arg(nd, 3);

                if (N == 0) // обычная переменная int a; или struct point p;
                {
                    if (eltype == LINT)
                    {
                        uni_printf(io, " %%var.%i = alloca i32, align 4\n", displ);
                        context->variable_location = LMEM;
                        context->request_reg = displ;
                    }
                }

                node_set_next(nd);
                if (all)
                    init(io, sx, nd, context);
            }
            break;
            case NOP:
            case TStructbeg:
            case TStructend:
                node_set_next(nd);
                break;
            default:
                statement(io, sx, nd, context);
        }
    } while (node_get_type(nd) != TEnd);
    node_set_next(nd); // TEnd   
}

/** Генерация кодов llvm. Первый проход по дереву */
static int codegen_llvm(universal_io *const io, syntax *const sx)
{
    info context;
    context.string_num = 1;
    context.register_num = 1;
    context.variable_location = LREG;
    context.request_reg = 0;
    context.answer_reg = 0;

    node root = node_get_root(&sx->tree);
    node_set_next(&root);
    block(io, sx, &root, &context);

    return 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_llvm(universal_io *const io, syntax *const sx)
{
	if (!out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	return codegen_llvm(io, sx);
}