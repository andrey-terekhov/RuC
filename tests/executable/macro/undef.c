int A = 1;
#define A 0
#define B 10
#define C B + A

int main()
{
	assert(C == 10, "before undef");
#undef A
	assert(C == 11, "after undef");
	assert(A, "undef ident");

	return 0;
}
