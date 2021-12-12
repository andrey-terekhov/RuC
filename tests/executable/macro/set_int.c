#define A 0
#define B 0
#define C 0
#define D 0
#define E 1

#set A 1
#set B 1 + 2
#set C A + B
#set D C - A - B
#set E E + A + B


int main()
{
	assert(A == 1, "set int");
	assert(B == 3, "set int + int");
	assert(C == 4, "set ident + ident");
	assert(D == 0, "set ident - ident - ident");
	assert(E == 5, "set origin_ident + ident + ident");

	return 0;
}
