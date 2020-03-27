
void main()
{
    float a, b, c, d = 0.5;
    a = b = c = d;
    printid(a);   // 0.5
    a = b += c-= d;
    printid(a,b,c,d);
    a = (b = 3.0) + (c = 5.0 + 6.0);
    printid(a);    // 14
}


