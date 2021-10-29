int a = 1;
#wrongkw a = 0;

int main()
{
	assert(a, "recovery is not working");
	return 0;
}
