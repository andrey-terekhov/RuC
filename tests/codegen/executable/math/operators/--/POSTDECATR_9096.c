float f[1], e;
int j;

void main()
{
    e = f[j]--;

    assert(e == 0.000000, "e must be 0.000000");
}
