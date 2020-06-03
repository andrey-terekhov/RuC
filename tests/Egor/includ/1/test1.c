#include"lib1.c"

int P(int a, int b)
{
	int r = 0;
	int i = 0;
	for(i = 0; i < b; i++)
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