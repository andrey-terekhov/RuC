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

#pragma once

#include "syntax.h"
#include "uniio.h"
#include "workspace.h"


//#define GENERATE_CODES


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Encode to virtual machine codes
 *
 *	@param	ws		Compiler workspace
 *	@param	io		Universal io structure
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int encode_to_vm(const workspace *const ws, universal_io *const io, syntax *const sx);

#ifdef __cplusplus
} /* extern "C" */
#endif
