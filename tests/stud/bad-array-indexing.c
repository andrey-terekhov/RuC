

void main() 
{
	int x[2] = { 3, 4 };
	// incorrect parenthesis sequence!!!
	x[1 - ] ] = 5; // compiles! "-" complements "]" somehow...
	printid(x); 
}