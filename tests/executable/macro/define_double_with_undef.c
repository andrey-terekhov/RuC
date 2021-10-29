#define A 1 + 1
#define B A + 100
#undef A

int main()
{
	int A = 0;
	assert(B == 100, "def A -> def B: A + const -> undef A -> A = const");

	return 0;
}
