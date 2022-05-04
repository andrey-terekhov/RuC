void main()
{
    int i = 135e2;
    float x = 3.;

    assert(i == 135e2, "i must be 135e2");
    assert(x == 3., "x must be 3.");

    x = 3000000000;

    assert(x == 3000000000, "x must be 3000000000");
}