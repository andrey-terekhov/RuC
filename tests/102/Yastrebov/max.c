void main()
{
	float a[10];
	int i;
	int k;
	float max;
	getid (a);
	k=1;
	max=a[0];
	for(i=1;i<10;i++)
	{
		if (max<a[i])
		{
			max=a[i];
			k=i;
		}
	}
	printid (max);
	printid (k);
}