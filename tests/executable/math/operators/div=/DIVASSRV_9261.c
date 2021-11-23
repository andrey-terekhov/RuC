int main() 
{
	float a = 2, b = 3;
	a /= b;

	assert(a == 0.666667, "a must be 0.666667");

	return 0;
}