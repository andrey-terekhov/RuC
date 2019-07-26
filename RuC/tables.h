#ifndef RUC_TABLES_H
#define RUC_TABLES_H

/**
 * Save up a string array to reprtab
 *
 * @param context   RuC context
 * @param str       Target string
 *
 * @return FIXME
 */
extern int toreprtab(compiler_context *context, char str[]);

/**
 * Mode table initialization
 *
 * @param context   RuC context
 */
void init_modetab(compiler_context *context);

#endif
