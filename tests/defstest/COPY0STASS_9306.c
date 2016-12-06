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
    struct point p1,p2;
    p1.x=1;p1.y=2;
    line1.pt1 = line2.pt2=p1;
    print("line1 1 2 0 0");
    printid(line1);
    print("line2 0 0 1 2");
    printid(line2);
}
