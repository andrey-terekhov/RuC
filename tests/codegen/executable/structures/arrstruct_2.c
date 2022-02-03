void main()
{
    struct point{int x; int y;};
    struct point l[2];
    l[0] = {13, 14};
    l[1] = l[0];

    assert(l[0].x == 13, "l[0].x must be 13");
    assert(l[0].y == 14, "l[0].y must be 14");
    assert(l[1].x == 13, "l[1].x must be 13");
    assert(l[1].y == 14, "l[1].y must be 14");
}

