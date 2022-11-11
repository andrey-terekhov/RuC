int add(int, int), mul(int, int);
void more_asserts();

void main()
{
	int a = 2, b = 3;
	int c = add(a, b), d = mul(a, b);
	int e = mul(d, a);
	int f1 = add(c, d), f2 = mul(c, d);
	int g = add(c, e), h = mul(f1, f2);
	assert(g == 17, "g should be 17");
	assert(h == 330, "h should be 330");

	more_asserts();
}

int add(int l, int r) { return l + r; }
int mul(int l, int r) { return l * r; }

void more_asserts()
{
	int a = add(5, 6);
	int b = add(1, 2);
	int c = mul(4, 5);
	int d = mul(3, 2);
	assert(a == 11, "a should be 11");
	assert(b == 3, "b should be 3");
	assert(c == 20, "c should be 20");
	assert(d == 6, "d should be 6");
}