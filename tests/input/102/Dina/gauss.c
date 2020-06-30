void main ()
{
    int N, i, j, d;
    float S, t;
    getid(N);
    printid(N);
    {
        float A[N][N], B[N], X[N];
        getid(A);
        printid(A);
        getid(B);
        printid(B);
        for(i = 0; i < N; i++)
            for (j = i+1; j < N; j++)
            {
	   t = A[j][i]/A[i][i];
	   for(d = 0; d < N; d++)
	       A[j][d] = A[j][d]-A[i][d]*t;
	   B[j] = B[j]-B[i]*t;
	}
        for(i = N-1; i >= 0; i--)
        {
            S = B[i];	
	for (j = i+1; j < N; j++)
	    S = S-A[i][j]*X[j];
	X[i] = S/A[i][i];
         }
         printid(X);
     }
}