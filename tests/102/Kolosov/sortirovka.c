float A[ 10 ] = {1, 23.3, 32.2, -13.1, 2, 2, 3.14, 2.71, 2.31, 1.32}, c;
int i, j ;

void main()
{
    for (i = 0; i < 9; i++)
        for (j = 0; j < 9-i; j++ )
            if (A[ j ] > A[ j+1 ])
            {
                c = A[ j ];
                A[ j ] = A[ j+1 ];
                A[ j+1 ] = c;
            }
    printid (A);    
}