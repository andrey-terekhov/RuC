/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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

#include "linker.h"
#include "storage.h"
#include "uniio.h"
#include "error.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Parser structure */
typedef struct parser
{
	linker *lk;						/**< Linker structure */
	storage *stg;					/**< Storage structure */

	universal_io *in;				/**< Input io structure */ 
	universal_io *out;				/**< Output io structure */ 

	size_t error_line;				/**< Сurrent line number in input */ 

	bool is_recovery_disabled;		/**< Set, if error recovery & multiple output disabled */
} parser;

/**
 *	Create parser structure
 *
 *	@param	lk		Linker structure
 *	@param	stg		Storage structure
 *	@param	out		Output
 *
 *	@return	Parser structure
 */
parser parser_create(linker *const lk, storage *const stg, universal_io *const out); 


/**
 *	Preprocess input data
 *
 *	@param	prs		Parser structure
 *	@param	in		Input data
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int parser_preprocess(parser *const prs, universal_io *const in); 


/**
 *	Disable error recovery
 *
 *	@param	prs		Parser structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int parser_disable_recovery(parser *const prs);

/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
void parser_error(parser *const prs, const error_t num); 


/**
 *	Check that parser structure is correct
 *
 *	@param	prs		Parser structure
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
