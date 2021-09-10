#define KEK 1

int main()
{
#if KEK == 0	// отсутствует #endif для этой директивы
	printf("0\n");
#elif KEK == 1
	printf("1\n");
#else
	printf("10\n");

	return 0;
}
