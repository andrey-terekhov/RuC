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

/** Scanner description */
typedef struct scanner_desc
{
    ruc_io_source source; /** Data source supported by scanner */
    io_getnext_t  getnext; /** The pointer to a function scanning the
                               input from the context */
} scanner_desc;

/**
 * Initialize scanner
 *
 * @param opts Scanner context
 */
extern void scanner_init(universal_scanner_options *opts);

/**
 * Get a next symbol from input stream
 *
 * @param opts Scanner context
 *
 * @return Read symbol
 */
extern int scanner_getnext(universal_scanner_options *opts);

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
