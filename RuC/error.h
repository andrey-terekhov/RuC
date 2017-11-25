#ifndef RUC_ERROR_H
#define RUC_ERROR_H

#include <stdlib.h>
#include <stdio.h>

extern void error(int ernum);

/** Print error and exit with given error code */
#define exit_err(errcode, str)  do {                                        \
                                    fprintf(stderr,                         \
                                            "%s: %s\n", __func__, str);     \
                                    exit(errcode);                          \
                                } while (0);

#endif
