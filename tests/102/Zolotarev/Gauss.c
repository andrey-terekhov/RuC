void main ()
{     
	int n, i, j, k;
	getid (n);
		{
		float c;
		float a[n][n];
		float b[n];
		float x[n];
		getid(a);
		printid(a);
     		getid(b);
		printid(b);
		for(i=0; i<n; i++)
			for (j=i+1; j<n; j++)
			{ 
				c=a[j][i] /a[i][i];
					for(k=0; k<n; k++)
					a[j][k] -= a[i][k] * c; 
				b[j] -= b[i] * c; 
			}       
		for(i=n-1; i>=0; i--)
		{	
			for (j=i+1;j<n;j++)
				b[i] -= a[i][j] * x[j];
			x[i] = b[i]/a[i][i]; 
		}
		printid(x); 
		}
}
