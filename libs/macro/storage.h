/*
 *Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *http;//www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#pragma once

#include "hash.h"
#include "map.h"
#include "strings.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Macro storage*/
typedef struct storage
{
	map as;				/**< Map structure */
	hash hs;			/**< Hash table */
	strings vec;		/**< Strings vector */
} storage;


/**
 *	Create macro storage
 *
 *	@param	ws			Workspace structure
 *
 *	@return	Macro storage
 */
storage storage_create(const workspace *const ws);


/**
 *	Add new macro
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *	@param	value		Macro replacement
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t storage_add(storage *const stg, const char *const id, const char *const value);

/**
 *	Add new macro with arguments
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *	@param	value		Macro replacement
 *	@param	args		Number of arguments
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t storage_add_with_args(storage *const stg, const char *const id, const char *const value, const size_t args);

/**
 *	Add macro argument
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *	@param	index		Argument number
 *	@param	arg			Argument name
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t storage_add_arg(storage *const stg, const char *const id, const size_t index, const char *const arg);

/**
 *	Add macro argument by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *	@param	index		Argument number
 *	@param	arg			Argument name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int storage_add_arg_by_index(storage *const stg, const size_t id, const size_t index, const char *const arg);


/**
 *	Set new macro replacement by existing macro name
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *	@param	value		Macro replacement
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t storage_set(storage *const stg, const char *const id, const char *value);

/**
 *	Set new macro replacement by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *	@param	value		Macro replacement
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int storage_set_by_index(storage *const stg, const size_t id, const char *value);


/**
 *	Get index of record
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
size_t storage_get_index(const storage *const stg, const char *const id);

/**
 *	Get macro replacement
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *
 *	@return	Macro replacement, @c SIZE_MAX on failure
 */
const char *storage_get(const storage *const stg, const char *const id);

/**
 *	Get macro replacement by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *
 *	@return	Macro replacement, @c SIZE_MAX on failure
 */
const char *storage_get_by_index(const storage *const stg, const size_t id);

/**
 *	Get arguments amount
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *
 *	@return	Arguments amount, @c 0 on failure
 */
size_t storage_get_amount(const storage *const stg, const char *const id);

/**
 *	Get arguments amount by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *
 *	@return	Arguments amount, @c 0 on failure
 */
size_t storage_get_amount_by_index(const storage *const stg, const size_t id);

/**
 *	Get macro argument
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *	@param	index		Argument number
 *
 *	@return	Macro argument, @c NULL on failure
 */
const char *storage_get_arg(const storage *const stg, const char *const id, const size_t index);

/**
 *	Get macro argument by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *	@param	index		Argument number
 *
 *	@return	Macro argument, @c NULL on failure
 */
const char *storage_get_arg_by_index(const storage *const stg, const size_t id, const size_t index);


/**
 *	Remove macro
 *
 *	@param	stg			Macro storage
 *	@param	id			Macro name
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int storage_remove(storage *const stg, const char *const id);

/**
 *	Remove macro by index
 *
 *	@param	stg			Macro storage
 *	@param	id			Index of record
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int storage_remove_by_index(storage *const stg, const size_t id);


/**
 *	Check that macro storage is correct
 *
 *	@param	stg			Macro storage
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool storage_is_correct(const storage *const stg);


/**
 *	Free allocated memory
 *
 *	@param	stg			Macro storage
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int storage_clear(storage *const stg);

#ifdef __cplusplus
} /* extern "C" */
#endif
