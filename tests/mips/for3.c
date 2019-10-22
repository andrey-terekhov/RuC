int i, a[5];
int f(int i)
{
    return i;
}
void main()
{
    for (i=0; i<5; ++i)
    {
        i = f(i+2)/3;
    }
}


