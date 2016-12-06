int A[5];

void trans (float(*g)(float))
{
    float a;
    int i;
    for (i=0; i<5; i++)
        print (g(A[i]));
}

float g1 (float x)
{
    return sqrt(x);
}
float g2 (float x)
{
    return sin(x);
}
float g3 (float x)
{
    return cos(x);
}

void main ()
{
    getid(A);
    trans (g1);
    trans (g2);
    trans (g3);
}





