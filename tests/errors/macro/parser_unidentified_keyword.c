int a = 0;
#wrongkw a = 1;

int main()
{
	assert(a, "recovery is not working");
	return 0;
}
