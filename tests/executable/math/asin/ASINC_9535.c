int main() 
{
	float a = 2, b = 0.3;
	a = asin(b);

	assert(abs(a - 0.304693) < 0.000001, "a must be 0.304693");

	return 0;
}