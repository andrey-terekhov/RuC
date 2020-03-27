
void main()
{
    int N = 3, i = 1;
    int a[N];
    while (i < N)
    {
        printf("%i\n", i%3);
        a[i] = i / (i % 3);
        printf("%i\n", a[i]);
        ++i;
        printid(i);
    }
    
}


