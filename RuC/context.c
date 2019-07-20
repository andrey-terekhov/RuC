#include <string.h>
#include "global_vars.h"

void
ruc_context_init(ruc_context *context)
{
    memset(context, 0, sizeof(ruc_context));
    context->charnum = 1;
    context->m_charnum = 1;
    context->rp = 1;
    context->id = 2;
    context->md = 1;
    context->startmode = 1;
    context->sopnd = -1;
    context->curid = 2;
    context->lg = -1;
    context->displ = -3;
    context->maxdispl = 3;
    context->maxdisplg = 3;
    context->procd = 1;
    context->pc = 4;
    context->funcnum = 2;
    context->blockflag = 1;
    context->notrobot = 1;
    context->prdf = -1;
    context->leftansttype = -1;

    context->fip = 1;
    context->mfp = 1;
    context->mfirstrp = -1;
    context->mlastrp = -1;
    context->mp = 3;
}