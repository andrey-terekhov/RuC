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
	print(f(3));
    printid(m);
	return 0;
}
