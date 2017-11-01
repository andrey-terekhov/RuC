void main ()
{
	int n;
	getid (n);
	{ 
		int i,j,x=0;
		int A [n][9*n+1];
		for(i =0; i<n; i++)
			A[i][0]=1; 
		for(i =0; i<10; i++)
			A[0][i]=1; 
		for(i =10; i<9*n; i++)
			A[0][i]=0;
		for(i =1; i<n-1; i++)
			for(j =1; j<9*(i+1)+1; j++) 
			{
				A[i][j] = A[i-1][j]+ A[i][j-1];
				if (j>9) 
					A[i][j]-= A[i-1][j-10];
			}
		for(j =1; j<9*n; j++) 
		{
			A[n-1][j]= A[n-2][j]+ A[n-1][j-1];
			if (j>9) 
				A[n-1][j]-= A[n-2][j-10];
			x+=A[n-1][j]*A[n-1][j];
		}

	print(x);

	}
}