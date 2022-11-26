void main()
{
    struct point{int x; int y;};
    struct lines{int a[2]; struct point b[1];} l[1][2];
    l[0][0].a[0] = 13;
    l[0][1].b[0].y = 14;

    assert(l[0][0].a[0] == 13, "l[0][0] must be 13");
    assert(l[0][1].b[0].y == 14, "l[0][1].b[0].y must be 14");
}

