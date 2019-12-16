#ifndef RUC_ERROR_H
#define RUC_ERROR_H

#include "context.h"

/**
 * Emit a warning for some problem
 *
 * @param context Compiler cocntext
 * @param errnum  Error number
 */
extern void warning(compiler_context *context, int errnum);

/**
 * Emit an error for some problem
 *
 * @param context Compiler cocntext
 * @param errnum  Error number
 */
extern void error(compiler_context *context, int errnum);

/**
 * Emit preprocessor error
 *
 * @param context Compiler conteext
 * @param errnum  Error number
 */
void m_error(compiler_context *context, int errnum);

#endif
