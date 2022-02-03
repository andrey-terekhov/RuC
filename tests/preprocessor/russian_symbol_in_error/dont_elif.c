#define перем 1

int main()
{
#ifdef перем
	printf("0\n");
#elif перем == 1
	printf("1\n");
#else
	printf("10\n");

	return 0;
}
