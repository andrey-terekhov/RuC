float b[3], c, d;
int i, j;

void main()
{
    ++b[i];
    ++c;
    d = ++b[j];
    d = ++c;

    assert(b[0] == 2.000000, "b[0] must be 2.000000");
    assert(b[1] == 0.000000, "b[1] must be 0.000000");
    assert(b[2] == 0.000000, "b[2] must be 0.000000");
    assert(c == 2.000000, "c must be 2.000000");
    assert(d == 2.000000, "d must be 2.000000");
}