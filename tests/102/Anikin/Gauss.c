void main ()
{     
  int N,i,j,a;
  float s,e;
 getid (N);
 printid(N);
   {
     float A[N][N], B[N], X[N];
     getid (A);
     printid(A);
     getid (B);
     printid(B);
     for(i=0;i<N;i++)
           for (j=i+1;j<N;j++)
           { e=A[j][i] /A[i][i];
               for(a=0;a<N;a++)
                  A[j][a] -= A[i][a] *e; 
            B[j] -= B[i] * e;
           }       
           for(i=N-1;i>=0;i--)
         {
           s=B[i];
         for (j=i+1;j<N;j++)
           s-=A[i][j]*X[j];
           X[i]=s/A[i][i];
          }
          printid(X);
     }
   }