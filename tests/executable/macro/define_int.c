#define A 1
#define B 1 + 2
#define C A + B
#define D C - A - B

int main()
{
	assert(A == 1, "define int");
	assert(B == 3, "define int + int");
	assert(C == 4, "define ident + ident");
	assert(D == 0, "define ident - ident - ident");

	return 0;
}
