int d = 72;

void MAIN()
{
    float r;
    r = d |= 70;

    assert(d == 78, "d must be 78");
}
