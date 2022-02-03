void main()
{
    float a = 2.3046922654015389;
    int b;
    b = round(a);

    assert(abs(a - 2.304692654015389) < 0.000001, "a must be 2.304692654015389");
    assert(b == 2, "b must be 2");

    a = -2.3046922654015389;
    b = round(a);

    assert(abs(a - -2.304692654015389) < 0.000001, "a must be -2.304692654015389");
    assert(b == -2, "b must be -2");
}
