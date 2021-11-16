int t = 9;

struct p
{
	int a[t];
};

void main()
{
	int n = 6;
	int a[n];
	assert(upb(0, a) == 6, "length must be 6");

	t = 10;
	struct p pt;
	assert(upb(0, pt.a) == 10, "length must be 10");
}
