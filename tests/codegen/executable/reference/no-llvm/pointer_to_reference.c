void main()
{
    int x = 5;
    int &y = x;
    int *t;
    t = &y;
    print(*t);
    assert(*t == 5, "Pointer to reference error. *t must be 5");
    *t = 3;
    assert(x == 3, "Pointer to reference. x must be 3");
}