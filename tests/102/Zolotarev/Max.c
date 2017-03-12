void main()
{
	int i, ind, max;
	int a[10];
	getid(a);
	max=a[0];
	ind=0;
	for (i=1; i<9; i++);
		if (a[i]>max)
		{
			ind=i;
			max=a[i];
		}
	printid(max);
	printid(ind);
}