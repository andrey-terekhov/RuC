int a = 9, b[3] = {7,8, 9};
float c = 1.2e1, d[2] = {3.14, 2.72};
void MAIN()
{
    a += b[2];
    printid(a);
    c -= d[0];
    printid(c);
    b[0] += 10;
    printid(b);
    d[1] -= 0.70;
    printid(d);
}