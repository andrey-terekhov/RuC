void func(int a[], float b[])
{
	assert(a[0] == -1, "a[0] must be -1");
	assert(a[1] == -2, "a[1] must be -2");
	assert(a[2] == -3, "a[2] must be -3");

	assert(b[0] == 0.000000, "b[0] must be 0.000000");
	assert(b[1] == 0.000000, "b[1] must be 0.000000");
}

void main()
{
	func({ -1, -2, -3 }, { 0, 0 });
}
