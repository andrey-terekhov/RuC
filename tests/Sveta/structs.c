struct point {int x; int y;};
struct line {struct point pt1; struct point pt2 [1];} line1, line2;

void main()
{
    
    struct point p, p1;
    p1.x = 1;
    p1.y = 2;
    line1.pt1 = line2.pt2[0] = p1;
    line1.pt2[0] = line2.pt1 = {0, 0};
    print("line1 {{1, 2}, {0, 0}}");
    printid(line1);
    print("line2 {{0, 0}, {1, 2}}");
    printid(line2);
}

