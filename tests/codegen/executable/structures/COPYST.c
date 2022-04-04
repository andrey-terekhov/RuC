struct point {int x; float y; int z;};

struct point func(int r)
{
	struct point rt = {r, r, r};
	return rt;
}


void main()
{
	struct point z = func(5);
	assert(z.x == 5, "z.x != 5");

	struct point *z2 = &z;
	assert(z2->x == 5, "z2->x != 5");

	assert(func(5).x == 5, "wrong select from rvalue");
}
