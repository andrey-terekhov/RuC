

void main() 
{
	int x = 5;
	{
		int x[1] = { x + x }; // before running, try to guess the out put!
		printid(x);
	}
	printid(x);
}