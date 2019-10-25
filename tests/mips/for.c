register int i;
register float r;
int a[5];
int f(int i)
{
    return i;
}
void main()
{
    int c, d;
    for (i=0; i<5; ++i)
    {
        a[i] = f(i)+2;
        a[i] += f(i)+3;
        c = c * d;
    }
}


