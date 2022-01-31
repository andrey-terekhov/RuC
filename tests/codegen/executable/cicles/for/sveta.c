int i, a = 0, j;
void main()
{
    for (i = 0, j = 2; i < 10; i++)
    {
        a += j + i;
    }

    assert(a == 65, "a must be 65");
}
