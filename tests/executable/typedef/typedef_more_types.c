typedef struct
{
	int a;
	int b;
} name;

typedef int name2;
typedef char name3;

int main()
{
	name tmp;
	name2 tmp2 = 1;
	name3 tmp3 = 'a';
	tmp.a = 2;
	tmp.b = 3;

	assert(tmp.a == 2, "");
	assert(tmp.b == 2, "");
	assert(tmp2 == 1, "");
	assert(tmp3 == 'a', "");
	return 0;
}
