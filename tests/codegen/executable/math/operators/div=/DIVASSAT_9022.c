int main() 
{
	int a[2] = {2, 3};
	float r;
	r = a[1] /= a[0];

	assert(a[0] == 2, "a[0] must be 2");
	assert(a[1] == 1, "a[1] must be 1");

	return 0;
}
