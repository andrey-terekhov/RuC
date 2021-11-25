int main() 
{
	float a = 2, b = 3;
	a /= b;

	assert(abs(a - 0.666667) < 0.000001, "a must be 0.666667");

	return 0;
}