int f(int i)
{
    return i;
}
void main()
{
    int i;
    float a[5];
    for (i=0; i<5; ++i)
    {
        a[i] = f(i+2)/3;
        printf("%f", a[i]);  // 0.67 1
    }
}


