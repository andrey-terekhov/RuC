//
//  threads.h
//  RuC
//
//  Created by Andrey Terekhov on 10/08/2017.
//  Copyright © 2017 SE chair. All rights reserved.
//

#ifndef threads_h
#define threads_h

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> //sleep Linux
#include <errno.h> //error Linux
#include <stdio.h> //perror Linux
#include <stdlib.h>

#ifndef _REENTRANT
#define _REENTRANT
#endif

#define __COUNT_TH_CONTAINER_DEFAULT 8
#define __COUNT_SEM_CONTAINER_DEFAULT 8
#define TRUE 1
#define FALSE 0


struct message{int numTh; int inf;};
// numTh при посылке сообщения говорит куда, при приеме - откуда

int t_create(void(*func)(int), int arg);
int t_createDetached(void* (*func)(void *), void *arg);  // пока не буду

void t_exit();

void t_join(int numTh);    // пока не буду
int t_getNum();            // пока не буду
void t_sleep(int miliseconds);

int t_sem_create(int level);
void t_sem_wait(int numSem);
void t_sem_post(int numSem);

void t_msg_send(struct message msg);
struct message t_msg_receive();


#endif /* threads_h */
