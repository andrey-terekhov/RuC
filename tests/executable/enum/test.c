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
	enum name e = 25;

	v = 10;
	l = 15;

	assert(t == 0, "t == 0");
	assert(a == 5, "a == 5");
	assert(b == 6, "b == 6");
	assert(g == 0, "g == 0");
	assert(u == 11, "u == 11");
	assert(uu == 3, "uu == 3");
	assert(v == 10, "v == 10");
	assert(l == 15, "l == 15");
	assert(e == 25, "e == 25");
	return 0;
}
