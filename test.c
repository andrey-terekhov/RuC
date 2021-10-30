int a = 1;
#macro A 
#define B 1
assert(a, "recovery is not working");
assert(a, "\n");
#endm /**/n
int main()
{
	A
	B
	return 0;
}
