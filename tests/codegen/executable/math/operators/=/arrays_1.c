int a[3] = {1,2,3};
int b[4] = {1,2,3,4};

void f()
{
    b[3] = a[2];
}


void main()
{
    assert(a[0] == 1, "a[0] must be 1");
    assert(a[1] == 2, "a[1] must be 2");
    assert(a[2] == 3, "a[2] must be 3");

    assert(b[0] == 1, "b[0] must be 1");
    assert(b[1] == 2, "b[1] must be 2");
    assert(b[2] == 3, "b[2] must be 3");
    assert(b[3] == 4, "b[3] must be 4");

    f();

    assert(b[0] == 1, "b[0] must be 1");
    assert(b[1] == 2, "b[1] must be 2");
    assert(b[2] == 3, "b[2] must be 3");
    assert(b[3] == 3, "b[3] must be 3");
}