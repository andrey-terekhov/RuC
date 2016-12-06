int i, ind;
float a[3]={0.010301, 22.01, 3.07};
float max=0;
void main()
{
	for (i=0; i<3; i++)
	{
		if (max<a[i])
		{
		max=a[i];
		ind=i+1;
		}
	}
	printid(max);
	printid(ind);
}