#ifndef RUC_THREADS_H
#define RUC_THREADS_H

#define TH_MAX_THREAD_COUNT         16
#define TH_MAX_SEMAPHORE_COUNT      16
#define TH_MAX_THREAD_MSG_COUNT     4

typedef struct msg_info
{
    int numTh;
    int data;
} msg_info;

typedef void* (*thread_func)(void *);

void t_init();
void t_destroy();

int t_create(thread_func func, void *arg);
int t_createDetached(thread_func func);

void t_exit();
void t_join(int numTh);
int t_getThNum();
void t_sleep(int miliseconds);

int t_sem_create(int level);
void t_sem_wait(int numSem);
void t_sem_post(int numSem);

void t_msg_send(struct msg_info msg);
struct msg_info t_msg_receive();


#endif
