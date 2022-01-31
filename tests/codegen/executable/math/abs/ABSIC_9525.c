void main()
{
    int x = abs(4);

    assert(x == 4, "x must be 4");

    x = abs(-4);

    assert(x == 4, "x must be 4");
}