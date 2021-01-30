/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev
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

#include "codegen_llvm1.h"
#include "tree.h"
#include "uniprinter.h"


static void statement(universal_io *const io, syntax *const sx, node *const nd)
{

}

static void block(universal_io *const io, syntax *const sx, node *const nd)
{
    do
    {
        switch (node_get_type(nd))
        {
            case TFuncdef:
            {
                node_set_next(nd); // TBegin
                node_set_next(nd);
                block(io, sx, nd);
                break;
            }
            case TDeclarr:  // todo
                break;
            case TDeclid:   // todo
                break;
            case NOP:
            case TStructbeg:
            case TStructend:
            {
                node_set_next(nd);
                break;
            }
            default:
                statement(io, sx, nd);
        }
    } while (node_get_type(nd) != TEnd);
    node_set_next(nd); // TEnd   
}

/** Генерация кодов llvm. Первый проход по дереву */
static int codegen_llvm1(universal_io *const io, syntax *const sx, const char *filename)
{
    uni_printf(io, "source_filename =  \"%s\"\n", filename);
    // архитектурно-зависимая часть
    // в дальнейшем в кодогенератор должны передаваться параметры с информацией, 
    // для какой архитектуры генерировать код
    // сейчас это архитектура mipsel
    uni_printf(io, "target datalayout = \"e-m:m-p:32:32-i8:8:32-i16:16:32-i64:64-n32-S64\"\n");
    uni_printf(io, "target triple = \"mipsel\"\n\n");

    node root = node_get_root(sx);
    node_set_next(&root);
    block(io, sx, &root);

    return 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_llvm1(universal_io *const io, syntax *const sx, const char *filename)
{
	if (!out_is_correct(io) || sx == NULL)
	{
		return -1;
	}

	return codegen_llvm1(io, sx, filename);
}
