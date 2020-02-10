register int i;
float a[5];
float f(int i)
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


