int a = 1, b = 2;
int f(), q(int, int);

int q(int x, int y)
{
    return x+y;
}


void main()
{
    assert((f() + 3) == 4, "Must be 4");
    assert(q(a + b, 2) == 5, "Must be 5");
}

int f()
{
    return a;
}