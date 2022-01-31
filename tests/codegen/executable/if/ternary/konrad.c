void main()
{
    int a = 3, b = 5;
    float c = 3.14;
    c = (a > b) ? a : (c > 0) ? c : b;

    assert(c == 3.14, "c must be 3.14");
}