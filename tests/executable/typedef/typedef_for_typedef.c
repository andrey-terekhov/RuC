typedef int name;
typedef name name2;
typedef name2 name3;

int main()
{
	name tmp = 1;
	name2 tmp2 = tmp;
	name3 tmp3 = tmp2;

	assert(tmp == 1, "tmp != 1");
	assert(tmp2 == 1, "tmp2 != 1");
	assert(tmp3 == 1, "tmp3 != 1");
	return 0;
}
