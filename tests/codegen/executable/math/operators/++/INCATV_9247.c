int b[3], c, d;
int i, j;

void main()
{
    ++b[i];
    ++c;
    d = ++b[j];
    d = ++c;

    assert(b[0] == 2, "b[0] must be 2");
    assert(b[1] == 0, "b[1] must be 0");
    assert(b[2] == 0, "b[2] must be 0");
    assert(c == 2, "c must be 2");
    assert(d == 2, "d must be 2");
}
