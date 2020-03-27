
void main()
{
    int a, b, c, d;
    a = 3;
    b = 4 + a;
    c = b - (5 - 6);
    d = 7 + 8 + 9;
    printid(a, b, c, d);     // 3 7 8 24
    a = (b + c) * (c - d);   // -240
    printid(a);
}


