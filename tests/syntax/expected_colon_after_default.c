// Expected ':' after 'default'
void main()
{
	int b = 10;
	switch (b)
	{
		case 10:
			b++;
			break;
	
		default
			break;
	}
	
	printid(b);
}
