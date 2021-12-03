float j = 1, k;

void main()
{
    k = ++j;

    assert(k == 2.000000, "k must be 2.000000");
}
