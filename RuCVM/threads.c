//#include "stdafx.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "context.h"
#include "th_static.h"

#define TRUE 1
#define FALSE 0

#ifndef _REENTRANT
#define _REENTRANT
#endif

// sem_t context->__sems[__COUNT_SEM];


// void perror(const char *str);

void
t_init(vm_context *context)
{
    int res = pthread_rwlock_init(&context->__lock_t_create, NULL);
    if (res != 0)
    {
        perror(
            "t_init : pthread_rwlock_init of context->__lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    context->__threads[0].th = pthread_self();
    context->__threads[0].isDetach = TRUE;

    res = pthread_cond_init(&(context->__threads[0].cond), NULL);
    if (res != 0)
    {
        perror(
            "t_init : pthread_cond_init of context->__threads[0].cond failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_mutex_init(&(context->__threads[0].lock), NULL);
    if (res != 0)
    {
        perror(
            "t_init : pthread_mutex_init of context->__threads[0].lock failed");
        exit(EXIT_FAILURE);
    }
    res = pthread_rwlock_init(&context->__lock_t_sem_create, NULL);
    if (res != 0)
    {
        perror("t_init : pthread_rwlock_init of context->__lock_t_sem_create "
               "failed");
        exit(EXIT_FAILURE);
    }
}

int
__t_create(vm_context *    context,
           pthread_attr_t *attr,
           void *(*func)(void *),
           void *arg,
           int   isDetach)
{
    pthread_t th;
    int       retVal;
    int       res = pthread_rwlock_wrlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("__t_create : pthread_rwlock_wrlock of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }


    context->threadargs[context->__countTh].arg = arg;
    context->threadargs[context->__countTh].context = context;
    res = pthread_create(&th, attr, func,
                         &context->threadargs[context->__countTh]);
    if (res != 0)
    {
        perror("t_create : Thread creation failed");
        exit(EXIT_FAILURE);
    }

    if (attr)
    {
        res = pthread_attr_destroy(attr);
        if (res != 0)
        {
            perror("t_create : Thread attribute destroy failed");
            exit(EXIT_FAILURE);
        }
    }
    if (context->__countTh >= __COUNT_TH)
    {
        perror("t_create : Trying to create too much threads");
        exit(EXIT_FAILURE);
    }

    context->__threads[context->__countTh].th = th;
    context->__threads[context->__countTh].isDetach = isDetach;

    res =
        pthread_cond_init(&(context->__threads[context->__countTh].cond), NULL);
    if (res != 0)
    {
        perror("__t_create : pthread_cond_init of "
               "context->__threads[context->__countTh].cond "
               "failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_mutex_init(&(context->__threads[context->__countTh].lock),
                             NULL);
    if (res != 0)
    {
        perror("__t_create : pthread_mutex_init of "
               "context->__threads[context->__countTh].lock "
               "failed");
        exit(EXIT_FAILURE);
    }

    context->__threads[context->__countTh].countMsg = 0;
    retVal = context->__countTh++;

    res = pthread_rwlock_unlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("__t_create : pthread_rwlock_unlock of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }
    return retVal;
}

int
t_create_inner(vm_context *context, void *(*func)(void *), void *arg)
{
    return __t_create(context, NULL, func, arg, FALSE);
}

int
t_createDetached(vm_context *context, void *(*func)(void *))
{
    pthread_attr_t attr;

    int res = pthread_attr_init(&attr);
    if (res != 0)
    {
        perror("t_createDetached : Attribute creation failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (res != 0)
    {
        perror("t_createDetached : Setting detached attribute failed");
        exit(EXIT_FAILURE);
    }

    return __t_create(context, &attr, func, NULL, TRUE);
}

void
t_exit(vm_context *context)
{
    UNUSED(context);
    pthread_exit(NULL);
}

void
t_join(vm_context *context, int numTh)
{
    int res = pthread_rwlock_rdlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("t_join : pthread_rwlock_rdlock of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }

    if (numTh > 0 && numTh < context->__countTh)
    {
        if (!context->__threads[numTh].isDetach)
        {
            pthread_t th = context->__threads[numTh].th;

            res = pthread_rwlock_unlock(&context->__lock_t_create);
            if (res != 0)
            {
                perror("t_join : pthread_rwlock_unlock of "
                       "context->__lock_t_create failed");
                exit(EXIT_FAILURE);
            }

            res = pthread_join(th, NULL);
            if (res != 0)
            {
                perror("t_join : Thread join failed");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            perror("t_join : Thread join failed - thread is detached");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror(
            "t_join : Thread join failed - number of thread is out of range");
        exit(EXIT_FAILURE);
    }
}

int
t_getThNum(vm_context *context)
{
    pthread_t th;
    int       index, i;
    int       res = pthread_rwlock_rdlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("t_getThNum : pthread_rwlock_rdlock of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }

    th = pthread_self();
    index = -1;

    for (i = 0; i < context->__countTh; i++)
    {
        if (pthread_equal(th, context->__threads[i].th))
        {
            index = i;
            break;
        }
    }

    if (index != -1)
    {
        res = pthread_rwlock_unlock(&context->__lock_t_create);
        if (res != 0)
        {
            perror("t_getThNum : pthread_rwlock_unlock of "
                   "context->__lock_t_create failed");
            exit(EXIT_FAILURE);
        }

        return index;
    }
    perror("t_getThNum : Thread is not registered");
    exit(EXIT_FAILURE);
}

void
t_sleep(vm_context *context, int miliseconds)
{
    UNUSED(context);

    // Sleep(seconds * 1000);
    usleep(miliseconds * 1000);
}

int
t_sem_create(vm_context *context, int level)
{
    int    res = pthread_rwlock_wrlock(&context->__lock_t_sem_create), retVal;
    sem_t *sem;
    char   csem[10];
    if (res != 0)
    {
        perror("t_sem_create : pthread_rwlock_wrlock of "
               "context->__lock_t_sem_create "
               "failed");
        exit(EXIT_FAILURE);
    }
    sprintf(csem, "%d", context->__countSem);
    sem_unlink(csem);
    sem = sem_open(csem, O_CREAT, S_IRUSR | S_IWUSR, level);
    if (sem == SEM_FAILED)
    {
        perror("t_sem_create : Semaphore initilization failed");
        exit(EXIT_FAILURE);
    }
    if (context->__countSem >= __COUNT_SEM)
    {
        perror("t_create : Trying to create too much semaphores");
        exit(EXIT_FAILURE);
    }
    context->__sems[context->__countSem] = sem;
    retVal = context->__countSem++;

    res = pthread_rwlock_unlock(&context->__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_create : pthread_rwlock_unlock of "
               "context->__lock_t_sem_create "
               "failed");
        exit(EXIT_FAILURE);
    }
    return retVal;
}

void
t_sem_wait(vm_context *context, int numSem)
{
    int res = pthread_rwlock_rdlock(&context->__lock_t_sem_create);
    //    printf("t_sem_wait numSem= %i context->__countSem=  %i\n", numSem,
    //    context->__countSem);
    if (res != 0)
    {
        perror("t_sem_wait : pthread_rwlock_rdlock of "
               "context->__lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    if (numSem >= 0 && numSem < context->__countSem)
    {
        sem_t *sem = context->__sems[numSem];

        res = pthread_rwlock_unlock(&context->__lock_t_sem_create);
        if (res != 0)
        {
            perror("t_sem_wait : pthread_rwlock_unlock of "
                   "context->__lock_t_sem_create "
                   "failed");
            exit(EXIT_FAILURE);
        }
        res = sem_wait(sem);
        if (res != 0)
        {
            perror("t_sem_wait : Semaphore wait failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror("t_sem_wait : Semaphore wait failed - semaphore number is out "
               "of range");
        exit(EXIT_FAILURE);
    }
}

void
t_sem_post(vm_context *context, int numSem)
{
    int res = pthread_rwlock_rdlock(&context->__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_post : pthread_rwlock_rdlock of "
               "context->__lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    if (numSem >= 0 && numSem < context->__countSem)
    {
        sem_t *sem = context->__sems[numSem];
        res = pthread_rwlock_unlock(&context->__lock_t_sem_create);
        if (res != 0)
        {
            perror("t_sem_post : pthread_rwlock_unlock of "
                   "context->__lock_t_sem_create "
                   "failed");
            exit(EXIT_FAILURE);
        }

        res = sem_post(sem);
        if (res != 0)
        {
            perror("t_sem_post : Semaphore post failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror("t_sem_post : Semaphore post failed - number of semaphore is "
               "out of range");
        exit(EXIT_FAILURE);
    }
}

void
t_msg_send(vm_context *context, struct msg_info msg)
{
    int res = pthread_rwlock_rdlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("t_msg_send : pthread_rwlock_rdlock of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }
    if (msg.numTh >= 0 && msg.numTh < context->__countTh)
    {
        struct ruc_thread_info *th_info = &(context->__threads[msg.numTh]);

        res = pthread_rwlock_unlock(&context->__lock_t_create);
        if (res != 0)
        {
            perror("t_msg_send : pthread_rwlock_unlock of "
                   "context->__lock_t_create failed");
            exit(EXIT_FAILURE);
        }
        res = pthread_mutex_lock(&(th_info->lock));
        if (res != 0)
        {
            perror("t_msg_send : pthread_mutex_lock of th_info.lock failed");
            exit(EXIT_FAILURE);
        }
        if (th_info->countMsg >= __COUNT_MSGS_FOR_TH)
        {
            perror("t_msg_send : Trying to send too much messages");
            exit(EXIT_FAILURE);
        }
        th_info->msgs[th_info->countMsg].numTh = t_getThNum(context);
        th_info->msgs[th_info->countMsg++].data = msg.data;

        if (th_info->countMsg == 1)
        {
            res = pthread_cond_signal(&(th_info->cond));
            if (res != 0)
            {
                perror(
                    "t_msg_send : pthread_cond_signal of th_info.cond failed");
                exit(EXIT_FAILURE);
            }
        }
        res = pthread_mutex_unlock(&(th_info->lock));
        if (res != 0)
        {
            perror("t_msg_send : pthread_mutex_unlock of th_info.lock failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror("t_msg_send : Message send failed - number of thread is out of "
               "range");
        exit(EXIT_FAILURE);
    }
}
struct msg_info
t_msg_receive(vm_context *context)
{
    int res = pthread_rwlock_rdlock(&context->__lock_t_create), numTh;
    struct msg_info msg;
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_rwlock_rdlock of "
               "context->__lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    numTh = t_getThNum(context);
    struct ruc_thread_info *th_info = &(context->__threads[numTh]);

    res = pthread_rwlock_unlock(&context->__lock_t_create);
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_rwlock_unlock of "
               "context->__lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    res = pthread_mutex_lock(&(th_info->lock));
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_mutex_lock of th_info.lock failed");
        exit(EXIT_FAILURE);
    }
    if (th_info->countMsg == 0)
    {
        res = pthread_cond_wait(&(th_info->cond), &(th_info->lock));
        if (res != 0)
        {
            perror("t_msg_recieve : pthread_cond_wait of th_info.cond and "
                   "th_info.lock failed");
            exit(EXIT_FAILURE);
        }
    }
    msg = th_info->msgs[--th_info->countMsg];
    res = pthread_mutex_unlock(&(th_info->lock));
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_mutex_unlock of th_info.lock failed");
        exit(EXIT_FAILURE);
    }
    return msg;
}

void
t_destroy(vm_context *context)
{
    int res, i;

    for (i = 0; i < context->__countTh; i++)
    {
        res = pthread_cond_destroy(&(context->__threads[i].cond));
        if (res != 0)
        {
            perror("t_destroy : pthread_cond_destroy of "
                   "context->__threads[i].cond failed");
            exit(EXIT_FAILURE);
        }
        res = pthread_mutex_destroy(&(context->__threads[i].lock));
        if (res != 0)
        {
            perror("t_destroy : pthread_mutex_destroy of "
                   "context->__threads[i].lock "
                   "failed");
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < context->__countSem; i++)
    {
        //        res = sem_destroy(&(context->__sems[i]));
        res = sem_close(context->__sems[i]);
        if (res != 0)
        {
            perror("t_destroy : sem_destroy of context->__sems[i] failed");
            exit(EXIT_FAILURE);
        }
    }
    res = pthread_rwlock_destroy(&context->__lock_t_create);
    if (res != 0)
    {
        perror("t_destroy : pthread_rwlock_destroy of context->__lock_t_create "
               "failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_rwlock_destroy(&context->__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_destroy : pthread_rwlock_destroy of "
               "context->__lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
}
