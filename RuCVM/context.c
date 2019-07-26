#include "context.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "uniprinter.h"

void
ruc_vm_context_init(ruc_vm_context *context)
{
    memset(context, 0, sizeof(ruc_vm_context));

    scanner_init(&context->input_options);
    printer_init(&context->output_options);
    printer_init(&context->error_options);
    printer_init(&context->miscout_options);
    context->__countTh = 1;
}
