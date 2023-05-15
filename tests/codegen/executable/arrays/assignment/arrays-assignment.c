void main()
{
	int a[3] = {1, 2, 3};
	int d[3] = {1, 2, 3};
	int b[2][3] =  {{1, 2, 3}, {4, 5, 6}};
	int c[2][2][3] = {{{7, 8, 9}, {10, 11, 12}}, {{1, 2, 3}, {4, 5, 6}}};

	a = {2 ,3, 4};
	b = {a, {1, 2, 3}};
	d = a;
	c[1] = b;
	c[1][1] = a;

	assert(a[0] == 2, "a[0] must be 2");
	assert(a[1] == 3, "a[1] must be 3");
	assert(a[2] == 4, "a[2] must be 4");

	assert(d[0] == 2, "d[0] must be 2");
	assert(d[1] == 3, "d[1] must be 3");
	assert(d[2] == 4, "d[2] must be 4");

	assert(b[0][0] == 2, "b[0][0] must be 2");
	assert(b[0][1] == 3, "b[0][1] must be 3");
	assert(b[0][2] == 4, "b[0][2] must be 4");
	assert(b[1][0] == 1, "b[1][0] must be 1");
	assert(b[1][1] == 2, "b[1][1] must be 2");
	assert(b[1][2] == 3, "b[1][2] must be 3");

	assert(c[0][0][0] == 7, "c[0][0][0] must be 7");
	assert(c[0][0][1] == 8, "c[0][0][1] must be 8");
	assert(c[0][0][2] == 9, "c[0][0][2] must be 9");
	assert(c[0][1][0] == 10, "c[0][1][0] must be 10");
	assert(c[0][1][1] == 11, "c[0][1][1] must be 11");
	assert(c[0][1][2] == 12, "c[0][1][2] must be 12");
	assert(c[1][0][0] == 2, "c[1][0][0] must be 2");
	assert(c[1][0][1] == 3, "c[1][0][1] must be 3");
	assert(c[1][0][2] == 4, "c[1][0][2] must be 4");
	assert(c[1][1][0] == 2, "c[1][1][0] must be 2");
	assert(c[1][1][1] == 3, "c[1][1][1] must be 3");
	assert(c[1][1][2] == 4, "c[1][1][2] must be 4");

	a = b[1];

	assert(a[0] == 1, "a[0] must be 1");
	assert(a[1] == 2, "a[1] must be 2");
	assert(a[2] == 3, "a[2] must be 3");

	a = c[0][0];

	assert(a[0] == 7, "a[0] must be 7");
	assert(a[1] == 8, "a[1] must be 8");
	assert(a[2] == 9, "a[2] must be 9");

	b[0] = c[0][0];

	assert(b[0][0] == 7, "b[0][0] must be 7");
	assert(b[0][1] == 8, "b[0][1] must be 8");
	assert(b[0][2] == 9, "b[0][2] must be 9");
	assert(b[1][0] == 1, "b[1][0] must be 1");
	assert(b[1][1] == 2, "b[1][1] must be 2");
	assert(b[1][2] == 3, "b[1][2] must be 3");

	c[0] = {b[1], a};

	assert(c[0][0][0] == 1, "c[0][0][0] must be 1");
	assert(c[0][0][1] == 2, "c[0][0][1] must be 2");
	assert(c[0][0][2] == 3, "c[0][0][2] must be 3");
	assert(c[0][1][0] == 7, "c[0][1][0] must be 7");
	assert(c[0][1][1] == 8, "c[0][1][1] must be 8");
	assert(c[0][1][2] == 9, "c[0][1][2] must be 9");
	assert(c[1][0][0] == 2, "c[1][0][0] must be 2");
	assert(c[1][0][1] == 3, "c[1][0][1] must be 3");
	assert(c[1][0][2] == 4, "c[1][0][2] must be 4");
	assert(c[1][1][0] == 2, "c[1][1][0] must be 2");
	assert(c[1][1][1] == 3, "c[1][1][1] must be 3");
	assert(c[1][1][2] == 4, "c[1][1][2] must be 4");

	/*

	c = {c[0], c[1]};


	assert(c[0][0][0] == 1, "c[0][0][0] must be 1");
	assert(c[0][0][1] == 2, "c[0][0][1] must be 2");
	assert(c[0][0][2] == 3, "c[0][0][2] must be 3");
	assert(c[0][1][0] == 7, "c[0][1][0] must be 7");
	assert(c[0][1][1] == 8, "c[0][1][1] must be 8");
	assert(c[0][1][2] == 9, "c[0][1][2] must be 9");
	assert(c[1][0][0] == 2, "c[1][0][0] must be 2");
	assert(c[1][0][1] == 3, "c[1][0][1] must be 3");
	assert(c[1][0][2] == 4, "c[1][0][2] must be 4");
	assert(c[1][1][0] == 2, "c[1][1][0] must be 2");
	assert(c[1][1][1] == 3, "c[1][1][1] must be 3");
	assert(c[1][1][2] == 4, "c[1][1][2] must be 4");

	*/

}

