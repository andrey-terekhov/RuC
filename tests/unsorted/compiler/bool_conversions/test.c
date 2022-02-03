void main()
{
    int a, b, c;
    c = a && b || c;
    c = a || b && c;

    assert(c == 0, "c must be 0");
}