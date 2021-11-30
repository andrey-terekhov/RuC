float f[1], e;
int j;

void main()
{
    e = --f[j];

    assert(e == -1.000000, "e must be -1.000000");
}
