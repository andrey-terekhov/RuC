int a = 2, b = 3;

void main()
{
    int c = 4;
    b = 5 + a - c + a;

    assert(b == 5, "b must be 5");

    b = a += c;

    assert(a == 6, "a must be 6");
    assert(b == 6, "b must be 6");

    a = a * b + c / 2;

    assert(a == 38, "a must be 38");
}
