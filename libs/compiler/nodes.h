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

#include "defs.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum NODE
{
	// Statements
	ND_LABEL		= TLabel,
	ND_CASE			= TCase,
	ND_DEFAULT		= TDefault,
	ND_BLOCK		= TBegin,
	ND_NULL			= NOP,
	ND_IF			= TIf,
	ND_SWITCH		= TSwitch,
	ND_WHILE		= TWhile,
	ND_DO			= TDo,
	ND_FOR			= TFor,
	ND_GOTO			= TGoto,
	ND_CONTINUE		= TContinue,
	ND_BREAK		= TBreak,
	ND_RETURN_VOID	= TReturnvoid,
	ND_RETURN_VAL	= TReturnval,
	ND_CREATEDIRECT	= CREATEDIRECTC,
	ND_EXITDIRECT	= EXITDIRECTC,
	ND_PRINTID		= TPrintid,
	ND_PRINT		= TPrint,
	ND_GETID		= TGetid,
	ND_PRINTF		= TPrintf,

	// Declarations
	ND_DECL_ID		= TDeclid,
	ND_DECL_ARR		= TDeclarr,
	ND_DECL_STRUCT	= TStructbeg,
	ND_FUNC_DEF		= TFuncdef,
	ND_ARRAY_INIT	= TBeginit,
	ND_STRUCT_INIT	= TStructinit,
} node_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
