float f(int);

int main()
{
	float a = f(5);

	assert(abs(a - -0.958924) < 0.000001, "a must be -0.958924");

	return 0;
}

float f(int a)
{
	return sin(a);
}