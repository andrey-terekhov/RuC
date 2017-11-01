#include <stdlib.h>
#include <stdint.h>
#include "th_static.h"
#define N 8
#define Q 3

void* work_thread(void *arg )
{
	int thNum = t_getThNum();
	int sum = thNum;
	int pt = 1;

	for (int i = 0; i < Q; i++)
	{
		if (thNum % (pt * 2) == pt)
		{
			struct msg_info msg = {thNum+pt, sum};
			t_msg_send(msg);
		}
		if (thNum % (pt * 2) == 0)
		{
			struct msg_info msg = t_msg_recieve();
			sum += msg.data;
		}
		pt *= 2;
	}
	if (thNum == N)
	{
		struct msg_info msg = {0, sum};
		t_msg_send(msg);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	t_init();
	
	for (int i = 0; i < N; i++)
	{ t_createDetached(work_thread); }
	int sum = t_msg_recieve().data;
	printf("Sum is %d\n" , sum);
	printf("Gauss check = %s\n",
		N*(N+1)/2 == sum ? "TRUE" : "FALSE");

	t_destroy();
	return 0;
}