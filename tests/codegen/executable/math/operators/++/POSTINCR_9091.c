float j = 1, k;

void main()
{
    k = j++;

    assert(k == 1.000000, "k must be 1.000000");
}
