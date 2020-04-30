#include "lib1.h" 


int S(int a);// 2.h
int P(int a, int b);

int sum(int a, int b);// 3.h

int S(int a)// 2.c
{
	return a + 1;
}

int P(int a, int b)
{
	int r = 0;
	for(int i = 0; i < b; i++)
	{
		r = sum(r, a);
	}

	return r;
}

void main()
{
	int k = P(5, 3);
	printid(k);
}