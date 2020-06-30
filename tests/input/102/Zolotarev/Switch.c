char c[10];

void main()
{
	int i;
	int a[5] = {0, 0, 0, 0, 0};
	getid(c);
	printid(c);
	for (i=0; i<10; i++)
	{		
	 	switch (c[i])
		{
			case 'а':
				a[0]++;
				break;
			case 'б':
				a[1]++;
				break;
			case 'в':
				a[2]++;
				break;
			case 'г':
				a[3]++;
				break;
			case 'д':
				a[4]++;
				break;
		}
	}
	printid(a);
}
			 