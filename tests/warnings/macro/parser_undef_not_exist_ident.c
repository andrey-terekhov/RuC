int A = 1;
#undef a

int main()
{
	assert(A, "undef not exist ident");
	return 0;
}
