int d[2] = {3, 72};

void MAIN()
{
    d[1] -= 70;

    assert(d[0] == 3, "d[0] must be 3");
    assert(d[1] == 2, "d[1] must be 2");
}
