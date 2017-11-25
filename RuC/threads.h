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

/** Prototype for thread callback */
typedef void* (*thread_func)(void *arg);

/* Thread library initializer/deinitializer */
extern void t_init();
extern void t_destroy();

/**
 * Create thread and start its execution.
 *
 * @param func Thread routine.
 * @param arg  Argument to pass to @p func.
 *
 * @return Thread ID.
 */
extern int t_create(thread_func func, void *arg);
/**
 * Create thread in detached state.
 *
 * @param func Thread routine.
 *
 * @return Thread ID.
 */
extern int t_createDetached(thread_func func);

/**
 * Terminate calling thread immediately.
 */
extern void t_exit();

/**
 * Wait until thread execution completes.
 *
 * @param thread Thread ID
 */
extern void t_join(int thread_id);

/**
 * Get current thread ID.
 *
 * @return Thread ID.
 */
extern int  t_getThNum();

/**
 * Make current thread sleep for specified number of milliseconds.
 *
 * @param time_ms Sleep time in milliseconds.
 */
extern void t_sleep(int time_ms);

/* Semaphore functions */
extern int  t_sem_create(int level);
extern void t_sem_wait(int numSem);
extern void t_sem_post(int numSem);

/**
 * Send specific message to common thread pipe.
 *
 * @param msg Message to send
 * @remark Target thread ID is within the @p msg.
 */
extern void     t_msg_send(msg_info msg);

/**
 * Receive a message for current thread. Blocks execution until the message
 * appears.
 *
 * @return Received msg_info object.
 */
extern msg_info t_msg_receive();

#endif
