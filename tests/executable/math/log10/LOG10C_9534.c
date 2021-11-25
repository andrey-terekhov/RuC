int main() 
{
	float a = 2, b = 3;
	a = log10(b);

	assert(abs(a - 0.477121) < 0.000001, "a must be 0.477121");

	return 0;
}