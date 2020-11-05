void main ()
{
	int n=0,flag=0, m=1, j = 0;
	float x = 1, y = 0, z = 0.1;
	char a [5], c;
	getid (a);
    printid(a);
    c = a[0];
	if (c == '-')
	{
		x = -1;
		c = a[++j];
	}
	if (( c <= '9' )&&(c >='0'))
		flag = 1;
	while (c <= '9' && c>='0' && j < 5)
	{
		n=n*10+c-'0';
		m=0;
		c = a[++j];
	}
	if(flag=1)
		if (c==' ')
		{
			print ("int nimber: ");
			if (x==-1)
				n*=-1; 
			printid(n);
		}
	else if((c=='.')||(c==','))
	{ 
		c = a[++j];
		while (c <= '9' && c>='0' && j < 5)
		{
			y += (c-'0')*z;
			z*=0.1;
			c = a[++j];
		}
        y += n;
                  print("real number :");
                  printid(y);
                  
	}
}
