struct point{float x; float y;} p = {1, 2};

struct point f(struct point pa, struct point pb, struct point pc)
{
    assert(abs(pa.x - 1) < 0.000001, "p.x must be 1");
    assert(abs(pa.y - 2) < 0.000001, "p.y must be 2");

    pb.x = 1.1;
    pb.y = 2.2;

    assert(abs(pb.x - 1.1) < 0.000001, "pd.x must be 1.1");
    assert(abs(pb.y - 2.2) < 0.000001, "pd.y must be 2.2");

    return pb;
}

void main()
{
    struct point pd;

    assert(abs(p.x - 1) < 0.000001, "p.x must be 1");
    assert(abs(p.y - 2) < 0.000001, "p.y must be 2");

    pd = f(p, p, p);

    assert(abs(pd.x - 1.1) < 0.000001, "pd.x must be 1.1");
    assert(abs(pd.y - 2.2) < 0.000001, "pd.y must be 2.2");
}