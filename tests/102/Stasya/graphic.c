float f (float x)
{
	return x/2;
	//return x*2;
	//return sin(x);
}
int C (float x)
{
	int k=-10;
	while (k<x)
	k++;
	return k;
}
void main ()
{
	char ekr[50][50], t[50];
	int i, j, y;
	float x;
	for (i=0; i<50; i++)
	{
		for (j=0; j<50; j++)
		ekr[i][j]=' ';
		
	x=i;
	x=f(x/50)*50;
	if ((x>-1)&&(x<50))
	{
		y=50-1-C(x);
		ekr[i][y]='ё';
	}
	}
	for (i=0; i<50; i++)
	{
		for (j=0; j<50; j++)
		t[j]=ekr[j][i];
		print(t);
		print('\n');
	}
}