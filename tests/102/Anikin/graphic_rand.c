float f (int q)
{     
            	float s=0;
	int i;
	for (i=0;i<q;i++)
	s+=rand();
	s/=q;
    	return s;
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
    char E[50][50], A[50];
    int i,j,b;
    int N[50];
    float a=5;
    int q; // Количество точек откоторых мы берём среднее
    int n; // Количество бросков
    getid(q);
    getid(n);
    for ( i=0;i<50;i++)
     {         
      	  N[i]=0;
      	  for (j=0;j<50;j++)
    	             E[i][j]='-';
      }  
     

            
     for ( j=0; j<1000; j++)
      {
            a=f(q) * 50;
            b= 50-C(a);
            N[b]++;
       //   print(b);
      }
        print(N);
    for ( i=0;i<50;i++)
    {
    N[i]=49-N[i];
    E[i][N[i]]='a';
    }     
    for ( i=0;i<50;i++)
    {
        for ( j=0;j<50;j++)
            A[j]=E[j][i];
       print(A);
       print("\n");
       
    }
}

