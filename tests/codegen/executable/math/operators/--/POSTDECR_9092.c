float r, d;
void main()
{
    r = d--;

    assert(d == -1.000000, "d must be -1.000000");
    assert(r == 0.000000, "r must be 0.000000");
}