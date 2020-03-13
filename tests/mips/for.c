int f(int i)
{
    return i;
}
void main()
{
    register int i, ai;
    int a[5];
    for (i=0; i<5; ++i)
    {
        ai = a[i] = f(i)+2;
        printf("%i\n", ai);   // 2 3 4 5 6
        ai = a[i] += f(i)+3;
        printf("%i\n", ai);   // 5 7 9 11 13
    }
}


