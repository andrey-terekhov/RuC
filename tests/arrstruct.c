void main()
{
    struct point{int x; int y;};
    struct lines{int a[2]; struct point b[1];} l[2][3];
    l[0][2].a[0] =13;
    l[1][1].b[0].y = 14;
    printid(l);
}

