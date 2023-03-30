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

#include "mipsgen.h"
#include "AST.h"
#include "hash.h"
#include "ir.h"
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"


int encode_to_mips(const workspace *const ws, syntax *const sx)
{
	ir_module module = create_ir_module();
	ir_builder builder = create_ir_builder(&module, sx);
	const node root = node_get_root(&sx->tree);
	ir_emit_module(&builder, &root);
	ir_dump(&builder);
	return 0;
}
