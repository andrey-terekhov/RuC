typedef struct
{
	int a;
	int b;
} name;

int main()
{
	name tmp;
	tmp.a = 2;
	tmp.b = 3;

	assert(tmp.a == 2, "field a != 2");
	assert(tmp.b == 3, "field b != 3");
	return 0;
}
