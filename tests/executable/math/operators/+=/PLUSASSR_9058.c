float a = 2, b = 3;

void main()
{
    int c = 4;
    b = a += c;

    assert(b == 6.000000, "b must be 6.000000");
}