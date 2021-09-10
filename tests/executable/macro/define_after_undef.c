#define A 10
#define B 1 + A

#undef A
#define A 20

int main()
{
	assert(B == 21, "B = 1 + A != 21");
	return 0;
}
