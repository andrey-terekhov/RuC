/*
 *	Copyright 2022 Andrey Terekhov, Victor Y. Fadeev
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

#include "error.h"
#include "linker.h"
#include "locator.h"
#include "storage.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Parser structure */
typedef struct parser
{
	linker *lk;						/**< Linker structure */
	storage *stg;					/**< Macro storage */

	universal_io *io;				/**< Universal IO structure */
	location *prev;					/**< Parent macro location */
	location *loc;					/**< Current location */

	size_t include;					/**< Current include depth */
	size_t call;					/**< Current macro call depth */

	bool is_recovery_disabled;		/**< Set, if error recovery & multiple output disabled */
	bool is_line_required;			/**< Set, if position directive required */
	bool was_error;					/**< Set, if error message occurred */
} parser;


/**
 *	Create parser structure
 *
 *	@param	lk			Linker structure
 *	@param	stg			Macro storage
 *	@param	out			Output stream
 *
 *	@return	Parser structure
 */
parser parser_create(linker *const lk, storage *const stg, universal_io *const out);


/**
 *	Preprocess input stream
 *
 *	@param	prs			Parser structure
 *	@param	in			Input stream
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int parser_preprocess(parser *const prs, universal_io *const in);


/**
 *	Disable error recovery
 *
 *	@param	prs			Parser structure
 *	@param	status		Recovery status
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int parser_disable_recovery(parser *const prs, const bool status);

/**
 *	Check that parser is correct
 *
 *	@param	prs			Parser structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool parser_is_correct(const parser *const prs);


/**
 *	Clear parser structure
 *
 *	@param	prs			Parser structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int parser_clear(parser *const prs);

#ifdef __cplusplus
} /* extern "C" */
#endif
