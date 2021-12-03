void main()
{
    float xx = 2.3, yy = 2.51;
    int x = round(xx), y = round(yy);

    assert(x == 2, "x must be 2");
    assert(y == 3, "y must be 3");

    x = round(-xx);

    assert(x == -2, "x must be -2");
    
    y = round(-yy);

    assert(y == -3, "y must be -3");
}
