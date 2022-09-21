int a = 11, b = 22, c = 0;

void main()
{
    int d = 1 < c ? a : c;

    assert(d == 0, "d must be 0");

    b = (14 > a ? a + 4 : a - 5 + 10) + (c <= a ? a : c);

    assert(b == 26, "b must be 26");
}
