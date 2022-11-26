int id(int);

void main()
{
	int a = id(1);
	int b = id(a);
	int c = id(b);
	int d = id(c);
	assert(d == 1, "d should be 1");
}

int id(int x) { return x; }