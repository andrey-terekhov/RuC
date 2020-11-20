int a = 11,   b = 22, c = 0;

float b1 = 3.14;

void main()
{
    int d = 14 < a ? a : 2 > b ? b : c;
    printid(d);         // 0
    b1 = 14 < a ? a : 2 < b ? b : b1;
    printid(b1);        // 22
    b1 = 1 < a ? a : 2 > b ? b1 : b;
    printid(b1);        // 11
}
