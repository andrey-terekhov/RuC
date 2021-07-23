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

	assert(tmp.a == 2, "");
	assert(tmp.b == 2, "");
	return 0;
}
