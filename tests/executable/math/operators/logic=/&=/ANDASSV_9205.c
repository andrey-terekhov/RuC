int d =  72;

void MAIN()
{
    d &= 70;

    assert(d == 64, "d must be 64");
}
