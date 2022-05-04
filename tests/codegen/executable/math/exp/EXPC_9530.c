int main() 
{
	float a = 2, b = 3;
	a += exp (b);

	assert(abs(a - 22.085537) < 0.000001, "a must be 22.085537");

	return 0;
}
