void main ()
  {
	int i, j, n,t ;
	getid(n);
	{
		int A[n];
		getid(A);
		for(i=0; i<n-1;i++)
            for(j=0; j<n-i-1;j++)
                if( A[j] > A[j+1] )
                {
                    t=A[j];
                    A[j]=A[j+1] ;
                    A[j+1]=t;
                }
		printid(A);
	}		               
  }
