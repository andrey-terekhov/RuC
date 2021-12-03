int a = 11,   b = 22, c = 0;

float b1 = 3.14;

void main()
{
    int d = 14 < a ? a : 2 > b ? b : c;

    assert(d == 0, "d must be 0");

    b1 = 14 < a ? a : 2 < b ? b : b1;

    assert(b1 == 22, "b1 must be 22");

    b1 = 1 < a ? a : 2 > b ? b1 : b;

    assert(b1 == 11, "b1 must be 11");
}
