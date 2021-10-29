#define A "1 + 1"

int main()
{
	int res;
	res = strcmp(A, "1 + 1");
	assert(res == 0, "string created by #define");

	return 0;
}
