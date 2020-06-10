int i, j, k, m, p;

void main()
{
   int N;
   getid (N);
   {
     float A [N][N], B[N], X[N];
     float c;
     getid (A);
     getid (B);
     for (p = 0; p < N; p++)                                             //приводим к треугольному виду
          {
           for (i = p; i < N; i++)
               {
                c = A[ i ][ p ];
                B[ i ] /= c;
                for (j = p; j < N; j++)
                      A[ i ][ j ] /= c;
                }
             for (k = (p+1); k < N; k++)
                 {
                  for (m = p; m < N; m++)
                      A[ k ][ m ] -= A[ p ][ m ];
                   B[ k ] -= B[ p ];
                  }
            }                                                                   //закончили приводить к треугольному виду и делаем обратные ходы
     X[ N-1 ] = B[ N-1 ];                                               
     for (j = N-2; j > -1; j--)
         {
          for (i = N-1; i > j; i--)
              A[ j ][ i ] *= X[ i ];
          c = 0;
          for (k = N-1; k > j; k--)
              c += A[ j ][ k ];
          X[ j ] = B[ j ] - c;
         }
    printid(X);
   }
}
