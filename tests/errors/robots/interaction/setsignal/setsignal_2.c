float pi = 3.141592653589793;

int main()
{
	
	while (1) 
	{
		setsignal(1, { 22, 21 }, { 255, 255, 255, 0, 0, 255, 255 });
		
		setsignal(0, { 22, 21 }, { 1, 0, 0, 0 });
		
	}
	
	return 0;
}
