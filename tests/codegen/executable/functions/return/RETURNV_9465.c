void main()
{
    int y = 4;

    assert(y == 4, "y must be 4");

    y += abs(-4);

    assert(y == 8, "y must be 8");
}
