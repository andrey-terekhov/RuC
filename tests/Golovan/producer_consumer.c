#include <stdlib.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "th_static.h"
#define BUF_SIZE 10
#define COUNT_PRODUCE 30

int accessNum;
int emptyNum;
int fullNum;
uint64_t buffer[BUF_SIZE];
int buf_count = 0;
uint64_t s[2] = { 42, 1350 };

uint64_t simple_random()
{
	uint64_t x = s[0];
	uint64_t const y = s[1];
	s[0] = y;
	x ^= x << 23;
	s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
	return s[1] + y;
}

void* producer_thread (void *arg )
{
	int count = 0;
	while(count++ < COUNT_PRODUCE)
	{
		uint64_t item = simple_random();
		t_sem_wait(fullNum);
		t_sem_wait(accessNum);
		buffer[buf_count++] = item;
		printf("Item %"PRIu64" produced\n", item);
		t_sem_post(accessNum);
		t_sem_post(emptyNum);
		t_sleep(item % 5);
	}
	printf("In total %d items was produced\n", COUNT_PRODUCE);

	return NULL;
}

void* consumer_thread(void *arg )
{
	int count = 0;
	while(count++ < COUNT_PRODUCE)
	{
		t_sem_wait(emptyNum);
		t_sem_wait(accessNum);
		uint64_t item = buffer[--buf_count];
		t_sem_post(accessNum);
		t_sem_post(fullNum);
		printf("Item %"PRIu64" consumed\n", item);
		t_sleep(item % 5);
	}
	printf("In total %d items was consumed\n", COUNT_PRODUCE);
	return NULL;
}

int main(int argc, char **argv)
{
	t_init();
	
	accessNum = t_sem_create(1);
	emptyNum = t_sem_create (0);
	fullNum = t_sem_create(BUF_SIZE);
	t_create(producer_thread);
	t_create(consumer_thread);
	t_join(1); t_join(2);

	t_destroy();
	return 0;
}