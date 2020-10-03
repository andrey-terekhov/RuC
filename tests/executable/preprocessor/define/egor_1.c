#define s(a, b, c) a + b - c
#define k(a, b) a + b
#define l(a, b) a - k(a, k(a, k(a, b)))
#define E 9


void main()
{
	int a = s(E, 6, 10);
	int b = k(1, k(3, 4));
	int c = l(11, 12);

	assert(a == 5, "Must be 5");
	assert(b == 8, "Must be 8");
	assert(c == 34, "Must be 34");

	c = s(a, b, c);
	assert(c == -21, "Must be -21"); 
} 