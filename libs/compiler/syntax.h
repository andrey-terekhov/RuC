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
#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct node node;

/** Global vars definition */
typedef struct syntax
{
	// memory & processes - usage here only for codes printing

	vector memory;					/**< Memory table */
	vector processes;				/**< Init processes table */

	vector predef;					/**< Predefined functions table */
	vector functions;				/**< Functions table */
	
	vector tree;					/**< Tree table */

	int identab[MAXIDENTAB];		/**< Identifiers table */
	size_t id;						/**< Number of identifiers */
	size_t curid;					/**< Start of current scope in identifiers table */

	int modetab[MAXMODETAB];		/**< Modes table */
	size_t md;						/**< Number of modes */
	size_t startmode;				/**< Start of last record in modetab */
	
	size_t hashtab[256];			/**< Hash table for reprtab */
	int hash;						/**< Last value of hash function */

	char32_t reprtab[MAXREPRTAB];	/**< Representations table */
	size_t rp;						/**< Representations size */

	int maxdispl;					/**< Max displacement */
	int maxdisplg;					/**< Max displacement */
	size_t main_ref;				/**< Main function reference */

	int displ;						/**< Stack displacement in current scope */
	int lg;							/**< Displacement from l (+1) or g (-1) */
	
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
 *	Check if syntax structure is correct
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int sx_is_correct(syntax *const sx);

/**
 *	Free allocated memory
 *
 *	@param	sx			Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int sx_clear(syntax *const sx);


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
 *	Add new value to memory table
 *
 *	@param	sx			Syntax structure
 *	@param	value		Value to record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int mem_add(syntax *const sx, const item_t value);

/**
 *	Set value by index in memory table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index to record
 *	@param	value		Value to record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int mem_set(syntax *const sx, const size_t index, const item_t value);

/**
 *	Get an item by index from memory table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in table
 *
 *	@return	Item by index from table, @c ITEM_MAX on failure
 */
item_t mem_get(const syntax *const sx, const size_t index);

/**
 *	Get size of memory table
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Program counter on success, @c SIZE_MAX on failure
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
int proc_set(syntax *const sx, const size_t index, const item_t value);

/**
 *	Get an item by index from init processes table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in table
 *
 *	@return	Item by index from table, @c ITEM_MAX on failure
 */
item_t proc_get(const syntax *const sx, const size_t index);


/**
 *	Add a new record to functions table
 *
 *	@param	sx			Syntax structure
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_add(syntax *const sx, const item_t ref);

/**
 *	Set function start reference by index in functions table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *	@param	ref			Start of function definition in syntax tree
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int func_set(syntax *const sx, const size_t index, const item_t ref);

/**
 *	Get an item from functions table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in functions table
 *
 *	@return	Item by index from functions table, @c ITEM_MAX on failure
 */
item_t func_get(const syntax *const sx, const size_t index);


/**
 *	Add new item to identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	repr		New identifier index in representations table
 *	@param	type		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	mode		New identifier mode
 *	@param	func_def	@c 0 for function without args,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for variables
 *
 *	@return	Index of the last item in identifiers table,
 *			@c SIZE_MAX @c - @c 1 on redeclaration
 *			@c SIZE_MAX on redefinition of main
 */
size_t ident_add(syntax *const sx, const size_t repr, const int type, const int mode, const int func_def);

/**
 *	Get item representation from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier index in representations table, @c INT_MAX on failure
 */
int ident_get_repr(const syntax *const sx, const size_t index);

/**
 *	Get item mode from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier mode, @c INT_MAX on failure
 */
int ident_get_mode(const syntax *const sx, const size_t index);

/**
 *	Get item displacement from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier displacement, @c INT_MAX on failure
 */
int ident_get_displ(const syntax *const sx, const size_t index);

/**
 *	Set identifier representation by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	repr		Index of record in representation table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_repr(syntax *const sx, const size_t index, const size_t repr);

/**
 *	Set identifier mode by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	mode		Identifier mode
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_mode(syntax *const sx, const size_t index, const int mode);

/**
 *	Set identifier displacement by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	displ		Identifier displacement
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_displ(syntax *const sx, const size_t index, const int displ);


/**
 *	Get mode size
 *	@note	Also used in codegen
 *
 *	@param	sx			Syntax structure
 *	@param	mode		Standart type or index for modes table
 *
 *	@return	Mode size
 */
int size_of(const syntax *const sx, const int mode);

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
 *	Enter block scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Variable to save previous stack displacement
 *	@param	lg			Variable to save previous value of lg
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_block_enter(syntax *const sx, int *const displ, int *const lg);

/**
 *	Exit block scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Stack displacement at the start of the scope
 *	@param	lg			Previous value of lg
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_block_exit(syntax *const sx, const int displ, const int lg);

/**
 *	Enter function scope
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Previous stack displacement, @c INT_MAX on failure
 */
int scope_func_enter(syntax *const sx);

/**
 *	Exit function scope
 *
 *	@param	sx			Syntax structure
 *	@param	decl_ref	Reference to function declaration in the tree
 *	@param	displ		Stack displacement at the start of the scope
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_func_exit(syntax *const sx, const size_t decl_ref, const int displ);

  
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
