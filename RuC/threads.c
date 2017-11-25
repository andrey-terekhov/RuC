#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "threads.h"
#include "error.h"
#include "common.h"

#ifndef _REENTRANT
#define _REENTRANT
#endif

#pragma region THREADS

typedef struct thread_info
{
    pthread_t       th;
    int             isDetach;
    pthread_cond_t  cond;
    pthread_mutex_t lock;
    msg_info        msgs[TH_MAX_THREAD_MSG_COUNT];
    int             countMsg;
} thread_info;

static int         threadCount = 1;
static thread_info threads[TH_MAX_THREAD_COUNT];

#pragma endregion

#pragma region SEMAPHORES

static int      semaphoreCount = 0;
static sem_t    semaphores[TH_MAX_SEMAPHORE_COUNT];

#pragma endregion

#pragma region LOCKS

static pthread_rwlock_t lock_t_create;
static pthread_rwlock_t lock_t_sem_create;

#pragma endregion

#pragma region INIT

void t_init()
{
#pragma region LOCK_T_CREATE
    int res = pthread_rwlock_init(&lock_t_create, NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_rwlock_init of lock_t_create failed");
    }
#pragma endregion

#pragma region MAIN_THREAD
    threads[0].th = pthread_self();
    threads[0].isDetach = TRUE;

    res = pthread_cond_init(&threads[0].cond, NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_cond_init of threads[0].cond failed");
    }

    res = pthread_mutex_init(&threads[0].lock, NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_mutex_init of threads[0].lock failed");
    }
#pragma endregion

#pragma region LOCK_T_SEM_CREATE
    res = pthread_rwlock_init(&lock_t_sem_create, NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_init of lock_t_sem_create failed");
    }
#pragma endregion
}
#pragma endregion

#pragma region IMPLEMENTATION
int
__t_create(pthread_attr_t *attr,
           thread_func func,
           void *arg,
           int isDetach)
{
#pragma region WRITE_LOCK_THREAD_TABLE
    int res = pthread_rwlock_wrlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_rwlock_wrlock of lock_t_create failed");
    }
#pragma endregion

#pragma region CREATE_THREAD
    pthread_t th;

    res = pthread_create(&th, attr, func, arg);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "Thread creation failed");
    }

    if (attr)
    {
        res = pthread_attr_destroy(attr);
        if(res != 0)
        {
            exit_err(EXIT_FAILURE, "Thread attribute destroy failed");
        }
    }
#pragma endregion

#pragma region STORE_DESCRIPTOR
    if (threadCount >= TH_MAX_THREAD_COUNT)
    {
        exit_err(EXIT_FAILURE, "Trying to create too much threads");
    }

#pragma region INIT_THREAD_INFO
    threads[threadCount].th = th;
    threads[threadCount].isDetach = isDetach;

    res = pthread_cond_init(&(threads[threadCount].cond), NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_cond_init of threads[threadCount].cond failed");
    }

    res = pthread_mutex_init(&(threads[threadCount].lock), NULL);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_mutex_init of threads[threadCount].lock failed");
    }

    threads[threadCount].countMsg = 0;
#pragma endregion

#pragma endregion

    int retVal = threadCount++;

#pragma region UNLOCK_THREAD_TABLE
    res = pthread_rwlock_unlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_rwlock_unlock of lock_t_create failed");
    }
#pragma endregion

    return retVal;
}

int
t_create(thread_func func, void *arg)
{
    return __t_create(NULL, func, arg, FALSE);
}

int
t_createDetached(thread_func func)
{
    pthread_attr_t attr;

    int res = pthread_attr_init(&attr);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "Attribute creation failed");
    }

    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "Setting detached attribute failed");
    }

    return __t_create(&attr, func, NULL, TRUE);
}

void
t_exit()
{
    pthread_exit(NULL);
}

void
t_join(int numTh)
{
#pragma region READ_LOCK_THREAD_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_rwlock_rdlock of lock_t_create failed");
    }
#pragma endregion

    if (numTh > 0 && numTh < threadCount)
    {
        if (!threads[numTh].isDetach)
        {
            pthread_t th = threads[numTh].th;

#pragma region UNLOCK_THREAD_TABLE
            res = pthread_rwlock_unlock(&lock_t_create);
            if (res != 0)
            {
                exit_err(EXIT_FAILURE,
                         "pthread_rwlock_unlock of lock_t_create failed");
            }
#pragma endregion

            res = pthread_join(th, NULL);
            if (res != 0)
            {
                exit_err(EXIT_FAILURE, "Thread join failed");
            }
        }
        else
        {
            exit_err(EXIT_FAILURE, "Thread join failed - thread is detached");
        }
    }
    else
    {
        exit_err(EXIT_FAILURE,
                 "Thread join failed - number of thread is out of range");
    }
}

int t_getThNum()
{
#pragma region READ_LOCK_THREAD_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "pthread_rwlock_rdlock of lock_t_create failed");
    }
#pragma endregion

    pthread_t th = pthread_self();
    int index = -1, i;

    for (i = 0; i < threadCount; i++)
    {
        if (pthread_equal(th, threads[i].th))
        { index = i; }
    }

    if (index != -1)
    {
#pragma region UNLOCK_THREAD_TABLE
        res = pthread_rwlock_unlock(&lock_t_create);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_rwlock_unlock of lock_t_create failed");
        }
#pragma endregion

        return index;
    }

    exit_err(EXIT_FAILURE, "Thread is not registered");
}

void
t_sleep(int miliseconds)
{
    //Sleep(seconds * 1000);
    usleep(miliseconds * 1000);
}

int
t_sem_create(int level)
{
#pragma region WRITE_LOCK_SEM_TABLE
    int res = pthread_rwlock_wrlock(&lock_t_sem_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_wrlock of lock_t_sem_create failed");
    }
#pragma endregion

#pragma region CREATE_SEMAPHORE
    sem_t sem;

    res = sem_init(&sem, 0, level);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE, "Semaphore initilization failed");
    }
#pragma endregion

#pragma region STORE_SEMAPHORE
    if (semaphoreCount >= TH_MAX_SEMAPHORE_COUNT)
    {
        exit_err(EXIT_FAILURE, "Trying to create too much semaphores");
    }
    semaphores[semaphoreCount] = sem;
#pragma endregion

    int retVal = semaphoreCount++;

#pragma region UNLOCK_SEM_TABLE
    res = pthread_rwlock_unlock(&lock_t_sem_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_unlock of lock_t_sem_create failed");
    }
#pragma endregion

    return retVal;
}

void
t_sem_wait(int numSem)
{
#pragma region READ_LOCK_SEM_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_sem_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_rdlock of lock_t_sem_create failed");
    }
#pragma endregion

    if (numSem >= 0 && numSem < semaphoreCount)
    {
        sem_t sem = semaphores[numSem];

#pragma region UNLOCK_SEM_TABLE
        res = pthread_rwlock_unlock(&lock_t_sem_create);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_rwlock_unlock of lock_t_sem_create failed");
        }
#pragma endregion

        int res = sem_wait(&sem);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "Semaphore wait failed");
        }
    }
    else
    {
        exit_err(EXIT_FAILURE,
                 "Semaphore wait failed - number of semaphore is out of range");
    }
}

void
t_sem_post(int numSem)
{
#pragma region READ_LOCK_SEM_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_sem_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_rdlock of lock_t_sem_create failed");
    }
#pragma endregion

    if (numSem >= 0 && numSem < semaphoreCount)
    {
        sem_t sem = semaphores[numSem];
#pragma region UNLOCK_SEM_TABLE
        res = pthread_rwlock_unlock(&lock_t_sem_create);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_rwlock_unlock of lock_t_sem_create failed");
        }
#pragma endregion

        int res = sem_post(&sem);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE, "Semaphore post failed");
        }
    }
    else
    {
        exit_err(EXIT_FAILURE,
                 "Semaphore post failed - number of semaphore is out of range");
    }
}

void
t_msg_send(msg_info msg)
{
#pragma region READ_LOCK_THREAD_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_rdlock of lock_t_create failed");
    }
#pragma endregion

    if (msg.numTh >= 0 && msg.numTh < threadCount)
    {
        thread_info *th_info = &(threads[msg.numTh]);

#pragma region UNLOCK_THREAD_TABLE
        res = pthread_rwlock_unlock(&lock_t_create);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_rwlock_unlock of lock_t_create failed");
        }
#pragma endregion

#pragma region LOCK_THREAD_INFO
        res = pthread_mutex_lock(&(th_info->lock));
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_mutex_lock of th_info.lock failed");
        }
#pragma endregion

#pragma region ADD_MSG_TO_LIST
        if (th_info->countMsg >= TH_MAX_THREAD_MSG_COUNT)
        {
            exit_err(EXIT_FAILURE, "Trying to send too many messages");
        }
        th_info->msgs[th_info->countMsg].numTh = t_getThNum();
        th_info->msgs[th_info->countMsg++].data = msg.data;

        if (th_info->countMsg == 1)
        {
#pragma region SIGNAL
            //printf("SEND HERE\n");
            res = pthread_cond_signal(&(th_info->cond));
            if (res != 0)
            {
                exit_err(EXIT_FAILURE,
                         "pthread_cond_signal of th_info.cond failed");
            }
            //printf("SEND HERE AND\n");
#pragma endregion
        }
#pragma endregion

#pragma region UNLOCK_THREAD_INFO
        res = pthread_mutex_unlock(&(th_info->lock));
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_mutex_unlock of th_info.lock failed");
        }
#pragma endregion
    }
    else
    {
        exit_err(EXIT_FAILURE,
                 "Message send failed - number of thread is out of range");
    }
    //printf("SENDED NUMTH=%d, DATA=%d\n", msg.numTh, msg.data);
}

msg_info
t_msg_receive()
{
#pragma region READ_LOCK_THREAD_TABLE
    int res = pthread_rwlock_rdlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_rdlock of lock_t_create failed");
    }
#pragma endregion

    int numTh = t_getThNum();
    thread_info *th_info = &(threads[numTh]);

#pragma region UNLOCK_THREAD_TABLE
    res = pthread_rwlock_unlock(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_unlock of lock_t_create failed");
    }
#pragma endregion

#pragma region LOCK_THREAD_INFO_LOCK
    res = pthread_mutex_lock(&(th_info->lock));
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_mutex_lock of th_info.lock failed");
    }
#pragma endregion

#pragma region WAIT_FOR_SIGNAL
    if (th_info->countMsg == 0)
    {
        //printf("RECV HERE\n");
        res = pthread_cond_wait(&th_info->cond, &th_info->lock);
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                "pthread_cond_wait of th_info.cond and th_info.lock failed");
        }
        //printf("RECV HERE AND\n");
    }
#pragma endregion

#pragma region GET_MSG_FROM_LIST
    struct msg_info msg = th_info->msgs[--th_info->countMsg];
#pragma endregion

#pragma region UNLOCK_THREAD_INFO
    res = pthread_mutex_unlock(&(th_info->lock));
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_mutex_unlock of th_info.lock failed");
    }
#pragma endregion

    return msg;
}

void
t_destroy()
{
    int res, i;

#pragma region THREAD_TABLE
    for (i = 0; i < threadCount; i++)
    {
#pragma region PTHREAD_COND
        res = pthread_cond_destroy(&(threads[i].cond));
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_cond_destroy of threads[i].cond failed");
        }
#pragma endregion

#pragma region PTHREAD_MUTEX
        res = pthread_mutex_destroy(&(threads[i].lock));
        if (res != 0)
        {
            exit_err(EXIT_FAILURE,
                     "pthread_mutex_destroy of threads[i].lock failed");
        }
#pragma endregion
    }
#pragma endregion

#pragma region SEM_TABLE
    for (i = 0; i < semaphoreCount; i++)
    {
        res = sem_destroy(&(semaphores[i]));
        if (res != 0)
        {
            exit_err(EXIT_FAILURE, "sem_destroy of semaphores[i] failed");
        }
    }
#pragma endregion

#pragma region LOCKS
    res = pthread_rwlock_destroy(&lock_t_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_destroy of lock_t_create failed");
    }

    res = pthread_rwlock_destroy(&lock_t_sem_create);
    if (res != 0)
    {
        exit_err(EXIT_FAILURE,
                 "pthread_rwlock_destroy of lock_t_sem_create failed");
    }
#pragma endregion
}
#pragma endregion
