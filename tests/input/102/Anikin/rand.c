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
    int i,j,b;
    int N[50];
    float a=5;
    int q; // Количество точек откоторых мы берём среднее
    int n; // Количество бросков
    getid(q);
    getid(n);
    for ( i=0;i<50;i++)
    	N[i]=0;  
    for ( j=0; j<n; j++)
      {
            a=f(q) * 50;
            b= 50-C(a);
            N[b]++;
       //   print(b);
      }
        print(N);
 
}
