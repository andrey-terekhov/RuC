float f(int);

int main()
{
	float a = f(5);
	printid(a);
	return 0;
}

float f(int a)
{
	return sin(a);
}