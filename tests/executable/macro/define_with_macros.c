#define A 1
#define B 2
#define C A + 1
#define D A + B

int main()
{
	assert(C == 2, "A + 1 != 2");
	assert(D == 3, "A + B != 3");
	return 0;
}
