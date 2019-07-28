#ifndef RUC_UNISCANNER_H
#define RUC_UNISCANNER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "io.h"

/** Options of universal scanner */
typedef struct universal_scanner_options
{
    ruc_io_source source; /** Data source */
    FILE *        input; /** Input file (in case of file source) */
    char *        ptr; /** Data pointer (in case of memory
                           source) */
    int   pos; /** Position to data within @p ptr */
    void *opaque; /** Scanner's opaque data */
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
typedef int (*io_scanf_t)(universal_scanner_options *opts,
                          const char *               fmt,
                          va_list                    args);

/** Scanner description */
typedef struct scanner_desc
{
    ruc_io_source source; /** Data source supported by scanner */
    io_getnext_t  getnext; /** The pointer to a function scanning the
                               input from the context */
    io_scanf_t scanf; /** the pointer to a scanf-like function */
} scanner_desc;

/**
 * Initialize scanner
 *
 * @param opts Scanner context
 */
extern void scanner_init(universal_scanner_options *opts);

/**
 * Deinitialize scanner
 *
 * @param opts Scanner context
 */
extern void scanner_deinit(universal_scanner_options *opts);

/**
 * Close current scanner context
 *
 * @param opts Scanner context
 */
extern void scanner_close(universal_scanner_options *opts);

/**
 * Get a next symbol from input stream
 *
 * @param opts Scanner context
 *
 * @return Read symbol
 */
extern int scanner_getnext(universal_scanner_options *opts);

/**
 * scanf() for the uniscanner stream
 *
 * @param opts Scanner context
 *
 * @return scanf()-like return value
 */
extern int scanner_scanf(universal_scanner_options *opts, const char *fmt, ...);

/**
 * Attach file to scanner
 *
 * @param opts  Universal scanner options
 * @param file  Target file
 *
 * @return @c true on success, @c false on failure
 */
extern bool scanner_attach_file(universal_scanner_options *opts, FILE *file);

/**
 * Attach buffer to scanner
 *
 * @param opts  Universal scanner options
 * @param ptr   Target buffer
 *
 * @return @c true on success, @c false on failure
 */
extern bool scanner_attach_buffer(universal_scanner_options *opts,
                                  const char *               ptr);

#endif
