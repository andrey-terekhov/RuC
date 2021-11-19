int main() 
{
	float a = 2, b = 3;
	a += exp (b);

	assert(a == 22.085537, "a must be 22.085537");

	return 0;
}
