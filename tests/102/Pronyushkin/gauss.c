void main ()
{
int N, i, j, k;
float t, S;
getid (N);
printid (N);
	{
	float A[N][N], B[N],X[N];
	getid (A); 
	printid (A);
	getid (B);
	printid (B);
for (i = 0; i < N; i++)
{
	for (j = i + 1; j < N; j++)
    	{
	t = A[j][i] / A[i][i];
             for (k = i; k < N; k++)
             {
             A[j][k] = A[j][k] - t * A[i][k];
             } 
             B[j] = B[j] - t * B[i]; 
             }
}
for (i = N - 1; i > -1; i--) 
{
t = 0;
	for (j = i + 1; j < N; j++)
	{
	S = A[i][j] * X[j];
	t = t + S; 
	}
X[i] = (B[i] - t) / A[i][i]; 
}
}
print (X);
}