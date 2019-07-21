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

struct ruc_vm_context;
typedef struct ruc_vm_context ruc_vm_context;

void t_init(ruc_vm_context *);
void t_destroy(ruc_vm_context *);

int t_create_inner(ruc_vm_context *, void *(*func)(void *), void *arg);
int t_create(ruc_vm_context *, void *(*func)(void *));

int t_createDetached(ruc_vm_context *, void *(*func)(void *));

void t_exit(ruc_vm_context *);
void t_join(ruc_vm_context *, int numTh);
int  t_getThNum(ruc_vm_context *);
void t_sleep(ruc_vm_context *, int miliseconds);

int  t_sem_create(ruc_vm_context *, int level);
void t_sem_wait(ruc_vm_context *, int numSem);
void t_sem_post(ruc_vm_context *, int numSem);

void            t_msg_send(ruc_vm_context *, struct msg_info msg);
struct msg_info t_msg_receive(ruc_vm_context *);

#endif
