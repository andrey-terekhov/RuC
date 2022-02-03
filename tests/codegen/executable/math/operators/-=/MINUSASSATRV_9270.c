float d[2] = {3.14, 2.72};

void MAIN()
{
    d[1] -= 0.70;

    assert(abs(d[0] - 3.140000) < 0.000001, "d[0] must be 3.140000");
    assert(abs(d[1] - 2.020000) < 0.000001, "d[1] must be 2.020000");
}
