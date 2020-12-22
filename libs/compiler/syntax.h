/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev
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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Global vars definition */
typedef struct syntax
{
	// mem, pc & iniprocs - usage here only for codes printing

	int mem[MAXMEMSIZE];		/**< Memory */
	int pc;						/**< Program counter */

	int iniprocs[INIPROSIZE];	/**< Init processes */
	int procd;					/**< Process management daemon */

	int functions[FUNCSIZE];	/**< Functions table */
	int funcnum;				/**< Number of functions */

	int identab[MAXIDENTAB];	/**< Identifiers table */
	int id;						/**< Number of identifiers */

	int modetab[MAXMODETAB];	/**< Modes table */
	int md;						/**< Number of modes */
	int startmode;				/**< Start of last record in modetab */
	
	int tree[MAXTREESIZE];		/**< Tree */
	int tc;						/**< Tree counter */

	int reprtab[MAXREPRTAB];	/**< Representations table */
	int rp;						/**< Representations size */
	int repr;					/**< Representations position */

	int maxdisplg;				/**< Max displacement */
	int wasmain;				/**< Main function flag */

	int anstdispl;				/**< Stack displacement */
} syntax;


/**
 *	Create Syntax structure
 *
 *	@return	Syntax structure
 */
syntax sx_create();


/**
 *	Add new record to functions table
 *
 *	@param	sx			Syntax structure
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_add(syntax *const sx, const size_t ref);

/**
 *	Set function start reference by index in functions table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_set(syntax *const sx, const size_t index, const size_t ref);

/**
 *	Get an item from functions table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *
 *	@return	Item by index from functions table, @c INT_MAX on failure
 */
int func_get(syntax *const sx, const size_t index);


/**
 *	Add new record to modes table
 *
 *	@param	sx			Syntax structure
 *	@param	record		Pointer to the new record
 *	@param	size		Size of the new record
 *
 *	@return	Index of the new record in modes table, @c SIZE_MAX on failure
 */
size_t mode_add(syntax *const sx, const int *const record, const size_t size);

/**
 *	Get an item from modes table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record
 *
 *	@return	Item by index from modes table, @c INT_MAX on failure
 */
int mode_get(syntax *const sx, const size_t index);

#ifdef __cplusplus
} /* extern "C" */
#endif
