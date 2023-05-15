void main()
{
    int a[3] = {1, 2, 3};
    int b[2][3] =  {a, {4, 5, 6}};
    int c[2][2][3] = {{{1, 2, 3}, {4, 5, 6}}, b};
	int d[3][2][2][] = {c, c, c};
	int e[2][3] = {b[0], b[1]};
	float f[] = {1, 2, 3};
	float g[2][3] = {{2, 3 ,4}, {5, 6 ,7}};
	float h[3][3] = {f, g[0], g[1]};
	const float delta = 0.001;

    assert(a[0] == 1, "a[0] must be 1");
    assert(a[1] == 2, "a[1] must be 2");
    assert(a[2] == 3, "a[2] must be 3");

    assert(b[0][0] == 1, "b[0][0] must be 1");
    assert(b[0][1] == 2, "b[0][1] must be 2");
    assert(b[0][2] == 3, "b[0][2] must be 3");
    assert(b[1][0] == 4, "b[1][0] must be 4");
    assert(b[1][1] == 5, "b[1][1] must be 5");
    assert(b[1][2] == 6, "b[1][2] must be 6");

    assert(c[0][0][0] == 1, "c[0][0][0] must be 1");
    assert(c[0][0][1] == 2, "c[0][0][1] must be 2");
    assert(c[0][0][2] == 3, "c[0][0][2] must be 3");
    assert(c[0][1][0] == 4, "c[0][1][0] must be 4");
    assert(c[0][1][1] == 5, "c[0][1][1] must be 5");
    assert(c[0][1][2] == 6, "c[0][1][2] must be 6");
    assert(c[1][0][0] == 1, "c[1][0][0] must be 1");
    assert(c[1][0][1] == 2, "c[1][0][1] must be 2");
    assert(c[1][0][2] == 3, "c[1][0][2] must be 3");
    assert(c[1][1][0] == 4, "c[1][1][0] must be 4");
    assert(c[1][1][1] == 5, "c[1][1][1] must be 5");
    assert(c[1][1][2] == 6, "c[1][1][2] must be 6");

	assert(d[0][0][0][0] == 1, "d[0][0][0][0] must be 1");
	assert(d[0][0][0][1] == 2, "d[0][0][0][1] must be 2");
	assert(d[0][0][0][2] == 3, "d[0][0][0][2] must be 3");
	assert(d[0][0][1][0] == 4, "d[0][0][1][0] must be 4");
	assert(d[0][0][1][1] == 5, "d[0][0][1][1] must be 5");
	assert(d[0][0][1][2] == 6, "d[0][0][1][2] must be 6");
	assert(d[0][1][0][0] == 1, "d[0][1][0][0] must be 1");
	assert(d[0][1][0][1] == 2, "d[0][1][0][1] must be 2");
	assert(d[0][1][0][2] == 3, "d[0][1][0][2] must be 3");
	assert(d[0][1][1][0] == 4, "d[0][1][1][0] must be 4");
	assert(d[0][1][1][1] == 5, "d[0][1][1][1] must be 5");
	assert(d[0][1][1][2] == 6, "d[0][1][1][2] must be 6");

	assert(d[1][0][0][0] == 1, "d[1][0][0][0] must be 1");
	assert(d[1][0][0][1] == 2, "d[1][0][0][1] must be 2");
	assert(d[1][0][0][2] == 3, "d[1][0][0][2] must be 3");
	assert(d[1][0][1][0] == 4, "d[1][0][1][0] must be 4");
	assert(d[1][0][1][1] == 5, "d[1][0][1][1] must be 5");
	assert(d[1][0][1][2] == 6, "d[1][0][1][2] must be 6");
	assert(d[1][1][0][0] == 1, "d[1][1][0][0] must be 1");
	assert(d[1][1][0][1] == 2, "d[1][1][0][1] must be 2");
	assert(d[1][1][0][2] == 3, "d[1][1][0][2] must be 3");
	assert(d[1][1][1][0] == 4, "d[1][1][1][0] must be 4");
	assert(d[1][1][1][1] == 5, "d[1][1][1][1] must be 5");
	assert(d[1][1][1][2] == 6, "d[1][1][1][2] must be 6");

	assert(d[2][0][0][0] == 1, "d[2][0][0][0] must be 1");
	assert(d[2][0][0][1] == 2, "d[2][0][0][1] must be 2");
	assert(d[2][0][0][2] == 3, "d[2][0][0][2] must be 3");
	assert(d[2][0][1][0] == 4, "d[2][0][1][0] must be 4");
	assert(d[2][0][1][1] == 5, "d[2][0][1][1] must be 5");
	assert(d[2][0][1][2] == 6, "d[2][0][1][2] must be 6");
	assert(d[2][1][0][0] == 1, "d[2][1][0][0] must be 1");
	assert(d[2][1][0][1] == 2, "d[2][1][0][1] must be 2");
	assert(d[2][1][0][2] == 3, "d[2][1][0][2] must be 3");
	assert(d[2][1][1][0] == 4, "d[2][1][1][0] must be 4");
	assert(d[2][1][1][1] == 5, "d[2][1][1][1] must be 5");
	assert(d[2][1][1][2] == 6, "d[2][1][1][2] must be 6");

	assert(e[0][0] == 1, "b[0][0] must be 1");
	assert(e[0][1] == 2, "b[0][1] must be 2");
	assert(e[0][2] == 3, "b[0][2] must be 3");
	assert(e[1][0] == 4, "b[1][0] must be 4");
	assert(e[1][1] == 5, "b[1][1] must be 5");
	assert(e[1][2] == 6, "b[1][2] must be 6");

	assert(abs(a[0] - 1) < delta, "a[0] must be 1");
	assert(abs(a[1] - 2) < delta, "a[1] must be 2");
	assert(abs(a[2] - 3) < delta, "a[2] must be 3");

	assert(abs(g[0][0] - 2) < delta, "g[0][0] must be 2");
	assert(abs(g[0][1] - 3) < delta, "g[0][1] must be 3");
	assert(abs(g[0][2] - 4) < delta, "g[0][2] must be 4");
	assert(abs(g[1][0] - 5) < delta, "g[1][0] must be 5");
	assert(abs(g[1][1] - 6) < delta, "g[1][1] must be 6");
	assert(abs(g[1][2] - 7) < delta, "g[1][2] must be 7");

	assert(abs(h[0][0] - 1) < delta, "h[0][0] must be 1");
	assert(abs(h[0][1] - 2) < delta, "h[0][1] must be 2");
	assert(abs(h[0][2] - 3) < delta, "h[0][2] must be 3");
	assert(abs(h[1][0] - 2) < delta, "h[1][0] must be 2");
	assert(abs(h[1][1] - 3) < delta, "h[1][1] must be 3");
	assert(abs(h[1][2] - 4) < delta, "h[1][2] must be 4");
	assert(abs(h[2][0] - 5) < delta, "h[1][0] must be 5");
	assert(abs(h[2][1] - 6) < delta, "h[1][1] must be 6");
	assert(abs(h[2][2] - 7) < delta, "h[1][2] must be 7");
}