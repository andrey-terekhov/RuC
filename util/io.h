#ifndef RUC_IO_H
#define RUC_IO_H

/**
 * Input/Output pipe type
 */
typedef enum ruc_io_type
{
    IO_TYPE_INPUT, /** Input pipe */
    IO_TYPE_OUTPUT, /** Output pipe */
    IO_TYPE_ERROR, /** Error pipe */
    IO_TYPE_MISC, /** Misc output pipe */
} ruc_io_type;

typedef enum ruc_io_source
{
    IO_SOURCE_FILE, /** File-based input/output */
    IO_SOURCE_MEM, /** Buffer-based input/output */
} ruc_io_source;

#endif
