void main()
{
	float a[3] = {1, 2, 3};
	float d[3] = {1, 2, 3};
	float b[2][3] =  {{1, 2, 3}, {4, 5, 6}};
	float c[2][2][3] = {{{7, 8, 9}, {10, 11, 12}}, {{1, 2, 3}, {4, 5, 6}}};
	const float delta = 0.001;

	a = {2 ,3, 4};
	b = {a, {1, 2, 3}};
	d = a;
	c[1] = b;
	c[1][1] = a;

	assert(abs(a[0] - 2) < delta, "a[0] must be 2");
	assert(abs(a[1] - 3) < delta, "a[1] must be 3");
	assert(abs(a[2] - 4) < delta, "a[2] must be 4");

	assert(abs(d[0] - 2) < delta, "d[0] must be 2");
	assert(abs(d[1] - 3) < delta, "d[1] must be 3");
	assert(abs(d[2] - 4) < delta, "d[2] must be 4");

	assert(abs(b[0][0] - 2) < delta, "b[0][0] must be 2");
	assert(abs(b[0][1] - 3) < delta, "b[0][1] must be 3");
	assert(abs(b[0][2] - 4) < delta, "b[0][2] must be 4");
	assert(abs(b[1][0] - 1) < delta, "b[1][0] must be 1");
	assert(abs(b[1][1] - 2) < delta, "b[1][1] must be 2");
	assert(abs(b[1][2] - 3) < delta, "b[1][2] must be 3");

	assert(abs(c[0][0][0] - 7) < delta, "c[0][0][0] must be 7");
	assert(abs(c[0][0][1] - 8) < delta, "c[0][0][1] must be 8");
	assert(abs(c[0][0][2] - 9) < delta, "c[0][0][2] must be 9");
	assert(abs(c[0][1][0] - 10) < delta, "c[0][1][0] must be 10");
	assert(abs(c[0][1][1] - 11) < delta, "c[0][1][1] must be 11");
	assert(abs(c[0][1][2] - 12) < delta, "c[0][1][2] must be 12");
	assert(abs(c[1][0][0] - 2) < delta, "c[1][0][0] must be 2");
	assert(abs(c[1][0][1] - 3) < delta, "c[1][0][1] must be 3");
	assert(abs(c[1][0][2] - 4) < delta, "c[1][0][2] must be 4");
	assert(abs(c[1][1][0] - 1) < delta, "c[1][1][0] must be 1");
	assert(abs(c[1][1][1] - 2) < delta, "c[1][1][1] must be 2");
	assert(abs(c[1][1][2] - 3) < delta, "c[1][1][2] must be 3");

	a = b[1];

	assert(abs(a[0] - 1) < delta, "a[0] must be 1");
	assert(abs(a[1] - 2) < delta, "a[1] must be 2");
	assert(abs(a[2] - 3) < delta, "a[2] must be 3");

	a = c[0][0];

	assert(abs(a[0] - 7) < delta, "a[0] must be 7");
	assert(abs(a[1] - 8) < delta, "a[1] must be 8");
	assert(abs(a[2] - 9) < delta, "a[2] must be 9");

	b[0] = c[0][0];

	assert(abs(b[0][0] - 7) < delta, "b[0][0] must be 7");
	assert(abs(b[0][1] - 8) < delta, "b[0][1] must be 8");
	assert(abs(b[0][2] - 9) < delta, "b[0][2] must be 9");
	assert(abs(b[1][0] - 1) < delta, "b[1][0] must be 1");
	assert(abs(b[1][1] - 2) < delta, "b[1][1] must be 2");
	assert(abs(b[1][2] - 3) < delta, "b[1][2] must be 3");

	c[0] = {b[1], a};

	assert(abs(c[0][0][0] - 1) < delta, "c[0][0][0] must be 1");
	assert(abs(c[0][0][1] - 2) < delta, "c[0][0][1] must be 2");
	assert(abs(c[0][0][2] - 3) < delta, "c[0][0][2] must be 3");
	assert(abs(c[0][1][0] - 7) < delta, "c[0][1][0] must be 7");
	assert(abs(c[0][1][1] - 8) < delta, "c[0][1][1] must be 8");
	assert(abs(c[0][1][2] - 9) < delta, "c[0][1][2] must be 9");
	assert(abs(c[1][0][0] - 2) < delta, "c[1][0][0] must be 2");
	assert(abs(c[1][0][1] - 3) < delta, "c[1][0][1] must be 3");
	assert(abs(c[1][0][2] - 4) < delta, "c[1][0][2] must be 4");
	assert(abs(c[1][1][0] - 2) < delta, "c[1][1][0] must be 2");
	assert(abs(c[1][1][1] - 3) < delta, "c[1][1][1] must be 3");
	assert(abs(c[1][1][2] - 4) < delta, "c[1][1][2] must be 4");

	/*
	c = {c[0], c[1]};

	assert(abs(c[0][0][0] - 1) < delta, "c[0][0][0] must be 1");
	assert(abs(c[0][0][1] - 2) < delta, "c[0][0][1] must be 2");
	assert(abs(c[0][0][2] - 3) < delta, "c[0][0][2] must be 3");
	assert(abs(c[0][1][0] - 7) < delta, "c[0][1][0] must be 7");
	assert(abs(c[0][1][1] - 8) < delta, "c[0][1][1] must be 8");
	assert(abs(c[0][1][2] - 9) < delta, "c[0][1][2] must be 9");
	assert(abs(c[1][0][0] - 2) < delta, "c[1][0][0] must be 2");
	assert(abs(c[1][0][1] - 3) < delta, "c[1][0][1] must be 3");
	assert(abs(c[1][0][2] - 4) < delta, "c[1][0][2] must be 4");
	assert(abs(c[1][1][0] - 2) < delta, "c[1][1][0] must be 2");
	assert(abs(c[1][1][1] - 3) < delta, "c[1][1][1] must be 3");
	assert(abs(c[1][1][2] - 4) < delta, "c[1][1][2] must be 4");
	*/
}

