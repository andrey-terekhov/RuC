#include "context.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "uniprinter.h"

void
vm_context_init(vm_context *context)
{
    memset(context, 0, sizeof(vm_context));

    scanner_init(&context->input_options);
    printer_init(&context->output_options);
    printer_init(&context->error_options);
    printer_init(&context->miscout_options);
    context->__countTh = 1;
}
