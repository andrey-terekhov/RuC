#define A 0
#undef /* long
comment*/ A

int main()
{
	int A = 1;

	assert(A, "long comment before ident");

	return 0;
}
