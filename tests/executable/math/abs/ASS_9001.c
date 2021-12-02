void main()
{
    int y = 4;

    assert(y == 4, "y must be 4");

    y = y = abs(-y);

    assert(y == 4, "y must be 4");
}
