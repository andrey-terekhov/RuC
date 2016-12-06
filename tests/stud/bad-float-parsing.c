

void main() 
{
	// will not compile in C
	float x[3] = { 5e, 5e-, 5e+ }; // must be a digit after exponential sign
	printid(x);
}