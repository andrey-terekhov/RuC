char *a, b[3]={'0','1','2'}, a1='7';
void f(char *aa, char bb[])
{
    bb[1] = *aa;
    
}
void MAIN()
{
    a = &a1;
    print(*a);    // 7
    f(&a1, b);
    printid(b);   // 0 7 2
}