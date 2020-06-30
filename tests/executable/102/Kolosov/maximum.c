float A[ 10 ] = {1, 3.41, 1.34, 6.904, 1, 0.1, -0.1, 1000, -2, 3.141592653}, max;
int i, ind;

void main()
{
    max = A[0];
    for (i = 1; i < 10; i++)
        if (A[ i ] > max)
        {
            max = A[ i ];
            ind = i;
        }
    printid (max);
    printid (ind);
}
