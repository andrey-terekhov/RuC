int d[2] = {72, 1};

void MAIN()
{
    float r;
    d[1] |= 70;

    assert(d[0] == 72, "d[0] must be 72");
    assert(d[1] == 71, "d[1] must be 71");
}
