#pragma region CONSTANTS
#define __COUNT_TH 16
#define __COUNT_SEM 16
#define __COUNT_MSGS_FOR_TH 4
#pragma endregion

#include <fcntl.h>

struct msg_info
{
	int numTh;
	int data;
};
void t_init();
void t_destroy();

int t_create_inner(void* (*func)(void*), void *arg);
int t_create(void* (*func)(void*));

int t_createDetached(void* (*func)(void*));

void t_exit();
void t_join(int numTh);
int t_getThNum();
void t_sleep(int miliseconds);

int t_sem_create(int level);
void t_sem_wait(int numSem);
void t_sem_post(int numSem);

void t_msg_send(struct msg_info msg);
struct msg_info t_msg_receive();

