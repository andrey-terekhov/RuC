float a = 2, b = 3;

void main()
{
    int c = 4;

    assert(c == 4, "c must be 4");

    b = 5 + a - c + a;

    assert(b == 5, "b must be 5");

    b = a += c;

    assert(a == 6, "a must be 6");
    assert(b == 6, "b must be 6");
    a = a * b + c / a;

    assert(abs(a - 36.666668) < 0.00001, "a must be 36.666668");
}