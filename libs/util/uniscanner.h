/*
 *	Copyright 2019 Andrey Terekhov
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

#include "io.h"
#include "util_internal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Options of universal scanner */
typedef struct universal_scanner_options
{
	ruc_io_source source; /** Data source */
	FILE *input;		  /** Input file (in case of file source) */
	char *ptr;			  /** Data pointer (in case of memory
							  source) */
	int pos;			  /** Position to data within @p ptr */
	void *opaque;		  /** Scanner's opaque data */
} universal_scanner_options;

/**
 * Prototype for a function getting a next symbol from the context
 *
 * @param opts Scanner options
 *
 * @return Read symbol
 */
typedef int (*io_getnext_t)(universal_scanner_options *opts);

/**
 * Prototype for a scanf-like function
 *
 * @param opts Scanner options
 * @param fmt  Format string
 * @param args List of arguments
 *
 * @return scanf()-like result
 */
typedef int (*io_scanf_t)(universal_scanner_options *opts, const char *fmt, va_list args);

/** Scanner description */
typedef struct scanner_desc
{
	ruc_io_source source; /** Data source supported by scanner */
	io_getnext_t getnext; /** The pointer to a function scanning the
							  input from the context */
	io_scanf_t scanf;	  /** the pointer to a scanf-like function */
} scanner_desc;

/**
 * Initialize scanner
 *
 * @param opts Scanner context
 */
UTIL_EXPORTED void scanner_init(universal_scanner_options *opts);

/**
 * Deinitialize scanner
 *
 * @param opts Scanner context
 */
UTIL_EXPORTED void scanner_deinit(universal_scanner_options *opts);

/**
 * Close current scanner context
 *
 * @param opts Scanner context
 */
UTIL_EXPORTED void scanner_close(universal_scanner_options *opts);

/**
 * Get a next symbol from input stream
 *
 * @param opts Scanner context
 *
 * @return Read symbol
 */
UTIL_EXPORTED int scanner_getnext(universal_scanner_options *opts);

/**
 * scanf() for the uniscanner stream
 *
 * @param opts Scanner context
 *
 * @return scanf()-like return value
 */
UTIL_EXPORTED int scanner_scanf(universal_scanner_options *opts, const char *fmt, ...);

/**
 * Attach file to scanner
 *
 * @param opts  Universal scanner options
 * @param file  Target file
 *
 * @return @c true on success, @c false on failure
 */
UTIL_EXPORTED bool scanner_attach_file(universal_scanner_options *opts, FILE *file);

/**
 * Attach buffer to scanner
 *
 * @param opts  Universal scanner options
 * @param ptr   Target buffer
 *
 * @return @c true on success, @c false on failure
 */
UTIL_EXPORTED bool scanner_attach_buffer(universal_scanner_options *opts, const char *ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif
