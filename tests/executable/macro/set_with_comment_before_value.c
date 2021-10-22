#define A 0
#set A /* long
comment*/ 1

int main()
{
	int a = 0;

	a = A;
	assert(a, "long comment before value");

	return 0;
}
