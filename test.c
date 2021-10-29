int a = 1;
#macro A 
assert(a, "recovery is not working");
assert(a, "\n");
#endm 

int main()
{
	A
	return 0;
}
