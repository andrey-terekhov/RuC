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
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct node node;

/** Global vars definition */
typedef struct syntax
{
	// mem, pc & iniprocs - usage here only for codes printing

	int mem[MAXMEMSIZE];			/**< Memory */
	int pc;							/**< Program counter */

	int iniprocs[INIPROSIZE];		/**< Init processes */
	int procd;						/**< Process management daemon */

	int functions[FUNCSIZE];		/**< Functions table */
	int funcnum;					/**< Number of functions */

	int identab[MAXIDENTAB];		/**< Identifiers table */
	int id;							/**< Number of identifiers */

	int modetab[MAXMODETAB];		/**< Modes table */
	int md;							/**< Number of modes */
	int startmode;					/**< Start of last record in modetab */
	
	int tree[MAXTREESIZE];			/**< Tree */
	int tc;							/**< Tree counter */
	
	size_t hashtab[256];			/**< Hash table for reprtab */
	int hash;						/**< Last value of hash function */

	char32_t reprtab[MAXREPRTAB];	/**< Representations table */
	size_t rp;						/**< Representations size */

	int maxdisplg;					/**< Max displacement */
	int wasmain;					/**< Main function flag */

	int anstdispl;					/**< Stack displacement */
	int keywordsnum;				/**< Number of read keyword */

	node *current; 					/**< Current node during traversing the tree */
} syntax;


/**
 *	Init Syntax structure
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int sx_init(syntax *const sx);


/**
 *	Increase size of memory table by value
 *
 *	@param	sx			Syntax structure
 *	@param	value		Value to increase
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int mem_increase(syntax *const sx, const size_t value);

/**
 *	Add new record to memory table
 *
 *	@param	sx			Syntax structure
 *	@param	value		Value to record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int mem_add(syntax *const sx, const int value);

/**
 *	Set value by index in memory table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index to record
 *	@param	value		Value to record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int mem_set(syntax *const sx, const size_t index, const int value);

/**
 *	Get an item by index from memory table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in table
 *
 *	@return	Item by index from table, @c INT_MAX on failure
 */
int mem_get(const syntax *const sx, const size_t index);

/**
 *	Get size of memory table
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Program counter on success, @c INT_MAX on failure
 */
size_t mem_get_size(const syntax *const sx);


/**
 *	Set value by index in init processes table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index to record
 *	@param	value		Value to record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int proc_set(syntax *const sx, const size_t index, const int value);

/**
 *	Get an item by index from init processes table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in table
 *
 *	@return	Item by index from table, @c INT_MAX on failure
 */
int proc_get(const syntax *const sx, const size_t index);


/**
 *	Add a new record to functions table
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
int func_get(const syntax *const sx, const size_t index);



/**
 *	Add a new record to modes table
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
 *	@param	index		Index of record in modes table
 *
 *	@return	Item by index from modes table, @c INT_MAX on failure
 */
int mode_get(const syntax *const sx, const size_t index);


/**
 *	Add a new record to representations table
 *
 *	@param	sx			Syntax structure
 *	@param	spelling	Spelling of new identifier or keyword
 *
 *	@return	Index of the new record in representations table, @c SIZE_MAX on failure
 */
size_t repr_add(syntax *const sx, const char32_t *const spelling);

/**
 *	Get a representation spelling from table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *	@param	spelling	String for result
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int repr_get_spelling(const syntax *const sx, const size_t index, char32_t *const spelling);

/**
 *	Get a representation reference from table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *
 *	@return	Reference by index from representations table, @c INT_MAX on failure
 */
int repr_get_reference(const syntax *const sx, const size_t index);

/**
 *	Set representation reference by index in table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *	@param	ref			Reference to identifiers table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int repr_set_reference(syntax *const sx, const size_t index, const size_t ref);


/**
 *	Set current node
 *
 *	@param	sx			Syntax structure
 *	@param	nd			Node to set
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_set_node(syntax *const sx, node *const nd);

/**
 *	Set next node in current node
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c -1 on failure, 
 *			@c  0 on success,
 *			@c  1 on the end of the tree
 */
int tree_next_node(syntax *const sx);

/**
 *	Get current node
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Current node, @c NULL on failure
 */
node *tree_get_node(syntax *const sx);

#ifdef __cplusplus
} /* extern "C" */
#endif
