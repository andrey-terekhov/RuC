int t = 9;

struct p
{
	int a[t];
};

void main()
{
	int n = 6;
	int a[n];
	assert(upb(a, 0) == 6, "length must be 6");
	printf("%i\n", upb(a, 0));

	t = 10;
	struct p pt;
	assert(upb(pt.a, 0) == 10, "length must be 10");
	printf("%i\n", upb(pt.a, 0));
}
