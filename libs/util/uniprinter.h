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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Options of universal printer */
typedef struct universal_printer_options
{
	ruc_io_source source;
	FILE *output;
	char *ptr;
	size_t pos;
	size_t size;
	void *opaque;
} universal_printer_options;

/**
 * Prototype for a function printing to a context
 *
 * @param opts      Printer options
 * @param fmt       String format
 * @param args      Variadic argument list
 *
 * @return printf()-like return value
 */
typedef int (*io_printf_t)(universal_printer_options *opts, const char *fmt, va_list args);

/** Printer description */
typedef struct printer_desc
{
	ruc_io_source source; /** Data source supported by printer */
	io_printf_t printf;	  /** The pointer to a function printing to data
							  destination */
} printer_desc;

/**
 * Initialize printer
 *
 * @param opts Printer context
 */
void printer_init(universal_printer_options *opts);

/**
 * Deinitialize printer
 *
 * @param opts Printer context
 */
void printer_deinit(universal_printer_options *opts);

/**
 * Close current printer stream
 *
 * @param opts Printer context
 */
void printer_close(universal_printer_options *opts);

/**
 * Universal function for printing data to some output
 *
 * @param opts Universal printer options
 * @param fmt  String format
 *
 * @return printf-like return value
 */
int printer_printf(universal_printer_options *opts, const char *fmt, ...);

/**
 * Universal function for printing (wide) characters
 *
 * @param opts  Universal printer options
 * @param wchar Symbol
 *
 * @return printf-like return value
 */
int printer_printchar(universal_printer_options *opts, int wchar);

/**
 * Attach file to printer
 *
 * @param opts  Universal printer options
 * @param file  Target file
 *
 * @return @c true on success, @c false on failure
 */
bool printer_attach_file(universal_printer_options *opts, FILE *file);

/**
 * Attach buffer to printer
 *
 * @param opts  Universal printer options
 * @param size  Buffer size
 *
 * @return @c true on success, @c false on failure
 */
bool printer_attach_buffer(universal_printer_options *opts, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif
