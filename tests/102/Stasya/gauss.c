void main ()
{
int N, i, j, k;
float Sum, t;
getid(N);
printid(N);
	{
	float A[N][N], B[N], X[N];
	getid(A);
	printid(A);
	getid(B);
	printid(B);
	for(i=0; i<N; i++)
		for (j=i+1; j<N; j++)
		{
			t=A[j][i]/A[i][i];
			for(k=0; k<N; k++)
				A[j][k]=A[j][k]-A[i][k]*t;
			B[j]=B[j]-B[i]*t;
		}
	for(i=N-1; i>=0; i--)
	{
		Sum=B[i];	
		for (j=i+1; j<N; j++)
			Sum=Sum-A[i][j]*X[j];
		X[i]=Sum/A[i][i];
	}
	printid(X);
	}
}