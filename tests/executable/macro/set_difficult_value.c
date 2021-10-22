#define  A assert(0, "difficult value")
#set A assert(a, "difficult value")

int main()
{
	int a = 0;

	a = 1;
	A;

	return 0;
}
