#ifndef RUC_VM_TH_STATIC_H
#define RUC_VM_TH_STATIC_H

#pragma region CONSTANTS
#define __COUNT_TH 10
#define __COUNT_SEM 16
#define __COUNT_MSGS_FOR_TH 4
#pragma endregion

#include <fcntl.h>

struct msg_info
{
    int numTh;
    int data;
};

typedef struct ruc_thread_info
{
    pthread_t th;
    int       isDetach;

    pthread_cond_t  cond;
    pthread_mutex_t lock;
    struct msg_info msgs[__COUNT_MSGS_FOR_TH];
    int             countMsg;
} ruc_thread_info;


#include "context.h"

struct vm_context;
typedef struct vm_context vm_context;

void t_init(vm_context *);
void t_destroy(vm_context *);

int t_create_inner(vm_context *, void *(*func)(void *), void *arg);
int t_create(vm_context *, void *(*func)(void *));

int t_createDetached(vm_context *, void *(*func)(void *));

void t_exit(vm_context *);
void t_join(vm_context *, int numTh);
int  t_getThNum(vm_context *);
void t_sleep(vm_context *, int miliseconds);

int  t_sem_create(vm_context *, int level);
void t_sem_wait(vm_context *, int numSem);
void t_sem_post(vm_context *, int numSem);

void            t_msg_send(vm_context *, struct msg_info msg);
struct msg_info t_msg_receive(vm_context *);

#endif
