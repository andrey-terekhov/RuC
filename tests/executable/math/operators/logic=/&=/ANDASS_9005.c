int d = 72, r;

void MAIN()
{
    r = d &= 70;

    assert(d == 64, "d must be 64");
}