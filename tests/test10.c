int a = 11, b = 22, c = 0;
void main()
{
    int d = 1<c ? a : c;
    printid(d);            // 0
    b = (14 > a ? a+4 : a-5) + (c <= a? a : c);
    printid(b);            // 26
}
