float d[2] = {3.14, 2.72};

void MAIN()
{
    d[1] -= 0.70;

    assert(d[0] == 3.140000, "d[0] must be 3.140000");
    assert(d[1] == 2.020000, "d[1] must be 2.020000");
}
