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

#include "dll.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Structure for storing information about location */
typedef struct location
{
	universal_io *io;	/**< IO to search and produce marks */

	size_t path;		/**< Filename path IO position */
	size_t code;		/**< Current code line IO position */

	size_t line;		/**< Line number */
	size_t symbol;		/**< Symbol in line */
} location;


/**
 *	Create location statement and produce its mark to output
 *
 *	@param	io			Universal io structure
 *
 *	@return	Location statement
 */
EXPORTED location loc_create(universal_io *const io);

/**
 *	Create location statement for the macro replacement begin mark
 *
 *	@param	io			Universal io structure
 *
 *	@return	Location statement
 */
EXPORTED location loc_create_begin(universal_io *const io);

/**
 *	Create location statement for the macro replacement end mark
 *
 *	@param	io			Universal io structure
 *
 *	@return	Location statement
 */
EXPORTED location loc_create_end(universal_io *const io);


/**
 *	Update location and produce its mark to output
 *
 *	@param	loc			Location to update input position
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int loc_update(location *const loc);

/**
 *	Update location for the macro replacement begin mark
 *
 *	@param	loc			Location to update input position
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int loc_update_begin(location *const loc);

/**
 *	Update location for the macro replacement end mark
 *
 *	@param	loc			Location to update input position
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int loc_update_end(location *const loc);


/**
 *	Search location by input mark
 *
 *	@param	io			Universal io structure
 *
 *	@return	Location statement
 */
EXPORTED location loc_search(universal_io *const io);

/**
 *	Continue search from other location
 *
 *	@param	loc			Location to search from
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int loc_search_from(location *const loc);


/**
 *	Append line number when @c '\\n' character scanned
 *	@note Unsafe, only for unprocessed files
 *
 *	@param	loc			Location
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int loc_line_break(location *const loc);

/**
 *	Check that location is correct
 *
 *	@param	loc			Location
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool loc_is_correct(const location *const loc);


/**
 *	Get tag from comment
 *
 *	@param	loc			Location
 *	@param	buffer		Tag entry location
 *
 *	@return Size of tag
 */
EXPORTED size_t loc_get_tag(location *const loc, char *const buffer);

/**
 *	Get current code line
 *
 *	@param	loc			Location
 *	@param	buffer		Code line entry location
 *
 *	@return Size of code line
 */
EXPORTED size_t loc_get_code_line(location *const loc, char *const buffer);

/**
 *	Get current filename path
 *
 *	@param	loc			Location
 *	@param	buffer		Path entry location
 *
 *	@return Size of path
 */
EXPORTED size_t loc_get_path(location *const loc, char *const buffer);

/**
 *	Get normalized line number in code
 *
 *	@param	loc			Location
 *
 *	@return	Line number
 */
EXPORTED size_t loc_get_line(const location *const loc);

/**
 *	Get normalized position in line
 *
 *	@param	loc			Location
 *
 *	@return	Position in line
 */
EXPORTED size_t loc_get_symbol(const location *const loc);

#ifdef __cplusplus
} /* extern "C" */
#endif
