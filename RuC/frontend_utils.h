#ifndef RUC_FRONTEND_UTILS_H
#define RUC_FRONTEND_UTILS_H

/**
 * Read keywords from input context
 *
 * @param context   RuC context
 */
void read_keywords(ruc_context *context);

/**
 * Print tables and tree to a file specified by @p path
 *
 * @param context   RuC context
 * @param path      Target file
 */
void output_tables_and_tree(ruc_context *context, const char *path);

/**
 * Print codes to a file specified by @p path
 *
 * @param context   RuC context
 * @param path      Target file
 */
void output_codes(ruc_context *context, const char *path);

/**
 * Print export tables to a file specified by @p path
 *
 * @param context   RuC context
 * @param path      Target file
 */
void output_export(ruc_context *context, const char *path);

#endif