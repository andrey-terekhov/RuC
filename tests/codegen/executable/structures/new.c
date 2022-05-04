struct point
{
	double x;
	double y;
};
double m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    
    
struct point f(int i)
{
	struct point result = {i + 1, i * 2};
	return result;
}


int main()
{
	assert(abs(f(3).x - 4) < 0.000001, "f(3).x must be 4");
	assert(abs(f(3).y - 6) < 0.000001, "f(3).y must be 6");

	assert(abs(m[0][0] - 1) < 0.000001, "m[0][0] must be 1");
	assert(abs(m[0][1] - 2) < 0.000001, "m[0][1] must be 2");
	assert(abs(m[0][2] - 3) < 0.000001, "m[0][2] must be 3");
	assert(abs(m[1][0] - 4) < 0.000001, "m[1][0] must be 4");
	assert(abs(m[1][1] - 5) < 0.000001, "m[1][1] must be 5");
	assert(abs(m[1][2] - 6) < 0.000001, "m[1][2] must be 6");

	return 0;
}
