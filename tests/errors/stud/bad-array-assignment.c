

void main() 
{
	int x[-10]; // it'll not complite in C
	x = 2;      // it'll also not compile, but independently (e.g. with: int x[1] = { 2 }; )
	printid(x); // here it compiles and crashes
}