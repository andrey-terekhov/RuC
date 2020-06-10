struct point{float x; float y;} p = {1, 2};

struct point f(struct point pa, struct point pb, struct point pc)
{
    printid(pa);    //  1 2
    pb.x = 1.1;
    pb.y = 2.2;
    printid(pb);    // 1.1 2.2
    return pb;
}
void main()
{
    struct point pd;
    printid(p);      // 1 2
    pd = f(p, p, p);
    printid(pd);     // 1.1 2.2
}