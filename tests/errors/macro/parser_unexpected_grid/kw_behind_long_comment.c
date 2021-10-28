int a = 1; /*long
comment*/#define a 0

int main()
{
	assert(a, "recovery is not working");
	return 0;
}
