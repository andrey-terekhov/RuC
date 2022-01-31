int a = 9, b[3] = {7, 8, 9};
float c = 1.2e1, d[2] = {3.14, 2.72};
void MAIN()
{
    a += b[2];

    assert(a == 18, "a must be 18");

    c -= d[0];

    assert(abs(c - 8.86) < 0.000001, "c must be 8.86");

    b[0] += 10;

    assert(b[0] == 17, "b[0] must be 17");
    assert(b[1] == 8, "b[1] must be 8");
    assert(b[2] == 9, "b[2] must be 9");

    d[1] -= 0.70;

    assert(abs(d[0] - 3.14) < 0.000001, "d[0] must be 3.14");
    assert(abs(d[1] - 2.02) < 0.000001, "d[1] must be 2.02");
}
