#include "context.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "uniprinter.h"

/* See descriptino in context.h */
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

/* See descriptino in context.h */
void
vm_context_deinit(vm_context *context)
{
    scanner_deinit(&context->input_options);
    printer_deinit(&context->output_options);
    printer_deinit(&context->error_options);
    printer_deinit(&context->miscout_options);
}