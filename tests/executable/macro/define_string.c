#define A "string"

#define B 0
#define B "string"

int main()
{
	assert(A[1] == 't', "define string");
	assert(B[1] == 't', "int redefine string");
	return 0;
}
