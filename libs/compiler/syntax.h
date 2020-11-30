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


#define COMPILER_TABLE_SIZE_DEFAULT	 (100)
#define COMPILER_TABLE_INCREMENT_MIN (100)


#ifdef __cplusplus
extern "C" {
#endif

/** A designated compiler table */
typedef struct compiler_table
{
	int *table; /** Actual table */
	int len;	/** Length of a useful part of table */
	int pos;	/** A position in a table */
	int size;	/** Total size of a table */
} compiler_table;

/** Global vars definition */
typedef struct syntax
{
	// mem, pc & iniprocs - usage here only for codes printing

	int mem[MAXMEMSIZE];		/** Memory */
	int pc;						/** Program counter */

	int iniprocs[INIPROSIZE];	/** Init processes */
	int procd;					/** Process management daemon */

	int functions[FUNCSIZE];	/** Functions table */
	int funcnum;				/** Number of functions */

	int identab[MAXIDENTAB];	/** Identifiers table */
	int id;						/** Number of identifiers */

	int modetab[MAXMODETAB];	/** Modes table */
	int md;						/** Number of modes */
	
	int tree[MAXTREESIZE];		/** Tree */
	int tc;						/** Tree counter */

	int maxdisplg;				/** Max displacement */
	int wasmain;				/** Main function flag */

	int anstdispl;				/** Stack displacement */

	compiler_table reprtab;		/** Representations table */
} syntax;


/**
 *	Initialize syntax structure (by allocating memory)
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int syntax_init(syntax *const sx);

/**
 *	Deinitialize syntax structure (by free memory)
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int syntax_deinit(syntax *const sx);


/**
 *	Initialize compiler table
 *
 *	@param	table	Target compiler table
 */
void compiler_table_init(compiler_table *table);

/**
 *	Ensure that specific offset is allocated in a table
 *
 *	@param	table	Target compiler table
 *	@param	pos		Target position
 *
 *	@return	Table size
 */
int compiler_table_ensure_allocated(compiler_table *table, int pos);

/**
 *	Expand compiler table
 *
 *	@param	table	Target compiler table
 *	@param	len		Requested length
 *
 *	@return	New size
 */
int compiler_table_expand(compiler_table *table, int len);

#ifdef __cplusplus
} /* extern "C" */
#endif
