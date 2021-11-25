int main() 
{
	float	a = 2, b = 3;
	a = log(b);

	assert(abs(a - 1.098612) < 0.000001, "a must be 1.098612");

	return 0;
}