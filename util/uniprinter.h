#ifndef RUC_UNIPRINTER_H
#define RUC_UNIPRINTER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "io.h"

/** Options of universal printer */
typedef struct universal_printer_options
{
    ruc_io_source source;
    FILE *        output;
    char *        ptr;
    int           pos;
    int           size;
    void *        opaque;
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
typedef int (*io_printf_t)(universal_printer_options *opts,
                           const char *               fmt,
                           va_list                    args);

/** Printer description */
typedef struct printer_desc
{
    ruc_io_source source; /** Data source supported by printer */
    io_printf_t   printf; /** The pointer to a function printing to data
                              destination */
} printer_desc;

/**
 * Initialize printer
 *
 * @param opts Printer context
 */
extern void printer_init(universal_printer_options *opts);

/**
 * Universal function for printing data to some output
 *
 * @param opts Universal printer options
 * @param fmt  String format
 *
 * @return printf-like return value
 */
extern int printer_printf(universal_printer_options *opts,
                          const char *               fmt,
                          ...);

/**
 * Universal function for printing (wide) characters
 *
 * @param opts  Universal printer options
 * @param wchar Symbol
 *
 * @return printf-like return value
 */
extern int printer_printchar(universal_printer_options *opts, int wchar);

/**
 * Attach file to printer
 *
 * @param opts  Universal printer options
 * @param file  Target file
 *
 * @return @c true on success, @c false on failure
 */
extern bool printer_attach_file(universal_printer_options *opts, FILE *file);

/**
 * Attach buffer to printer
 *
 * @param opts  Universal printer options
 * @param size  Buffer size
 *
 * @return @c true on success, @c false on failure
 */
extern bool printer_attach_buffer(universal_printer_options *opts, size_t size);

#endif
