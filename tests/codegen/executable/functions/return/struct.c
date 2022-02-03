struct point{float x; float y;} p = {1, 2};

struct point f(struct point pa, struct point pb, struct point pc)
{
    assert(pa.x == 1, "pa.x must be 1");
    assert(pa.y == 2, "pa.y must be 2");

    pb.x = 1.1;
    pb.y = 2.2;

    assert(pb.x == 1.1, "pb.x must be 1.1");
    assert(pb.y == 2.2, "pb.y must be 2.2");

    return pb;
}

void main()
{
    struct point pd;

    assert(p.x == 1, "p.x must be 1");
    assert(p.y == 2, "p.y must be 2");

    pd = f(p, p, p);

    assert(pd.x == 1.1, "pd.x must be 1.1");
    assert(pd.y == 2.2, "pd.y must be 2.2");
}