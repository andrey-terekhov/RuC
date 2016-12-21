void main()
{
 int i,k,t,u,n,z;
 float p,m;
 print("¬ведите число уравнений");
 getid(n);
  {
   float a[n][n+1];
   float b[n];
   print("¬ведите матрицу");
   getid(a);
   for (z=0;z<n;z++)   
    {
     for (i=z;i<n;i++) 
      if (a[i][z]!=0)		
       { 
           for (t=0;t<n+1;t++)
           {
               p=a[i][t];
               a[i][t]=a[z][t];
               a[z][t]=p;
           }
       }
     for(k=z+1;k<n;k++) 
      { 
       m=a[k][z]/a[z][z];
       for (i=z;i<n+1;i++) 
        {
            a[k][i]=a[k][i]-m*a[z][i];
        }
      }			
  }
 p=0;
 b[n-1]=a[n-1][n]/a[n-1][n-1];
 for (i=n-2;i>=0;i--) 
  {
   p=a[i][n];
   for (k=i+1;k<n;k++) p=p-a[i][k]*b[k];
   b[i]=p/a[i][i];		}				
   print("–ешени€\n");
   print(b);
  }
}
//Miller A.
