#define A 1 + 1
#define B A + 100

int main()
{
	assert(B == 102, "def A -> def B: A + const");

	return 0;
}
