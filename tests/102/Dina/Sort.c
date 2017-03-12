void main ()
{
    int i, j, N, tmp ;
    getid(N);
    {
        int A[N];
        getid(A);
            for(i = 0; i < N-1; i++)
                for(j = 0; j < N-i-1; j++)
                    if( A[ j ] > A[ j+1] )
                    {
                        tmp=A[ j ];
                        A[ j ]=A[ j+1];
                        A[j+1]=tmp;
                    }
        printid(A);
    }		               
}
