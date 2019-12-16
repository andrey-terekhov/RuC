#ifndef RUC_SCANNER_H
#define RUC_SCANNER_H

void        onemore(compiler_context *context);
extern int  scan(compiler_context *context);
extern int  getnext(compiler_context *context);
extern int  scaner(compiler_context *context);
extern void nextch(compiler_context *context);
extern int  letter(compiler_context *);
extern int  digit(compiler_context *);
extern int  equal(compiler_context *, int, int);

#endif