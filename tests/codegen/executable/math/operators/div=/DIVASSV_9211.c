int main() 
{
	int a = 2, b = 3;
	a /= b;

	assert(a == 0, "a must be 0");

	return 0;
}