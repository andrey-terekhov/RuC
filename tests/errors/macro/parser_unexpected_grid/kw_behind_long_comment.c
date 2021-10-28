int a = 0; /*long
comment*/#define a 1

int main()
{
	assert(a, "recovery is not working");
	return 0;
}
