void main()
{
    float a = 3.14, b = 2.72, c;
    c = a + b;

    assert(abs(c - 5.860000) < 0.000001, "c must be 5.860000");
}