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

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "map.h"
#include "reporter.h"
#include "strings.h"
#include "tree.h"
#include "vector.h"


#define MAX_STRING_LENGTH 128


#ifdef __cplusplus
extern "C" {
#endif

/** Type qualifiers */
typedef enum TYPE
{
	TYPE_VARARG			= -9,
	TYPE_NULL_POINTER,
	TYPE_FILE,
	TYPE_VOID,
	TYPE_BOOLEAN		= -4,
	TYPE_FLOATING,
	TYPE_CHARACTER,
	TYPE_INTEGER,
	TYPE_UNDEFINED,

	TYPE_MSG_INFO 		= 2,
	TYPE_FUNCTION		= 1001,
	TYPE_STRUCTURE,
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_ENUM,

	BEGIN_USER_TYPE = 15,
} type_t;


/** Global vars definition */
typedef struct syntax
{
	universal_io *io;			/**< Universal io structure */
	reporter rprt;				/**< Reporter */

	strings string_literals;	/**< String literals list */

	vector predef;				/**< Predefined functions table */
	vector functions;			/**< Functions table */

	vector tree;				/**< Tree table */

	vector identifiers;			/**< Identifiers table */
	size_t cur_id;				/**< Start of current scope in identifiers table */

	vector types;				/**< Types table */
	size_t start_type;			/**< Start of last record in types table */

	map representations;		/**< Representations table */

	item_t max_displ;			/**< Max displacement */
	item_t max_displg;			/**< Max displacement */

	item_t displ;				/**< Stack displacement in current scope */
	item_t lg;					/**< Displacement from l (+1) or g (-1) */

	size_t ref_main;			/**< Main function reference */
} syntax;

/** Scope */
typedef struct scope
{
	item_t displ;
	item_t lg;
} scope;


/**
 *	Create Syntax structure
 *
 *	@param	ws				Compiler workspace
 *	@param	io				Universal io structure
 *
 *	@return	Syntax structure
 */
syntax sx_create(const workspace *const ws, universal_io *const io);

/**
 *	Check if syntax structure is correct
 *
 *	@param	sx				Syntax structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool sx_is_correct(syntax *const sx);

/**
 *	Free allocated memory
 *
 *	@param	sx				Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int sx_clear(syntax *const sx);


/**
 *	Add new dynamic UTF-8 string to string literal vector
 *
 *	@param	sx				Syntax structure
 *	@param	str				Dynamic UTF-8 string
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
size_t string_add(syntax *const sx, const vector *const str);

/**
 *	Get string
 *
 *	@param	sx				Syntax structure
 *	@param	index			Index
 *
 *	@return	String, @c NULL on failure
 */
const char* string_get(const syntax *const sx, const size_t index);

/**
 *	Get length of a string
 *
 *	@param  sx	  Syntax structure
 *
 *	@return Length of a string
 */
size_t strings_length(const syntax *const sx, const size_t index);

/**
 *	Get amount of strings
 *
 *	@param  sx	  Syntax structure
 *
 *	@return Amount of strings
 */
size_t strings_amount(const syntax *const sx);


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
 *	Reserve one element in functions table
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Previous functions size
 */
size_t func_reserve(syntax *const sx);


/**
 *	Add new item to identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	repr		New identifier index in representations table
 *	@param	kind		@c -1 for function as parameter,
 *						@c  0 for variable,
 *						@c  1 for label,
 *						@c  funcnum for function,
 *						@c  >= @c 100 for type specifier
 *	@param	type		New identifier type
 *	@param	func_def	@c 0 for function without args,
 *						@c 1 for function definition,
 *						@c 2 for function declaration,
 *						@c 3 for variables
 *
 *	@return	Index of the last item in identifiers table,
 *			@c SIZE_MAX @c - @c 1 on redeclaration
 *			@c SIZE_MAX on redefinition of main
 */
size_t ident_add(syntax *const sx, const size_t repr, const item_t kind, const item_t type, const int func_def);

/**
 *	Get index of previous declaration from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Index of previous declaration in identifiers table, @c SIZE_MAX on failure
 */
size_t ident_get_prev(const syntax *const sx, const size_t index);

/**
 *	Get item representation from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier index in representations table, @c ITEM_MAX on failure
 */
item_t ident_get_repr(const syntax *const sx, const size_t index);

/**
 *	Get item type from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier type, @c ITEM_MAX on failure
 */
item_t ident_get_type(const syntax *const sx, const size_t index);

/**
 *	Get item displacement from identifiers table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Identifier displacement, @c ITEM_MAX on failure
 */
item_t ident_get_displ(const syntax *const sx, const size_t index);

/**
 *	Get identifier spelling by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	Pointer to spelling of identifier
 */
const char *ident_get_spelling(const syntax *const sx, const size_t index);

/**
 *	Set identifier representation by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	repr		Index of record in representation table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_repr(syntax *const sx, const size_t index, const item_t repr);

/**
 *	Set identifier type by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	type		Identifier type
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_type(syntax *const sx, const size_t index, const item_t type);

/**
 *	Set identifier displacement by index in identifiers table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *	@param	displ		Identifier displacement
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int ident_set_displ(syntax *const sx, const size_t index, const item_t displ);

/**
 *	Check if identifier is declared as type specifier
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return	@c 0 on true, @c 0 on false
 */
bool ident_is_type_specifier(syntax *const sx, const size_t index);

/**
 *	Check if identifier is local by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in identifiers table
 *
 *	@return @c 1 on true, @c 0 on false
 */
bool ident_is_local(const syntax *const sx, const size_t index);


/**
 *	Add a new record to types table
 *
 *	@param	sx			Syntax structure
 *	@param	record		Pointer to the new record
 *	@param	size		Size of the new record
 *
 *	@return	New type, @c ITEM_MAX on failure
 */
item_t type_add(syntax *const sx, const item_t *const record, const size_t size);

/**
 *	Add a new enum fields to types table
 *
 *	@param	sx			Syntax structure
 *	@param	record		Pointer to the new record
 *	@param	size		Size of the new record
 *
 *	@return	New type, @c ITEM_MAX on failure
 */
item_t type_enum_add_fields(syntax *const sx, const item_t *const record, const size_t size);

/**
 *	Get type class
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type
 *
 *	@return	Type class
 */
type_t type_get_class(const syntax *const sx, const item_t type);

/**
 *	Get type size
 *
 *	@param	sx			Syntax structure
 *	@param	type		Standard type or index of the types table
 *
 *	@return	Type size
 */
size_t type_size(const syntax *const sx, const item_t type);

/**
 *	Check if type is boolean
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_boolean(const item_t type);

/**
 *	Check if type is integer
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_integer(const syntax *const sx, const item_t type);

/**
 *	Check if type is floating
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_floating(const item_t type);

/**
 *	Check if type is arithmetic
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_arithmetic(const syntax *const sx, const item_t type);

/**
 *	Check if type is void
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_void(const item_t type);

/**
 *	Check if type is null pointer
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_null_pointer(const item_t type);

/**
 *	Check if type is array
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_array(const syntax *const sx, const item_t type);

/**
 *	Check if type is structure
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_structure(const syntax *const sx, const item_t type);

/**
 *	Check if type is enum
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_enum(const syntax *const sx, const item_t type);

/**
 *	Check if type is enum field
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_enum_field(const syntax *const sx, const item_t type);

/**
 *	Check if type is function
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_function(const syntax *const sx, const item_t type);

/**
 *	Check if type is pointer
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_pointer(const syntax *const sx, const item_t type);

/**
 *	Check if type is scalar
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_scalar(const syntax *const sx, const item_t type);

/**
 *	Check if type is aggregate
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_aggregate(const syntax *const sx, const item_t type);

/**
 *	Check if type is string
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_string(const syntax *const sx, const item_t type);

/**
 *	Check if type is struct pointer
 *
 *	@param	sx			Syntax structure
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_struct_pointer(const syntax *const sx, const item_t type);

/**
 *	Check if type is undefined
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_undefined(const item_t type);

/**
 *	Check if type is FILE
 *
 *	@param	type		Type for check
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool type_is_file(const item_t type);

/**
 *	Get element type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Array type
 *
 *	@return	Element type, @c ITEM_MAX on failure
 */
item_t type_array_get_element_type(const syntax *const sx, const item_t type);

/**
 *	Create structure type
 *
 *	@param	sx			Syntax structure
 *	@param	types		Member types
 *	@param	names		Member names
 *
 *	@return	Structure type
 */
item_t type_structure(syntax *const sx, vector *const types, vector *const names);

/**
 *	Get member amount
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *
 *	@return	Member amount, @c SIZE_MAX on failure
 */
size_t type_structure_get_member_amount(const syntax *const sx, const item_t type);

/**
 *	Get member name by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *	@param	index		Member number
 *
 *	@return	Member name, @c SIZE_MAX on failure
 */
size_t type_structure_get_member_name(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get member type by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Structure type
 *	@param	index		Member number
 *
 *	@return	Member type, @c ITEM_MAX on failure
 */
item_t type_structure_get_member_type(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get return type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *
 *	@return	Return type, @c ITEM_MAX on failure
 */
item_t type_function_get_return_type(const syntax *const sx, const item_t type);

/**
 *	Get parameter amount
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *
 *	@return	Parameter amount, @c SIZE_MAX on failure
 */
size_t type_function_get_parameter_amount(const syntax *const sx, const item_t type);

/**
 *	Get parameter type by index
 *
 *	@param	sx			Syntax structure
 *	@param	type		Function type
 *	@param	index		Parameter number
 *
 *	@return	Parameter type, @c ITEM_MAX on failure
 */
item_t type_function_get_parameter_type(const syntax *const sx, const item_t type, const size_t index);

/**
 *	Get element type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Pointer type
 *
 *	@return	Element type, @c ITEM_MAX on failure
 */
item_t type_pointer_get_element_type(const syntax *const sx, const item_t type);

/**
 *	Create array type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Element type
 *
 *	@return	Array type
 */
item_t type_array(syntax *const sx, const item_t type);

/**
 *	Create string type
 *
 *	@param	sx			Syntax structure
 *
 *	@return	String type
 */
item_t type_string(syntax *const sx);

/**
 *	Get enum field type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Enum type
 *
 *	@return	Enum field type
 */
item_t get_enum_field_type(const syntax *const sx, const item_t type);

/**
 *	Create function type
 *
 *	@param	sx			Syntax structure
 *	@param	return_type	Return type
 *	@param	args		List of argument types
 *
 *	@return	Function type
 */
item_t type_function(syntax *const sx, const item_t return_type, const char *const args);

/**
 *	Create pointer type
 *
 *	@param	sx			Syntax structure
 *	@param	type		Referenced type
 *
 *	@return	Pointer type
 */
item_t type_pointer(syntax *const sx, const item_t type);


/**
 *	Add a new record from io to representations table or return existing
 *
 *	@param	sx			Syntax structure
 *	@param	last		Next character after key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t repr_reserve(syntax *const sx, char32_t *const last);

/**
 *	Get identifier name from representations table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *
 *	@return	Pointer to name of identifier
 */
const char *repr_get_name(const syntax *const sx, const size_t index);

/**
 *	Get a representation reference from table by index
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *
 *	@return	Reference by index from representations table, @c ITEM_MAX on failure
 */
item_t repr_get_reference(const syntax *const sx, const size_t index);

/**
 *	Set representation reference by index in table
 *
 *	@param	sx			Syntax structure
 *	@param	index		Index of record in representations table
 *	@param	ref			Reference to identifiers table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int repr_set_reference(syntax *const sx, const size_t index, const item_t ref);


/**
 *	Enter block scope
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Scope
 */
scope scope_block_enter(syntax *const sx);

/**
 *	Exit block scope
 *
 *	@param	sx			Syntax structure
 *	@param	scp			Scope
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int scope_block_exit(syntax *const sx, const scope scp);

/**
 *	Enter function scope
 *
 *	@param	sx			Syntax structure
 *
 *	@return	Previous stack displacement, @c ITEM_MAX on failure
 */
item_t scope_func_enter(syntax *const sx);

/**
 *	Exit function scope
 *
 *	@param	sx			Syntax structure
 *	@param	displ		Stack displacement at the start of the scope
 *
 *	@return	Max displacement of the function, @c ITEM_MAX on failure
 */
item_t scope_func_exit(syntax *const sx, const item_t displ);

#ifdef __cplusplus
} /* extern "C" */
#endif
