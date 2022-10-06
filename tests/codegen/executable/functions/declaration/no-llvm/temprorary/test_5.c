int add(int, int), mul(int, int);

void main()
{
	int a = 2, b = 3;
	int c = add(a, b), d = mul(a, b);
	int e = mul(d, a);
	int f1 = add(c, d), f2 = mul(c, d);
	int g = add(c, e), h = mul(f1, f2);
	assert(g == 17, "g should be 17");
	assert(h == 330, "h should be 330");
}

int add(int l, int r) { return l + r; }
int mul(int l, int r) { return l * r; }