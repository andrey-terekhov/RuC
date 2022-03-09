enum name
{
	t,
	a = 10 / 2,
	b
} v;

enum
{
	g,
} l;

enum
{
	u = 11,
	uu = (b + 5 * 2 - 1 ) / a
};

int main()
{
	int num = 1 + 2 * 3 + 4 - 5;
	int num_2 = 10 + 7 + 3;
	enum name e = a;

	v = e;
	l = g;

	assert(num == 6, "hhhh == 15");
	assert(num_2 == 20, "hhhh == 15");
	assert(t == 0, "t == 0");
	assert(a == 5, "a == 5");
	assert(b == 6, "b == 6");
	assert(g == 0, "g == 0");
	assert(u == 11, "u == 11");
	assert(uu == 3, "uu == 3");
	assert(v == 5, "v == 10");
	assert(l == 0, "l == 15");
	assert(e == 5, "e == 25");
	return 0;
}
