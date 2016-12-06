float f (float x)
{
    return x*2;
    //return sin (x);
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
    int j,b,i;
    float a;
    for ( i=0;i<50;i++)
    {
        for (j=0;j<50;j++)
            E[i][j]=' ';
        a=i;
        a=f(a/50) * 50;
        if((a>-1) && (a<50))
        {
            b=50-1-C(a);
            // print(b);
            E[i][b]='@';
            
        }
        
    }
    for ( i=0;i<50;i++)
    {
        for ( j=0;j<50;j++)
            A[j]=E[j][i];
        print(A);
    }
}

