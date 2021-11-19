int main() 
{
	float a = 2, b = 3;
	a = log10(b);

	assert(a == 0.477121, "a must be 0.477121");

	return 0;
}