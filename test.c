int a = 1;
#macro A gg
jk
#endm

int main()
{
	A
	assert(a, "recovery is not working");
	return 0;
}
