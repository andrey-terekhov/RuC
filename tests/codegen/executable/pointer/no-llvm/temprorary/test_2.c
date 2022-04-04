char *a, b[3] = {'0' ,'1', '2'}, a1 = '7';

void f(char *aa, char bb[])
{
    bb[1] = *aa;
}


void MAIN()
{
    a = &a1;

    assert(*a == '7', "*a must be '7'");

    f(&a1, b);

    assert(b[0] == '0', "b[0] must be '0'");
    assert(b[1] == '7', "b[1] must be '7'");
    assert(b[2] == '2', "b[2] must be '2'");
}