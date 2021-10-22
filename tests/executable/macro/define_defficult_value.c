#define  A assert(a, "long comment before ident")

int main()
{
	int a = 0;

	a = 1;
	A;

	return 0;
}
