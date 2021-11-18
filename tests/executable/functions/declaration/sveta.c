float f(int);

int main()
{
	float a = f(5);

	assert(a == -0.958924, "a must be -0.958924");

	return 0;
}

float f(int a)
{
	return sin(a);
}