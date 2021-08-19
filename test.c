void test()
{
#ifdef A
	printf("A\n");
#else
	printf("B\n");
#endif
}

#define A
