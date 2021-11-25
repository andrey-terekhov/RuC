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
	assert(f(3).x == 4, "f(3).x must be 4");
	assert(f(3).y == 6, "f(3).y must be 6");

	assert(m[0][0] == 1, "m[0][0] must be 1");
	assert(m[0][1] == 2, "m[0][1] must be 2");
	assert(m[0][2] == 3, "m[0][2] must be 3");
	assert(m[1][0] == 4, "m[1][0] must be 4");
	assert(m[1][1] == 5, "m[1][1] must be 5");
	assert(m[1][2] == 6, "m[1][2] must be 6");

	return 0;
}
