float f(int i)
{
    return i;
}
void main()
{
    register int i;
    float a[5];
    int c = 2, d = 3;
    for (i=0; i<5; ++i)
    {
        a[i] = f(i)+2;
        printf("%f\n", a[i]);    // 2 3 4
        a[i] += f(i)+3;
        printf("%f\n", a[i]);    // 5 7 9
        c = c * d;
        printid(c);  // 6 18 54 162
    }
}


