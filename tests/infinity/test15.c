int a,   b;
void main()
{
        goto l3;
        a = 1;
        goto l3;
    l2:
        a = 2;
    l3:
        a = 3;
        goto l2;
    l4:
        a = 4;
        goto l2;
}
