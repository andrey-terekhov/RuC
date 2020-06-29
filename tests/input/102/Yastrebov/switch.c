void main()
{	
	int i;
	char s[20]; 
	int a[5];
	getid (s);
	printid (s);
	for (i=0;i<20;i++) 
	{
		switch (s[i])
		{
			case 'а' : 
				a[0]++;
				break;
			case 'б' : 
				a[1]++;
				break;
			case 'в' : 
				a[2]++;
				break;
			case 'г' :
				 a[3]++;
				break;
			case 'д' : 
				a[4]++;
				break;	
		}
	}
		print (a);
		
}
