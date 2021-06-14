void main ()
{
    char A[10];
    int B[5] = {0, 0, 0, 0, 0};
    int i;

    getid(A);
    printid(A);

    for (i = 0; i < 10; i++)
        switch (A[i])
    {
            case 'a':
                B[0]++;
            break;
            
            case 'b':
                B[1]++;
            break;
            
            case 'c':
                B[2]++;
            break;
            
            case 'd':
                B[3]++;
            break;
            
            case 'e':
                B[4]++;
            break;
    }
    printid(B);
}
