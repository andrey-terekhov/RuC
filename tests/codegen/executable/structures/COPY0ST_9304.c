struct point{
    int x;
    int y;
};

struct rect{
    struct point pt1;
    struct point pt2;
} line1, line2;


void main()
{
    struct point p1, p2;
    p1.x = 1; p1.y = 2;
    line1.pt1 = line2.pt2 = p1;

    assert(line1.pt1.x == 1, "line1.pt1.x must be 1");
    assert(line1.pt1.y == 2, "line1.pt1.y must be 2");
    assert(line1.pt2.x == 0, "line1.pt2.x must be 0");
    assert(line1.pt2.y == 0, "line1.pt2.y must be 0");

    assert(line2.pt1.x == 0, "line2.pt1.x must be 0");
    assert(line2.pt1.y == 0, "line2.pt1.y must be 0");
    assert(line2.pt2.x == 1, "line2.pt2.x must be 1");
    assert(line2.pt2.y == 2, "line2.pt2.y must be 2");
}
