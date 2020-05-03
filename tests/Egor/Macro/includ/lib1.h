int sum(int a, int b);// 3.h

int S(int a);// 2.h
int P(int a, int b);

int sum(int a, int b) // 3.c
{
	return S(a) + S(b);
}