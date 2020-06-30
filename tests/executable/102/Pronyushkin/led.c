float f (float x)
{
	return 7*sqrt (x);
}
void main ()
{
    int i, j;
    char A[50][50], B[50];
    for (i = 0; i < 50; i++)
    {
        for (j = 0; j < 50; j++)
        {
            A[i][j] = ' ';
            B[j] = A[i][j];
        }
        j = round (f(i));
        A[i][j] = 'x';
        B[j] = A[i][j];
        print (B);
    }
}
