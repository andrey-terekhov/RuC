void main()
{
    struct point{int x; int y;};
    struct point l[2];
    l[0] = {13, 14};
    l[1] = l[0];
    printid(l);
}

