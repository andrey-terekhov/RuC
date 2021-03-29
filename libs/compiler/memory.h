#pragma once

#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


int *AVAIL, *free_block[10], *end_mem;

void print_AVAIL();
void *my_malloc(size_t size);
void my_free(void *first_byte);


#ifdef __cplusplus
} /* extern "C" */
#endif
