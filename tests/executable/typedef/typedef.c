typedef int tint;

int main()
{
	tint a = 2;
	print ("a  2");
	printid(a);

	int b = 3;
	print ("b  3");
	printid(b);

	assert(a == 2, "a != 2");
	assert(b == 3, "b != 3");
	return 0;
}
