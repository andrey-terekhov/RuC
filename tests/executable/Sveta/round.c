void main()
{
    float xx=2.3, yy=2.51;
    int x = round(xx), y = round(yy);
    print("x 2");
    printid(x);
    print("y 3");
    printid(y);
    print("x -2");
    x = round(-xx);
    printid(x);
    print("y -3");
    y = round(-yy);
    printid(y);
}
