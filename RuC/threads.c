//
//  threads.c
//  RuC
//
//  Created by Andrey Terekhov on 10/08/2017.
//  Copyright © 2017 SE chair. All rights reserved.
//

#include "threads.h"

// void* t_seize(void* (*func)(void *), void *arg);  // пока не буду

int __countTh = 1;
int __countThContainer = __COUNT_TH_CONTAINER_DEFAULT;

struct __msgList
{
    void *msg;
    struct __msgList *next;
};
struct __threadInfo
{
    pthread_t th;
    int isDetach;
    
    pthread_cond_t cond;
    pthread_mutex_t lock;
    struct __msgList *head;
    struct __msgList *tail;
};
struct __threadInfo *__threads;
int __countSem = 0;
int __countSemContainer = __COUNT_SEM_CONTAINER_DEFAULT;

sem_t *__sems;
pthread_rwlock_t __lock_t_create;
pthread_rwlock_t __lock_t_sem_create;
pthread_mutex_t __lock_t_seize;
int t_initAll()
{
    __threads = (struct __threadInfo *)malloc(__COUNT_TH_CONTAINER_DEFAULT * sizeof *__threads);
    if (!__threads)
    {
        perror("t_initAll : Threads contrainer memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    __sems = (sem_t *)malloc(__COUNT_SEM_CONTAINER_DEFAULT * sizeof *__sems);
    if (!__sems)
    {
        perror("t_initAll : Semaphores contrainer memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    int res = pthread_rwlock_init(&__lock_t_create, NULL);
    if (res != 0)
    {
        perror("t_initAll : pthread_rwlock_init of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    __threads[0].th = pthread_self();
    __threads[0].isDetach = TRUE;
    
    res = pthread_cond_init(&(__threads[0].cond), NULL);
    if (res != 0)
    {
        perror("t_initAll : pthread_cond_init of __threads[0].cond failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutex_init(&(__threads[0].lock), NULL);
    if (res != 0)
    {
        perror("t_initAll : pthread_mutex_init of __threads[0].lock failed");
        exit(EXIT_FAILURE);
    }
    
    __threads[0].head = NULL;
    __threads[0].tail = NULL;
    res = pthread_rwlock_init(&__lock_t_sem_create, NULL);
    if (res != 0)
    {
        perror("t_initAll : pthread_rwlock_init of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    pthread_mutexattr_t seize_attr;
    
    res = pthread_mutexattr_init(&seize_attr);
    if (res != 0)
    {
        perror("t_initAll : pthread_mutexattr_init of seize_attr failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutexattr_settype(&seize_attr, PTHREAD_MUTEX_RECURSIVE);
    if (res != 0)
    {
        perror("t_initAll : pthread_mutexattr_settype of seize_attr failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutex_init(&__lock_t_seize, &seize_attr);
    if (res != 0)
    {
        perror("t_initAll : pthread_mutex_init of __lock_t_seize and seize_attr failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutexattr_destroy(&seize_attr);
    if (res != 0)
    {
        perror("t_initAll : pthread_mutexattr_destroy of seize_attr failed");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}

void t_destroyAll();
int __t_create(pthread_attr_t *attr, void(*func)(int), int arg, int isDetach)
{
    int res = pthread_rwlock_wrlock(&__lock_t_create);
    if (res != 0)
    {
        perror("__t_create : pthread_rwlock_wrlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    pthread_t th;
    
    res = pthread_create(&th, attr, func, (void*) arg);
    if (res != 0)
    {
        perror("t_create : Thread creation failed");
        exit(EXIT_FAILURE);
    }
    
    if (attr)
    {
        res = pthread_attr_destroy(attr);
        if(res != 0)
        {
            perror("t_create : Thread attribute destroy failed");
            exit(EXIT_FAILURE);
        }
    }
    if (__countTh >= __countThContainer)
    {
        struct __threadInfo * treadsEx = (struct __threadInfo *)realloc(__threads, 2 * __countThContainer * sizeof *__threads);
        if (treadsEx)
        {
            __countThContainer *= 2;
            __threads = treadsEx;
        }
        else
        {
            free(__threads);
            
            perror("t_create : Threads contrainer memory reallocation failed");
            exit(EXIT_FAILURE);
        }
    }
    
    __threads[__countTh].th = th;
    __threads[__countTh].isDetach = isDetach;
    
    res = pthread_cond_init(&(__threads[__countTh].cond), NULL);
    if (res != 0)
    {
        perror("__t_create : pthread_cond_init of __threads[__countTh].cond failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutex_init(&(__threads[__countTh].lock), NULL);
    if (res != 0)
    {
        perror("__t_create : pthread_mutex_init of __threads[__countTh].lock failed");
        exit(EXIT_FAILURE);
    }
    
    __threads[__countTh].head = NULL;
    __threads[__countTh].tail = NULL;
    
    int retVal = __countTh++;
    
    res = pthread_rwlock_unlock(&__lock_t_create);
    if (res != 0)
    {
        perror("__t_create : pthread_rwlock_unlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    
    return retVal;
}
int t_create(void(*func)(int), int arg)
{
    return __t_create(NULL, func, arg, FALSE);
}
int t_createDetached(void* (*func)(void *), void *arg)
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
    
    return __t_create(&attr, func, arg, TRUE);
}

void t_exit()
{
    pthread_exit(NULL);
}
void t_join(int numTh)
{
    int res = pthread_rwlock_rdlock(&__lock_t_create);
    if (res != 0)
    {
        perror("t_join : pthread_rwlock_rdlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    if (numTh > 0 && numTh < __countTh)
    {
        if (!__threads[numTh].isDetach)
        {
            pthread_t th = __threads[numTh].th;
            
            res = pthread_rwlock_unlock(&__lock_t_create);
            if (res != 0)
            {
                perror("t_join : pthread_rwlock_unlock of __lock_t_create failed");
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
        perror("t_join : Thread join failed - number of thread is out of range");
        exit(EXIT_FAILURE);
    }
}

int t_getThNum()
{
    int res = pthread_rwlock_rdlock(&__lock_t_create);
    if (res != 0)
    {
        perror("t_getThNum : pthread_rwlock_rdlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    
    pthread_t th = pthread_self();
    int index = -1;
    
    for (int i = 0; i < __countTh; i++)
    {
        if (pthread_equal(th, __threads[i].th))
        {
            index = i;
        }
    }
    
    if (index != -1)
    {
        res = pthread_rwlock_unlock(&__lock_t_create);
        if (res != 0)
        {
            perror("t_getThNum : pthread_rwlock_unlock of __lock_t_create failed");
            exit(EXIT_FAILURE);
        }
        
        return index;
    }
    
    perror("t_getThNum : Thread is not registered");
    exit(EXIT_FAILURE);
}

void t_sleep(int seconds)
{
    sleep(seconds);       // я хочу miliseconds ?
}

int t_sem_create(int level)
{
    int res = pthread_rwlock_wrlock(&__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_create : pthread_rwlock_wrlock of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    sem_t sem;
    
    res = sem_init(&sem, 0, level);
    if (res != 0)
    {
        perror("t_sem_create : Semaphore initilization failed");
        exit(EXIT_FAILURE);
    }
    if (__countSem >= __countSemContainer)
    {
        sem_t * semsEx = (sem_t *)realloc(__sems, 2 * __countSemContainer * sizeof *__sems);
        if (semsEx)
        {
            __countSemContainer *= 2;
            __sems = semsEx;
        }
        else
        {
            free(__sems);
            
            perror("t_sem_create : Semafors contrainer memory reallocation failed");
            exit(EXIT_FAILURE);
        }
    }
    __sems[__countSem] = sem;
    
    int retVal = __countSem++;
    
    res = pthread_rwlock_unlock(&__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_create : pthread_rwlock_unlock of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    
    return retVal;
}

void t_sem_wait(int numSem)
{
    int res = pthread_rwlock_rdlock(&__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_wait : pthread_rwlock_rdlock of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    
    if (numSem >= 0 && numSem < __countSem)
    {
        sem_t sem = __sems[numSem];
        
        res = pthread_rwlock_unlock(&__lock_t_sem_create);
        if (res != 0)
        {
            perror("t_sem_wait : pthread_rwlock_unlock of __lock_t_sem_create failed");
            exit(EXIT_FAILURE);
        }
        
        int res = sem_wait(&sem);
        if (res != 0)
        {
            perror("t_sem_wait : Semaphore wait failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror("t_sem_wait : Semaphore wait failed - number of semaphore is out of range");
        exit(EXIT_FAILURE);
    }
}

void t_sem_post(int numSem)
{
    int res = pthread_rwlock_rdlock(&__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_sem_post : pthread_rwlock_rdlock of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    
    if (numSem >= 0 && numSem < __countSem)
    {
        sem_t sem = __sems[numSem];
        
        res = pthread_rwlock_unlock(&__lock_t_sem_create);
        if (res != 0)
        {
            perror("t_sem_post : pthread_rwlock_unlock of __lock_t_sem_create failed");
            exit(EXIT_FAILURE);
        }
        
        int res = sem_post(&sem);
        if (res != 0)
        {
            perror("t_sem_post : Semaphore post failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        perror("t_sem_post : Semaphore post failed - number of semaphore is out of range");
        exit(EXIT_FAILURE);
    }
}

void t_msg_send(struct message msg)
{
    int res = pthread_rwlock_rdlock(&__lock_t_create);
    if (res != 0)
    {
        perror("t_msg_send : pthread_rwlock_rdlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    
    if (msg.numTh > 0 && msg.numTh < __countTh)
    {
        struct __threadInfo *th_info = &(__threads[msg.numTh]);
        
        res = pthread_rwlock_unlock(&__lock_t_create);
        if (res != 0)
        {
            perror("t_msg_send : pthread_rwlock_unlock of __lock_t_create failed");
            exit(EXIT_FAILURE);
        }
        res = pthread_mutex_lock(&(th_info->lock));
        if (res != 0)
        {
            perror("t_msg_send : pthread_mutex_lock of th_info.lock failed");
            exit(EXIT_FAILURE);
        }
        struct __msgList *next = (struct __msgList *)malloc(sizeof *next);
        //        next->msg = msg;
        next->next = NULL;
        
        if (th_info->head)
        {
            th_info->tail->next = next;
            th_info->tail = next;
        }
        else
        {
            th_info->head = next;
            th_info->tail = next;
            
            res = pthread_cond_signal(&(th_info->cond));
            if (res != 0)
            {
                perror("t_msg_send : pthread_cond_signal of th_info.cond failed");
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
        perror("t_msg_send : Message send failed - number of thread is out of range");
        exit(EXIT_FAILURE);
    }
}

struct message t_msg_receive()
{
    struct message mes;      //  временное решение
    int res = pthread_rwlock_rdlock(&__lock_t_create);
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_rwlock_rdlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    
    int numTh = t_getThNum();
    struct __threadInfo *th_info = &(__threads[numTh]);
    
    res = pthread_rwlock_unlock(&__lock_t_create);
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_rwlock_unlock of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    res = pthread_mutex_lock(&(th_info->lock));
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_mutex_lock of th_info.lock failed");
        exit(EXIT_FAILURE);
    }
    if (!th_info->head)
    {
        res = pthread_cond_wait(&(th_info->cond), &(th_info->lock));
        if (res != 0)
        {
            perror("t_msg_recieve : pthread_cond_wait of th_info.cond and th_info.lock failed");
            exit(EXIT_FAILURE);
        }
    }
    struct __msgList *exHead = th_info->head;
    void *msg = exHead->msg;
    th_info->head = exHead->next;
    
    if (!th_info->head)
    {
        th_info->tail = NULL;
    }
    
    free(exHead);
    res = pthread_mutex_unlock(&(th_info->lock));
    if (res != 0)
    {
        perror("t_msg_recieve : pthread_mutex_unlock of th_info.lock failed");
        exit(EXIT_FAILURE);
    }
    
    //    return msg;
    return mes;
}

void* t_seize(void* (*func)(void *), void *arg)
{
    int res = pthread_mutex_lock(&__lock_t_seize);
    if (res != 0)
    {
        perror("t_seize : pthread_mutex_lock of __lock_t_seize failed");
        exit(EXIT_FAILURE);
    }
    
    void *ret = func(arg);
    
    res = pthread_mutex_unlock(&__lock_t_seize);
    if (res != 0)
    {
        perror("t_seize : pthread_mutex_unlock of __lock_t_seize failed");
        exit(EXIT_FAILURE);
    }
    
    return ret;
}

void t_destroyAll()
{
    int res;
    
    for (int i = 0; i < __countTh; i++)
    {
        res = pthread_cond_destroy(&(__threads[i].cond));
        if (res != 0)
        {
            perror("t_destroyAll : pthread_cond_destroy of __threads[i].cond failed");
            exit(EXIT_FAILURE);
        }
        res = pthread_mutex_destroy(&(__threads[i].lock));
        if (res != 0)
        {
            perror("t_destroyAll : pthread_mutex_destroy of __threads[i].lock failed");
            exit(EXIT_FAILURE);
        }
        
        struct __msgList *msg = __threads[i].head;
        __threads[i].head = NULL;
        __threads[i].tail = NULL;
        while (msg)
        {
            struct __msgList *exMsg = msg;
            msg = msg->next;
            free(exMsg);
        }
    }
    free(__threads);
    for (int i = 0; i < __countSem; i++)
    {
        res = sem_destroy(&(__sems[i]));
        if (res != 0)
        {
            perror("t_destroyAll : sem_destroy of __sems[i] failed");
            exit(EXIT_FAILURE);
        }
    }
    free(__sems);
    res = pthread_rwlock_destroy(&__lock_t_create);
    if (res != 0)
    {
        perror("t_destroyAll : pthread_rwlock_destroy of __lock_t_create failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_rwlock_destroy(&__lock_t_sem_create);
    if (res != 0)
    {
        perror("t_destroyAll : pthread_rwlock_destroy of __lock_t_sem_create failed");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_mutex_destroy(&__lock_t_seize);
    if (res != 0)
    {
        perror("t_destroyAll : pthread_mutex_destroy of __lock_t_seize failed");
        exit(EXIT_FAILURE);
    }
}
