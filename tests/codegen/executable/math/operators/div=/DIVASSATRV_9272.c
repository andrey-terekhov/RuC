int main() 
{
	float a[2] = {2, 3};
	a[1] /= a[0];

	assert(a[0] == 2.000000, "a[0] must be 2.000000");
	assert(a[1] == 1.500000, "a[1] must be 1.500000");

	return 0;
}