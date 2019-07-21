#include "context.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void
ruc_vm_context_init(ruc_vm_context *context)
{
    memset(context, 0, sizeof(ruc_vm_context));

    context->__countTh = 1;
}
