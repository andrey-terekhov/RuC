int a = 1;
/* */ #define a 0

#include /*comment*/"file.c"

int main()
{
	assert(a, "lexeme before kw");

	return 0;
}
