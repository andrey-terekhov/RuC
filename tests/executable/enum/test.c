enum name
{
	t,
	a = 10,
	b
} v;

enum
{
	g,
} l;

enum
{
	u = 11,
	uu
};

int main()
{
	enum name e = 25;

	v = 10;
	l = 15;

	assert(t == 0, "t == 0");
	assert(a == 10, "a == 10");
	assert(b == 11, "b == 11");
	assert(g == 0, "g == 0");
	assert(u == 11, "u == 11");
	assert(uu == 12, "u == 11");
	assert(v == 10, "v == 10");
	assert(l == 15, "l == 15");
	assert(e == 25, "e == 25");
	return 0;
}