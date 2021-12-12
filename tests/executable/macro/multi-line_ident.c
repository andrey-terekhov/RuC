#define A 1\
1
#macro B
1\
2
#endm

int main()
{
	assert(A == 11, "\"1\\\n1\" -> \"11\"");
	assert(B == 12, "\"1\\\n2\" -> \"12\"");

	return 0;
}
